/*
 * Copyright (c) 2018,2022 Tristan Le Guern <tleguern@bouledef.eu>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "config.h"

#include <arpa/inet.h>

#include <ctype.h>
#if HAVE_ERR
# include <err.h>
#endif
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <zlib.h>

#include "lgpng.h"
#include "icc.h"

void usage(void);
void info_IHDR(struct IHDR *);
void info_PLTE(struct PLTE *);
void info_IDAT(uint8_t *, size_t);
void info_tRNS(struct IHDR *, struct PLTE *, uint8_t *, size_t);
void info_cHRM(uint8_t *, size_t);
void info_gAMA(uint8_t *, size_t);
void info_iCCP(uint8_t *, size_t);
void info_sBIT(struct IHDR *, uint8_t *, size_t);
void info_sRGB(uint8_t *, size_t);
void info_cICP(uint8_t *, size_t);
void info_tEXt(uint8_t *, size_t);
void info_zTXt(uint8_t *, size_t);
void info_bKGD(struct IHDR *, struct PLTE *, uint8_t *, size_t);
void info_hIST(struct PLTE *, uint8_t *, size_t);
void info_pHYs(uint8_t *, size_t);
void info_sPLT(uint8_t *, size_t);
void info_eXIf(uint8_t *, size_t);
void info_tIME(uint8_t *, size_t);
void info_acTL(uint8_t *, size_t);
void info_fcTL(uint8_t *, size_t);
void info_fdAT(uint8_t *, size_t);
void info_oFFs(uint8_t *, size_t);

int
main(int argc, char *argv[])
{
	int		 ch;
	long		 offset;
	bool		 cflag = false, lflag = true, sflag = false;
	bool		 loopexit = false;
	int		 chunk = CHUNK_TYPE__MAX;
	struct IHDR	 ihdr;
	struct PLTE	 plte;
	FILE		*source = stdin;

#if HAVE_PLEDGE
	pledge("stdio rpath", NULL);
#endif
	(void)memset(&ihdr, 0, sizeof(ihdr));
	(void)memset(&plte, 0, sizeof(plte));
	while (-1 != (ch = getopt(argc, argv, "c:f:ls")))
		switch (ch) {
		case 'c':
			cflag = true;
			lflag = false;
			for (int i = 0; i < CHUNK_TYPE__MAX; i++) {
				if (strcmp(optarg, chunktypemap[i]) == 0) {
					chunk = i;
					break;
				}
			}
			/* TODO: Allow arbitrary chunk ? */
			if (CHUNK_TYPE__MAX == chunk) {
				errx(EXIT_FAILURE, "%s: invalid chunk", optarg);
			}
			break;
		case 'f':
			if (NULL == (source = fopen(optarg, "r"))) {
				err(EXIT_FAILURE, "%s", optarg);
			}
			break;
		case 'l':
			cflag = false;
			lflag = true;
			break;
		case 's':
			sflag = true;
			break;
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	/* Read the file byte by byte until the PNG signature is found */
	offset = 0;
	if (false == sflag) {
		if (false == lgpng_stream_is_png(source)) {
			errx(EXIT_FAILURE, "not a PNG file");
		}
	} else {
		do {
			if (0 != feof(source) || 0 != ferror(source)) {
				errx(EXIT_FAILURE, "not a PNG file");
			}
			if (0 != fseek(source, offset, SEEK_SET)) {
				errx(EXIT_FAILURE, "not a PNG file");
			}
			offset += 1;
		} while (false == lgpng_stream_is_png(source));
	}

	do {
		int		 chunktype = CHUNK_TYPE__MAX;
		uint32_t	 length = 0, chunk_crc = 0, calc_crc = 0;
		uint8_t		*data = NULL;
		uint8_t		 str_type[5] = {0, 0, 0, 0, 0};

		if (false == lgpng_stream_get_length(source, &length)) {
			break;
		}
		if (false == lgpng_stream_get_type(source, &chunktype,
		    (uint8_t *)str_type)) {
			break;
		}
		/*
		 * Do not bother allocating memory in -l mode
		 */
		if (cflag) {
			if (NULL == (data = malloc(length + 1))) {
				warn("malloc(length + 1)");
				break;
			}
			if (false == lgpng_stream_get_data(source, length, &data)) {
				goto stop;
			}
		} else if (lflag) {
			lgpng_stream_skip_data(source, length);
		} else {
			warnx("Internal error");
			break;
		}
		if (false == lgpng_stream_get_crc(source, &chunk_crc)) {
			goto stop;
		}
		if (cflag && false == lgpng_chunk_crc(length, str_type, data,
		    &calc_crc)) {
			warnx("Invalid CRC for chunk %s, skipping", str_type);
			goto stop;
		}
		if (lflag) {
			/* Simply list chunks' name */
			printf("%s\n", str_type);
		} else if (cflag) {
			/*
			 * The IHDR chunk contains important information used to
			 * decode other chunks, such as bKGD, sBIT and tRNS.
			 */
			if (CHUNK_TYPE_IHDR == chunktype) {
				if (-1 == lgpng_create_IHDR_from_data(&ihdr,
				    data, length)) {
					warnx("IHDR: Invalid IHDR chunk");
					loopexit = true;
					goto stop;
				}
			}
			/*
			 * The hIST chunk mirrors the size of the PLTE chunk,
			 * so it is important to keep it around if it is encountered.
			 */
			if (CHUNK_TYPE_PLTE == chunktype) {
				if (-1 == lgpng_create_PLTE_from_data(&plte,
				    data, length)) {
					loopexit = true;
					warnx("PLTE: Invalid PLTE chunk");
					goto stop;
				}
			}
			if (chunktype == chunk) {
				switch (chunktype) {
				case CHUNK_TYPE_IHDR:
					info_IHDR(&ihdr);
					break;
				case CHUNK_TYPE_PLTE:
					info_PLTE(&plte);
					break;
				case CHUNK_TYPE_IDAT:
					info_IDAT(data, length);
					break;
				case CHUNK_TYPE_tRNS:
					info_tRNS(&ihdr, &plte, data, length);
					break;
				case CHUNK_TYPE_cHRM:
					info_cHRM(data, length);
					break;
				case CHUNK_TYPE_gAMA:
					info_gAMA(data, length);
					break;
				case CHUNK_TYPE_iCCP:
					info_iCCP(data, length);
					break;
				case CHUNK_TYPE_sBIT:
					info_sBIT(&ihdr, data, length);
					break;
				case CHUNK_TYPE_sRGB:
					info_sRGB(data, length);
					break;
				case CHUNK_TYPE_cICP:
					info_cICP(data, length);
					break;
				case CHUNK_TYPE_tEXt:
					info_tEXt(data, length);
					break;
				case CHUNK_TYPE_zTXt:
					info_zTXt(data, length);
					break;
				case CHUNK_TYPE_bKGD:
					info_bKGD(&ihdr, &plte, data, length);
					break;
				case CHUNK_TYPE_hIST:
					info_hIST(&plte, data, length);
					break;
				case CHUNK_TYPE_pHYs:
					info_pHYs(data, length);
					break;
				case CHUNK_TYPE_sPLT:
					info_sPLT(data, length);
					break;
				case CHUNK_TYPE_eXIf:
					info_eXIf(data, length);
					break;
				case CHUNK_TYPE_tIME:
					info_tIME(data, length);
					break;
				case CHUNK_TYPE_acTL:
					info_acTL(data, length);
					break;
				case CHUNK_TYPE_fcTL:
					info_fcTL(data, length);
					break;
				case CHUNK_TYPE_fdAT:
					info_fdAT(data, length);
					break;
				case CHUNK_TYPE_oFFs:
					info_oFFs(data, length);
					break;
				case CHUNK_TYPE__MAX:
					/* FALLTHROUGH */
				default:
					errx(EXIT_FAILURE, "Unsupported chunk type");
				}
			}
			free(data);
			data = NULL;
		} else {
			warnx("Internal error");
			break;
		}
stop:
		if (CHUNK_TYPE_IEND == chunktype) {
			loopexit = true;
		}
	} while(! loopexit);
	fclose(source);
	return(EXIT_SUCCESS);
}

