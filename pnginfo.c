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

void usage(void);
int  info_zlib(uint8_t, uint8_t, uint8_t [4]);
void info_IHDR(struct IHDR *);
void info_PLTE(struct PLTE *);
void info_IDAT(uint8_t *, size_t, int);
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
void info_gIFg(uint8_t *, size_t);
void info_gIFx(uint8_t *, size_t);
void info_vpAg(uint8_t *, size_t);
void info_caNv(uint8_t *, size_t);
void info_orNt(uint8_t *, size_t);
void info_skMf(uint8_t *, size_t);
void info_skRf(uint8_t *, size_t);
void info_unknown(uint8_t [5], uint8_t *, size_t);

int
main(int argc, char *argv[])
{
	int		 ch, idatnum = 0;
	long		 offset;
	bool		 cflag = false, dflag = false;
	bool		 lflag = true, sflag = false;
	bool		 loopexit = false;
	int		 chunk = CHUNK_TYPE__MAX;
	struct IHDR	 ihdr;
	struct PLTE	 plte;
	FILE		*source = stdin;
	uint8_t		 str_cflag[4] = {0, 0, 0, 0};

#if HAVE_PLEDGE
	pledge("stdio rpath", NULL);
#endif
	(void)memset(&ihdr, 0, sizeof(ihdr));
	(void)memset(&plte, 0, sizeof(plte));
	while (-1 != (ch = getopt(argc, argv, "c:df:ls")))
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
			if (CHUNK_TYPE__MAX == chunk) {
				(void)memcpy(str_cflag, optarg, 4);
			}
			break;
		case 'd':
			dflag = true;
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

	if (dflag && ! cflag) {
		errx(EXIT_FAILURE, "-d is only valid with -c");
	}

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
		/* Validate the CRC in chunk mode */
		if (cflag && false == lgpng_chunk_crc(length, str_type, data,
		    &calc_crc)) {
			warnx("Invalid CRC for chunk %s, skipping", str_type);
			goto stop;
		}
		if (lflag) {
			/* Simply list chunks' name */
			printf("%.4s\n", str_type);
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
			if (chunktype == chunk && dflag) {
				/* Dump the chunk's data */
				if (length != fwrite(data, 1, length, stdout)) {
					warn("I/O error");
				}
			} else if (chunktype == chunk && !dflag) {
				switch (chunktype) {
				case CHUNK_TYPE_IHDR:
					info_IHDR(&ihdr);
					break;
				case CHUNK_TYPE_PLTE:
					info_PLTE(&plte);
					break;
				case CHUNK_TYPE_IDAT:
					info_IDAT(data, length, idatnum);
					idatnum += 1;
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
				case CHUNK_TYPE_gIFg:
					info_gIFg(data, length);
					break;
				case CHUNK_TYPE_gIFx:
					info_gIFx(data, length);
					break;
				case CHUNK_TYPE_vpAg:
					info_vpAg(data, length);
					break;
				case CHUNK_TYPE_caNv:
					info_caNv(data, length);
					break;
				case CHUNK_TYPE_orNt:
					info_orNt(data, length);
					break;
				case CHUNK_TYPE_skMf:
					info_skMf(data, length);
					break;
				case CHUNK_TYPE_skRf:
					info_skRf(data, length);
					break;

				case CHUNK_TYPE__MAX:
					/* FALLTHROUGH */
				default:
					if (0 == memcmp(str_type, str_cflag, 4)) {
						info_unknown(str_type, data, length);
					}
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

int
info_zlib(uint8_t cmf, uint8_t flg, uint8_t chunk[4])
{
	uint8_t          cm, cinfo;
	uint8_t          fcheck, fdict, flevel;
	const char	*level;

	/* See RFC1950 ZLIB Data Format */
	/* cmf */
	cm = cmf & 0x0f;
	if (cm != 8) {
		fprintf(stderr, "%.4s: zlib invalid compression method %i\n",
		    chunk, cm);
		return(-1);
	}
	cinfo = (cmf & 0xf0) >> 4;
	if (cinfo > 7) {
		fprintf(stderr, "%.4s: zlib invalid compression info %i\n",
		     chunk, cinfo);
		return(-1);
	}
	/* flg */
	fcheck = (flg & 0xf0) >> 4;
	fdict = flg & (0x01 << 5);
	if (fdict == 1) {
		fprintf(stderr, "%.4s: zlib invalid compression info: "
		    "preset dictionary\n", chunk);
	}
	flevel = (flg & 0xc0) >> 6;
        switch (flevel) {
                case 0: level = "fastest"; break;
                case 1: level = "fast"; break;
                case 2: level = "default"; break;
                case 3: level = "slowest"; break;
                default: level = "error"; break;
        }
        printf("%.4s: zlib compression method: %i\n", chunk, cm);
        printf("%.4s: zlib window size: %i\n", chunk, cinfo);
        printf("%.4s: zlib check bits: %i\n", chunk, fcheck);
        printf("%.4s: zlib presset dictionnary: %s\n", chunk,
	    fdict ? "true" : "false");
        printf("%.4s: zlib compression level: %s\n", chunk, level);
	return(0);
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
info_IDAT(uint8_t *data, size_t dataz, int idatnum)
{
	struct IDAT	 idat;

	(void)lgpng_create_IDAT_from_data(&idat, data, dataz);
	printf("IDAT: compressed bytes %u\n", idat.length);
	if (dataz && idatnum == 0) {
		info_zlib(data[0], data[1], (uint8_t *)"IDAT");
	}
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
	struct iCCP		 iccp;

	if (-1 == lgpng_create_iCCP_from_data(&iccp, data, dataz)) {
		warnx("Bad iCCP chunk, skipping.");
		return;
	}
	printf("iCCP: profile name: %.80s\n", iccp.data.name);
	printf("iCCP: compression method: %s\n",
	    compressiontypemap[iccp.data.compression]);
	info_zlib(iccp.data.profile[0], iccp.data.profile[1], (uint8_t *)"iCCP");
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
	info_zlib(ztxt.data.text[0], ztxt.data.text[1], (uint8_t *)"zTXt");
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

	if (-1 == lgpng_create_bKGD_from_data(&bkgd, ihdr, plte, data, dataz)) {
		warnx("Bad bKGD chunk");
		return;
	}
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

	if (-1 == lgpng_create_pHYs_from_data(&phys, data, dataz)) {
		warnx("Bad pHYs chunk");
		return;
	}
	printf("pHYs: pixel per unit, X axis: %u\n", phys.data.ppux);
	printf("pHYs: pixel per unit, Y axis: %u\n", phys.data.ppuy);
	printf("pHYs: unit specifier: %s\n",
	    unitspecifiermap[phys.data.unitspecifier]);
}

void
info_sPLT(uint8_t *data, size_t dataz)
{
	struct sPLT splt;

	if (-1 == lgpng_create_sPLT_from_data(&splt, data, dataz)) {
		warnx("Bad sPLT chunk, skipping.");
		return;
	}
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

	if (-1 == lgpng_create_tIME_from_data(&time, data, dataz)) {
		warnx("Bad tIME chunk, skipping.");
		return;
	}
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

	if (-1 == lgpng_create_acTL_from_data(&actl, data, dataz)) {
		warnx("Bad acTL chunk, skipping.");
		return;
	}
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

	if (-1 == lgpng_create_fcTL_from_data(&fctl, data, dataz)) {
		warnx("Bad fcTL chunk, skipping.");
		return;
	}
	printf("fcTL: squence number: %u\n", fctl.data.sequence_number);
	printf("fcTL: width: %u\n", fctl.data.width);
	printf("fcTL: height: %u\n", fctl.data.height);
	printf("fcTL: x_offset: %u\n", fctl.data.x_offset);
	printf("fcTL: y_offset: %u\n", fctl.data.y_offset);
	printf("fcTL: delay_num: %u\n", fctl.data.delay_num);
	printf("fcTL: delay_den: %u\n", fctl.data.delay_den);
	printf("fcTL: dispose_op: %s\n", dispose_opmap[fctl.data.dispose_op]);
	printf("fcTL: blend_op: %s\n", blend_opmap[fctl.data.blend_op]);
}

void
info_fdAT(uint8_t *data, size_t dataz)
{
	struct fdAT fdat;

	if (-1 == lgpng_create_fdAT_from_data(&fdat, data, dataz)) {
		warnx("Bad fdAT chunk, skipping.");
		return;
	}
	printf("fdAT: squence number: %u\n", fdat.data.sequence_number);
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
info_gIFg(uint8_t *data, size_t dataz)
{
	struct gIFg gifg;

	if (-1 == lgpng_create_gIFg_from_data(&gifg, data, dataz)) {
		warnx("Bad gIFg chunk, skipping.");
		return;
	}
	printf("gIFg: disposal method: %s\n", disposal_methodmap[gifg.data.disposal_method]);
	printf("gIFg: user input: %s\n", user_inputmap[gifg.data.user_input]);
	printf("gIFg: delay time: %hu\n", gifg.data.delay_time);
}

void
info_gIFx(uint8_t *data, size_t dataz)
{
	struct gIFx	gifx;

	if (-1 == lgpng_create_gIFx_from_data(&gifx, data, dataz)) {
		warnx("Bad gIFx chunk, skipping.");
		return;
	}
	printf("gIFx: application identifier: %.8s\n", gifx.data.identifier);
	printf("gIFx: application code: %.3s\n", gifx.data.code);
	printf("gIFx: application data: ");
	if (gifx.length - 11 > 0) {
		for (unsigned long i = 0; i < gifx.length - 11; i++) {
			printf("%x", data[i + 11]);
			if (i != gifx.length - 12) {
				printf(" ");
			}
		}
		printf("\n");
	}
}

void
info_vpAg(uint8_t *data, size_t dataz)
{
	struct vpAg vpag;

	if (-1 == lgpng_create_vpAg_from_data(&vpag, data, dataz)) {
		warnx("Bad vpAg chunk, skipping.");
		return;
	}
	printf("vpAg: width: %u\n", vpag.data.width);
	printf("vpAg: height: %u\n", vpag.data.height);
	printf("vpAg: unit specifier: %s\n", vpagunitspecifiermap[vpag.data.unitspecifier]);
}


void
info_caNv(uint8_t *data, size_t dataz)
{
	struct caNv canv;

	if (-1 == lgpng_create_caNv_from_data(&canv, data, dataz)) {
		warnx("Bad caNv chunk, skipping.");
		return;
	}
	printf("caNv: width: %u\n", canv.data.width);
	printf("caNv: height: %u\n", canv.data.height);
	printf("caNv: x position: %d\n", canv.data.x_position);
	printf("caNv: y position: %d\n", canv.data.y_position);
}

void
info_orNt(uint8_t *data, size_t dataz)
{
	struct orNt ornt;

	if (-1 == lgpng_create_orNt_from_data(&ornt, data, dataz)) {
		warnx("Bad orNt chunk, skipping.");
		return;
	}
	printf("orNt: orientation: %s\n", orientationmap[ornt.data.orientation]);
}

void
info_skMf(uint8_t *data, size_t dataz)
{
	struct skMf skmf;

	if (-1 == lgpng_create_skMf_from_data(&skmf, data, dataz)) {
		warnx("Bad skMf chunk, skipping.");
		return;
	}
	printf("skMf: json data: %s\n", skmf.data.json);
}

void
info_skRf(uint8_t *data, size_t dataz)
{
	struct skRf skrf;

	if (-1 == lgpng_create_skRf_from_data(&skrf, data, dataz)) {
		warnx("Bad skRf chunk, skipping.");
		return;
	}
	printf("skRf: header: %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x\n",
	    skrf.data.header[0], skrf.data.header[1], skrf.data.header[2],
	    skrf.data.header[3], skrf.data.header[4], skrf.data.header[5],
	    skrf.data.header[6], skrf.data.header[7], skrf.data.header[8],
	    skrf.data.header[9], skrf.data.header[11], skrf.data.header[12],
	    skrf.data.header[13], skrf.data.header[14], skrf.data.header[15]);
	printf("skRf: embeded PNG image size: %u bytes\n", skrf.length - 16);
}

void
info_unknown(uint8_t name[5], uint8_t *data, size_t dataz)
{
	(void)data;
	printf("%s: bytes %zu\n", name, dataz);
}

void
usage(void)
{
	fprintf(stderr, "usage: %s [-l] [-c chunk] [-f file]\n", getprogname());
	exit(EXIT_FAILURE);
}