void
info_IHDR(struct IHDR *ihdr)
{
	if (0 == ihdr->data.width) {
		warnx("IHDR: Invalid width 0");
	}
	if (0 == ihdr->data.height) {
		warnx("IHDR: Invalid height 0");
	}
	switch (ihdr->data.colourtype) {
	case COLOUR_TYPE_GREYSCALE:
		if (1 != ihdr->data.bitdepth && 2 != ihdr->data.bitdepth
		    && 4 != ihdr->data.bitdepth && 8 != ihdr->data.bitdepth
		    && 16 != ihdr->data.bitdepth) {
			warnx("IHDR: Invalid bit depth %i, should be 1, 2, 4, 8 or 16", ihdr->data.bitdepth);
		}
		break;
	case COLOUR_TYPE_TRUECOLOUR_ALPHA:
		/* FALLTHROUGH */
	case COLOUR_TYPE_GREYSCALE_ALPHA:
		/* FALLTHROUGH */
	case COLOUR_TYPE_TRUECOLOUR:
		if ( 8 != ihdr->data.bitdepth && 16 != ihdr->data.bitdepth) {
			warnx("IHDR: Invalid bit depth %i, should be 8 or 16",
			    ihdr->data.bitdepth);
		}
		break;
	case COLOUR_TYPE_INDEXED:
		if (1 != ihdr->data.bitdepth && 2 != ihdr->data.bitdepth
		    && 4 != ihdr->data.bitdepth && 8 != ihdr->data.bitdepth) {
			warnx("IHDR: Invalid bit depth %i, should be 1, 2, 4 or 8", ihdr->data.bitdepth);
		}
		break;
	case COLOUR_TYPE_FILLER1:
	case COLOUR_TYPE_FILLER5:
	case COLOUR_TYPE__MAX:
	default:
		warnx("IHDR: Invalid colour type %i", ihdr->data.colourtype);
		ihdr->data.colourtype = COLOUR_TYPE_FILLER1;
	}
	printf("IHDR: width: %u\n", ihdr->data.width);
	printf("IHDR: height: %u\n", ihdr->data.height);
	printf("IHDR: bitdepth: %i\n", ihdr->data.bitdepth);
	printf("IHDR: colourtype: %s\n", colourtypemap[ihdr->data.colourtype]);
	if (COMPRESSION_TYPE_DEFLATE != ihdr->data.compression) {
		warnx("IHDR: Invalid compression type %i", ihdr->data.compression);
	} else {
		printf("IHDR: compression: %s\n",
		    compressiontypemap[ihdr->data.compression]);
	}
	if (FILTER_METHOD_ADAPTIVE != ihdr->data.filter) {
		warnx("IHDR: Invalid filter method %i", ihdr->data.filter);
	} else {
		printf("IHDR: filter: %s\n", filtermethodmap[ihdr->data.filter]);
	}
	if (INTERLACE_METHOD_STANDARD != ihdr->data.interlace
	    && INTERLACE_METHOD_ADAM7 != ihdr->data.interlace) {
		warnx("IHDR: Invalid interlace method %i", ihdr->data.interlace);
	} else {
		printf("IHDR: interlace method: %s\n",
		    interlacemap[ihdr->data.interlace]);
	}
}

void
info_PLTE(struct PLTE *plte)
{
	printf("PLTE: %zu entries\n", plte->data.entries);
	for (size_t i = 0; i < plte->data.entries; i++) {
		printf("PLTE: entry %3zu: 0x%02x%02x%02x\n", i,
		    plte->data.entry[i].red,
		    plte->data.entry[i].green,
		    plte->data.entry[i].blue);
	}
}

void
info_IDAT(uint8_t *data, size_t dataz)
{
	struct IDAT idat;

	(void)lgpng_create_IDAT_from_data(&idat, data, dataz);
	printf("IDAT: compressed bytes %u\n", idat.length);
}

void
info_tRNS(struct IHDR *ihdr, struct PLTE *plte, uint8_t *data, size_t dataz)
{
	struct tRNS trns;

	if (-1 == lgpng_create_tRNS_from_data(&trns, ihdr, data, dataz)) {
		warnx("Bad tRNS chunk, skipping.");
		return;
	}
	switch (ihdr->data.colourtype) {
	case COLOUR_TYPE_GREYSCALE:
		printf("tRNS: gray: %u\n", trns.data.gray);
		break;
	case COLOUR_TYPE_TRUECOLOUR:
		printf("tRNS: red: %u\n", trns.data.red);
		printf("tRNS: green: %u\n", trns.data.green);
		printf("tRNS: blue: %u\n", trns.data.blue);
		break;
	case COLOUR_TYPE_INDEXED:
		if (trns.data.entries > plte->data.entries) {
			warnx("tRNS should not have more entries than PLTE");
		}
		for (size_t i = 0; i < trns.data.entries; i++) {
			printf("tRNS: palette index %zu: %u\n",
			    i, trns.data.palette[i]);
		}
		break;
	default:
		errx(EXIT_FAILURE, "%s: wrong call to info_tRNS", getprogname());
	}
}

void
info_cHRM(uint8_t *data, size_t dataz)
{
	double 		whitex, whitey;
	double		redx, redy;
	double		greenx, greeny;
	double		bluex, bluey;
	struct cHRM	chrm;

	if (-1 == lgpng_create_cHRM_from_data(&chrm, data, dataz)) {
		warnx("Bad cHRM chunk, skipping.");
		return;
	}
	whitex = (double)chrm.data.whitex / 100000.0;
	whitey = (double)chrm.data.whitey / 100000.0;
	redx = (double)chrm.data.redx / 100000.0;
	redy = (double)chrm.data.redy / 100000.0;
	greenx = (double)chrm.data.greenx / 100000.0;
	greeny = (double)chrm.data.greeny / 100000.0;
	bluex = (double)chrm.data.bluex / 100000.0;
	bluey = (double)chrm.data.bluey / 100000.0;
	printf("cHRM: white point x: %f\n", whitex);
	printf("cHRM: white point y: %f\n", whitey);
	printf("cHRM: red x: %f\n", redx);
	printf("cHRM: red y: %f\n", redy);
	printf("cHRM: green x: %f\n", greenx);
	printf("cHRM: green y: %f\n", greeny);
	printf("cHRM: blue x: %f\n", bluex);
	printf("cHRM: blue y: %f\n", bluey);
}

void
info_gAMA(uint8_t *data, size_t dataz)
{
	struct gAMA	gama;

	if (-1 == lgpng_create_gAMA_from_data(&gama, data, dataz)) {
		warnx("Bad gAMA chunk, skipping.");
		return;
	}
	printf("gAMA: image gamma: %u\n", gama.data.gamma);
}

void
info_iCCP(uint8_t *data, size_t dataz)
{
	int			 retry = 2, zret;
	size_t			 outz, outtmpz;
	struct iCCP		 iccp;
	struct icc_profile	 profile;
	uint8_t			*out = NULL, *outtmp = NULL;

	if (-1 == lgpng_create_iCCP_from_data(&iccp, data, dataz)) {
		warnx("Bad iCCP chunk, skipping.");
		return;
	}
	printf("iCCP: profile name: %s\n", iccp.data.name);
	printf("iCCP: compression method: %s\n",
	    compressiontypemap[iccp.data.compression]);
	do {
		outtmpz = iccp.data.profilez * retry;
		if (NULL == (outtmp = realloc(out, outtmpz + 1))) {
			warn("realloc(outtmpz + 1)");
			goto error;
		}
		out = outtmp;
		outz = outtmpz;
		zret = uncompress(out, &outz,
		    iccp.data.profile, iccp.data.profilez);
		if (Z_BUF_ERROR != zret && Z_OK != zret) {
			if (Z_MEM_ERROR == zret) {
				warn("uncompress(iccp.data.textz)");
			} else if (Z_DATA_ERROR == zret) {
				warnx("Invalid input data");
			}
			warnx("iCCP: Failed decompression");
			goto error;
		}
		out[outz] = '\0';
		retry += 1;
	} while (Z_OK != zret);
	printf("iCCP: profile size: %zu\n", outz);
	if (false == icc_create_from_data(&profile, out, outz)) {
		warnx("Invalid ICC profile");
		return;
	}
	printf("iCCP: profile: CMM Type signature: %c%c%c%c\n",
	    profile.cmm_type_sig[0], profile.cmm_type_sig[1],
	    profile.cmm_type_sig[2], profile.cmm_type_sig[3]);
	printf("iCCP: profile: version: %d.%d.%d\n",
	    profile.major_version,
	    profile.minor_version,
	    profile.patch_version);
	printf("iCCP: profile: class: %s\n",
	    icc_classmap[profile.class]);
	printf("iCCP: profile: color space: %s\n",
	    icc_colorspacemap[profile.colorspace]);
	printf("iCCP: profile: connection space: %s\n",
	    icc_colorspacemap[profile.profile_colorspace]);
	printf("iCCP: profile: creation date: %d-%02d-%02d %02d:%02d:%02d\n",
	    profile.year, profile.month, profile.day, profile.hour,
	    profile.minute, profile.second);
	printf("iCCP: profile: primary platform: %s\n",
	    icc_platformmap[profile.platform]);
	printf("iCCP: profile: flags: not impemented\n");
	printf("iCCP: profile: device manufacturer: %s\n", profile.manufacturer);
	printf("iCCP: profile: device model: %s\n", profile.model);
	printf("iCCP: profile: attributes: not impemented\n");
	printf("iCCP: profile: rendering intent: %s\n",
	    icc_rendering_intentmap[profile.rendering_intent]);
	printf("iCCP: profile: creator: %s\n", profile.creator);
	printf("iCCP: entries in the tag table: %d\n",
	    profile.tag_count);
error:
	free(out);
	out = NULL;
	outz = 0;
}

void
info_sBIT(struct IHDR *ihdr, uint8_t *data, size_t dataz)
{
	struct sBIT sbit;

	if (-1 == lgpng_create_sBIT_from_data(&sbit, ihdr, data, dataz)) {
		warnx("Bad sBIT chunk, skipping.");
		return;
	}
	if (COLOUR_TYPE_GREYSCALE == ihdr->data.colourtype
	    || COLOUR_TYPE_GREYSCALE_ALPHA == ihdr->data.colourtype) {
		printf("sBIT: significant greyscale bits: %i\n",
		    sbit.data.sgreyscale);
	} else if (COLOUR_TYPE_TRUECOLOUR == ihdr->data.colourtype
	    || COLOUR_TYPE_INDEXED == ihdr->data.colourtype
	    || COLOUR_TYPE_TRUECOLOUR_ALPHA == ihdr->data.colourtype) {
		printf("sBIT: significant red bits: %i\n",
		    sbit.data.sred);
		printf("sBIT: significant green bits: %i\n",
		    sbit.data.sgreen);
		printf("sBIT: significant blue bits: %i\n",
		    sbit.data.sblue);

	}
	if (COLOUR_TYPE_GREYSCALE_ALPHA == ihdr->data.colourtype
	    || COLOUR_TYPE_TRUECOLOUR_ALPHA == ihdr->data.colourtype) {
		printf("sBIT: significant alpha bits: %i\n",
		    sbit.data.salpha);
	}
}

void
info_sRGB(uint8_t *data, size_t dataz)
{
	struct sRGB srgb;

	if (-1 == lgpng_create_sRGB_from_data(&srgb, data, dataz)) {
		warnx("Bad sRGB chunk, skipping.");
		return;
	}
	if (srgb.data.intent >= RENDERING_INTENT__MAX) {
		warnx("sRGB: invalid rendering intent value");
		return;
	}
	printf("sRGB: rendering intent: %s\n",
	    rendering_intentmap[srgb.data.intent]);
}

void
info_cICP(uint8_t *data, size_t dataz)
{
	struct cICP cicp;

	if (-1 == lgpng_create_cICP_from_data(&cicp, data, dataz)) {
		warnx("Bad cICP chunk, skipping.");
		return;
	}
	printf("cICP: colour primaries: %d\n", cicp.data.colour_primaries);
	printf("cICP: transfer function: %d\n", cicp.data.transfer_function);
	printf("cICP: matrix coefficients: %d\n", cicp.data.matrix_coefficients);
	printf("cICP: video full range flag: %d\n", cicp.data.video_full_range);
}

void
info_tEXt(uint8_t *data, size_t dataz)
{
	struct tEXt	text;

	if (-1 == lgpng_create_tEXt_from_data(&text, data, dataz)) {
		warnx("Bad tEXt chunk, skipping.");
		return;
	}

	if (!lgpng_is_official_keyword(text.data.keyword,
	    strlen((char *)text.data.keyword))) {
		printf("tEXt: %s is not an official keyword\n",
		    text.data.keyword);
	}
	printf("tEXt: %s: %s\n", text.data.keyword, text.data.text);
}

void
info_zTXt(uint8_t *data, size_t dataz)
{
	int		 retry = 2, zret;
	size_t		 outz, outtmpz;
	struct zTXt	 ztxt;
	uint8_t		*out = NULL, *outtmp = NULL;

	lgpng_create_zTXt_from_data(&ztxt, data, dataz);
	if (-1 == lgpng_create_zTXt_from_data(&ztxt, data, dataz)) {
		warnx("Bad zTXt chunk, skipping.");
		return;
	}
	if (!lgpng_is_official_keyword(ztxt.data.keyword,
	    strlen((char *)ztxt.data.keyword))) {
		printf("zTXt: %s is not an official keyword\n",
		    ztxt.data.keyword);
	}
	printf("zTXt: compression method: %s\n",
	    compressiontypemap[ztxt.data.compression]);
	do {
		outtmpz = ztxt.data.textz * retry;
		if (NULL == (outtmp = realloc(out, outtmpz + 1))) {
			warn("realloc(outtmpz + 1)");
			goto error;
		}
		out = outtmp;
		outz = outtmpz;
		zret = uncompress(out, &outz, ztxt.data.text, ztxt.data.textz);
		if (Z_BUF_ERROR != zret && Z_OK != zret) {
			if (Z_MEM_ERROR == zret) {
				warn("uncompress(ztxt.data.textz)");
			} else if (Z_DATA_ERROR == zret) {
				warnx("Invalid input data");
			}
			warnx("zTXt: Failed decompression");
			goto error;
		}
		out[outz] = '\0';
		retry += 1;
	} while (Z_OK != zret);
	if (outz > ztxt.data.textz) {
		printf("zTXt: compressed data is bigger than uncompressed\n");
	}
	printf("zTXt: keyword: %s\n", ztxt.data.keyword);
	printf("zTXt: text: %s\n", out);
error:
	free(out);
	out = NULL;
	outz = 0;
}

void
info_bKGD(struct IHDR *ihdr, struct PLTE *plte, uint8_t *data, size_t dataz)
{
	struct bKGD bkgd;

	lgpng_create_bKGD_from_data(&bkgd, ihdr, plte, data, dataz);
	switch (ihdr->data.colourtype) {
	case COLOUR_TYPE_GREYSCALE:
		/* FALLTHROUGH */
	case COLOUR_TYPE_GREYSCALE_ALPHA:
		if (ihdr->data.bitdepth < 16) {
			if (0 != MSB16(bkgd.data.greyscale)) {
				printf("bKGD: MSB is not zero for greyscale value\n");
			}
			printf("bKGD: greyscale 0x%04x\n",
			    LSB16(bkgd.data.greyscale));
		} else {
			printf("bKGD: greyscale 0x%04x\n", bkgd.data.greyscale);
		}
		break;
	case COLOUR_TYPE_TRUECOLOUR:
		/* FALLTHROUGH */
	case COLOUR_TYPE_TRUECOLOUR_ALPHA:
		if (ihdr->data.bitdepth < 16) {
			if (0 != MSB16(bkgd.data.rgb.red)) {
				printf("bKGD: MSB is not zero for red value\n");
			}
			if (0 != MSB16(bkgd.data.rgb.green)) {
				printf("bKGD: MSB is not zero for green value\n");
			}
			if (0 != MSB16(bkgd.data.rgb.blue)) {
				printf("bKGD: MSB is not zero for blue value\n");
			}
			printf("bKGD: rgb value 0x%x%x%x\n",
			    LSB16(bkgd.data.rgb.red),
			    LSB16(bkgd.data.rgb.green),
			    LSB16(bkgd.data.rgb.blue));
		} else {
			printf("bKGD: rgb value 0x%x%x%x\n",
			    bkgd.data.rgb.red,
			    bkgd.data.rgb.green,
			    bkgd.data.rgb.blue);
		}
		break;
	case COLOUR_TYPE_INDEXED:
		printf("bKGD: palette index %u\n", bkgd.data.paletteindex);
		printf("bKGD: PLTE entry 0x%02x%02x%02x\n",
		    plte->data.entry[bkgd.data.paletteindex].red,
		    plte->data.entry[bkgd.data.paletteindex].green,
		    plte->data.entry[bkgd.data.paletteindex].blue);
		break;
	}
}

void
info_hIST(struct PLTE *plte, uint8_t *data, size_t dataz)
{
	struct hIST hist;

	if (-1 == lgpng_create_hIST_from_data(&hist, plte, data, dataz)) {
		warnx("Bad hIST chunk");
		return;
	}
	for (size_t i = 0; i < plte->data.entries; i++) {
		printf("hIST: entry %3zu: %d\n", i,
		    hist.data.frequency[i]);
	}
}

void
info_pHYs(uint8_t *data, size_t dataz)
{
	struct pHYs phys;

	lgpng_create_pHYs_from_data(&phys, data, dataz);
	printf("pHYs: pixel per unit, X axis: %i\n", phys.data.ppux);
	printf("pHYs: pixel per unit, Y axis: %i\n", phys.data.ppuy);
	printf("pHYs: unit specifier: %s\n",
	    unitspecifiermap[phys.data.unitspecifier]);
}

void
info_sPLT(uint8_t *data, size_t dataz)
{
	struct sPLT splt;

	lgpng_create_sPLT_from_data(&splt, data, dataz);
	printf("sPLT: palette name: %s\n", splt.data.palettename);
	printf("sPLT: sample depth: %i\n", splt.data.sampledepth);
	printf("sPLT: %zu entries\n", splt.data.entries);
}

void
info_eXIf(uint8_t *data, size_t dataz)
{
	struct eXIf exif;
	uint8_t little_endian[4] = { 73, 73, 42, 0};
	uint8_t big_endian[4] = { 77, 77, 0, 42};

	lgpng_create_eXIf_from_data(&exif, data, dataz);
	if (0 == memcmp(exif.data.profile, little_endian, 4)) {
		printf("eXIf: endianese: little-endian\n");
	} else if (0 == memcmp(exif.data.profile, big_endian, 4)) {
		printf("eXIf: endianese: big-endian\n");
	} else {
		printf("eXIf: endianese: weird\n");
	}
}

void
info_tIME(uint8_t *data, size_t dataz)
{
	struct tIME time;

	lgpng_create_tIME_from_data(&time, data, dataz);
	if (time.data.month == 0 || time.data.month > 12) {
		warnx("tIME: invalid month value");
	}
	if (time.data.day == 0 || time.data.day > 31) {
		warnx("tIME: invalid day value");
	}
	if (time.data.hour > 23) {
		warnx("tIME: invalid hour value");
	}
	if (time.data.minute > 59) {
		warnx("tIME: invalid minute value");
	}
	if (time.data.second > 60) {
		warnx("tIME: invalid second value");
	}
	printf("tIME: %i-%02i-%02i %02i:%02i:%02i\n",
	    time.data.year, time.data.month, time.data.day,
	    time.data.hour, time.data.minute, time.data.second);
}

void
info_acTL(uint8_t *data, size_t dataz)
{
	struct acTL actl;

	lgpng_create_acTL_from_data(&actl, data, dataz);
	printf("acTL: number of frames: %u\n", actl.data.num_frames);
	if (0 == actl.data.num_plays) {
		printf("acTL: number of plays: indefinitely\n");
	} else {
		printf("acTL: number of plays: %u\n", actl.data.num_plays);
	}
}

void
info_fcTL(uint8_t *data, size_t dataz)
{
	struct fcTL fctl;

	lgpng_create_fcTL_from_data(&fctl, data, dataz);
	printf("fcTL: squence number: %d\n", fctl.data.sequence_number);
	printf("fcTL: width: %d\n", fctl.data.width);
	printf("fcTL: height: %d\n", fctl.data.height);
	printf("fcTL: x_offset: %d\n", fctl.data.x_offset);
	printf("fcTL: y_offset: %d\n", fctl.data.y_offset);
	printf("fcTL: delay_num: %d\n", fctl.data.delay_num);
	printf("fcTL: delay_den: %d\n", fctl.data.delay_den);
	printf("fcTL: dispose_op: %s\n", dispose_opmap[fctl.data.dispose_op]);
	printf("fcTL: blend_op: %s\n", blend_opmap[fctl.data.blend_op]);
}

void
info_fdAT(uint8_t *data, size_t dataz)
{
	struct fdAT fdat;

	lgpng_create_fdAT_from_data(&fdat, data, dataz);
	printf("fdAT: squence number: %d\n", fdat.data.sequence_number);
}

void
info_oFFs(uint8_t *data, size_t dataz)
{
	struct oFFs offs;

	if (-1 == lgpng_create_oFFs_from_data(&offs, data, dataz)) {
		warnx("Bad oFFs chunk, skipping.");
		return;
	}
	printf("oFFs: x position: %d\n", offs.data.x_position);
	printf("oFFs: y position: %d\n", offs.data.y_position);
	printf("oFFs: unit specifier: %s\n", offsunitspecifiermap[offs.data.unitspecifier]);
}

void
usage(void)
{
	fprintf(stderr, "usage: %s [-l] [-c chunk] [-f file]\n", getprogname());
	exit(EXIT_FAILURE);
}

