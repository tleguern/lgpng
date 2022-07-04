#include <arpa/inet.h>

#include <ctype.h>
#include <err.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "lgpng.h"

void usage(void);
void info_IHDR(struct IHDR *);
void info_PLTE(struct PLTE *);
void info_tRNS(uint8_t *, size_t);
void info_cHRM(uint8_t *, size_t);
void info_gAMA(uint8_t *, size_t);
void info_sBIT(uint8_t *, size_t);
void info_sRGB(uint8_t *, size_t);
void info_tEXt(uint8_t *, size_t);
void info_zTXt(uint8_t *, size_t);
void info_bKGD(struct IHDR *, struct PLTE *, uint8_t *, size_t);
void info_hIST(struct PLTE *, uint8_t *, size_t);
void info_pHYs(uint8_t *, size_t);
void info_sPLT(uint8_t *, size_t);
void info_tIME(uint8_t *, size_t);

void
print_position(FILE *file)
{
	fpos_t		 pos = 0;

	(void)fgetpos(file, &pos);
	fprintf(stderr, "Position is %lld\n", pos);
}

int
main(int argc, char *argv[])
{
	int		 ch;
	long		 offset;
	bool		 cflag = false, lflag = true, sflag = false;
	bool		 loopexit = false;
	enum chunktype	 chunk = CHUNK_TYPE__MAX;
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
		if (false == lgpng_is_stream_png(source)) {
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
		} while (false == lgpng_is_stream_png(source));
	}

	do {
		enum chunktype		 chunktype;
		struct unknown_chunk	 unknown_chunk;
		uint8_t			*data = NULL;

		if (-1 == lgpng_get_next_chunk_from_stream(source, &unknown_chunk, &data)) {
			break;
		}
		/* TODO:
		if (-1 == lgpng_validate_chunk_crc(&unknown_chunk)) {
			...
		}
		 */
		chunktype = lgpng_identify_chunk(&unknown_chunk);
		if (lflag) {
			/* Simply list chunks' name */
			if (CHUNK_TYPE__MAX == chunktype) {
				printf("%s\n", unknown_chunk.type);
			} else {
				printf("%s\n", chunktypemap[chunktype]);
			}
		}
		/*
		 * The IHDR chunk contains important information used to
		 * decode other chunks, such as bKGD, sBIT and tRNS.
		 */
		if (CHUNK_TYPE_IHDR == chunktype) {
			lgpng_create_IHDR_from_data(&ihdr, data, unknown_chunk.length);
		}
		/*
		 * The hIST chunk mirrors the size of the PLTE chunk,
		 * so it is important to keep it around if it is encountered.
		 */
		if (CHUNK_TYPE_PLTE == chunktype) {
			if (-1 == lgpng_create_PLTE_from_data(&plte, data, unknown_chunk.length)) {
				warnx("PLTE: Invalid PLTE chunk");
			}

		}
		if (cflag && chunktype == chunk) {
			size_t dataz = unknown_chunk.length;

			switch (chunktype) {
			case CHUNK_TYPE_IHDR:
				info_IHDR(&ihdr);
				break;
			case CHUNK_TYPE_PLTE:
				info_PLTE(&plte);
				break;
			case CHUNK_TYPE_tRNS:
				info_tRNS(data, dataz);
				break;
			case CHUNK_TYPE_cHRM:
				info_cHRM(data, dataz);
				break;
			case CHUNK_TYPE_gAMA:
				info_gAMA(data, dataz);
				break;
			case CHUNK_TYPE_sBIT:
				info_sBIT(data, dataz);
				break;
			case CHUNK_TYPE_sRGB:
				info_sRGB(data, dataz);
				break;
			case CHUNK_TYPE_tEXt:
				info_tEXt(data, dataz);
				break;
			case CHUNK_TYPE_zTXt:
				info_zTXt(data, dataz);
				break;
			case CHUNK_TYPE_bKGD:
				info_bKGD(&ihdr, &plte, data, dataz);
				break;
			case CHUNK_TYPE_hIST:
				info_hIST(&plte, data, dataz);
				break;
			case CHUNK_TYPE_pHYs:
				info_pHYs(data, dataz);
				break;
			case CHUNK_TYPE_sPLT:
				info_sPLT(data, dataz);
				break;
			case CHUNK_TYPE_tIME:
				info_tIME(data, dataz);
				break;
			case CHUNK_TYPE__MAX:
				/* FALLTHROUGH */
			default:
				errx(EXIT_FAILURE, "Unsupported chunk type");
			}
		}
		if (CHUNK_TYPE_IEND == chunktype) {
			loopexit = true;
		}
		free(data);
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
info_tRNS(uint8_t *data, size_t dataz)
{
	struct tRNS trns;

	lgpng_create_tRNS_from_data(&trns, data, dataz);
}

void
info_cHRM(uint8_t *data, size_t dataz)
{
	double 		whitex, whitey;
	double		redx, redy;
	double		greenx, greeny;
	double		bluex, bluey;
	struct cHRM	chrm;

	lgpng_create_cHRM_from_data(&chrm, data, dataz);
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

	lgpng_create_gAMA_from_data(&gama, data, dataz);
	printf("gAMA: image gamma: %u\n", gama.data.gamma);
}

void
info_sBIT(uint8_t *data, size_t dataz)
{
	struct sBIT sbit;

	lgpng_create_sBIT_from_data(&sbit, data, dataz);
}

void
info_sRGB(uint8_t *data, size_t dataz)
{
	struct sRGB srgb;

	lgpng_create_sRGB_from_data(&srgb, data, dataz);
	if (srgb.data.intent >= RENDERING_INTENT__MAX) {
		warnx("sRGB: invalid rendering intent value");
		return;
	}
	printf("sRGB: rendering intent: %s\n",
	    rendering_intentmap[srgb.data.intent]);
}

void
info_tEXt(uint8_t *data, size_t dataz)
{
	struct tEXt	text;

	lgpng_create_tEXt_from_data(&text, data, dataz);

	/* TODO: Check if keywords are from the registered list or not */
	printf("tEXt: keyword: %s\n", text.data.keyword);
	printf("tEXt: text: %s\n", text.data.text);
}

void
info_zTXt(uint8_t *data, size_t dataz)
{
	struct zTXt	ztxt;

	lgpng_create_zTXt_from_data(&ztxt, data, dataz);
}

#define BINARY_PATTERN_INT8 "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY_INT8(i)    \
	(((i) & 0x80ll) ? '1' : '0'), \
	(((i) & 0x40ll) ? '1' : '0'), \
	(((i) & 0x20ll) ? '1' : '0'), \
	(((i) & 0x10ll) ? '1' : '0'), \
	(((i) & 0x08ll) ? '1' : '0'), \
	(((i) & 0x04ll) ? '1' : '0'), \
	(((i) & 0x02ll) ? '1' : '0'), \
	(((i) & 0x01ll) ? '1' : '0')
#define BINARY_PATTERN_INT16 \
	BINARY_PATTERN_INT8 " " BINARY_PATTERN_INT8
#define BYTE_TO_BINARY_INT16(i) \
	BYTE_TO_BINARY_INT8((i) >> 8), BYTE_TO_BINARY_INT8(i)

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
	printf("tIME: %i-%i-%i %i:%i:%i\n",
	    time.data.year, time.data.month, time.data.day,
	    time.data.hour, time.data.minute, time.data.second);
}


void
usage(void)
{
	fprintf(stderr, "usage: %s [-l] [-c chunk] [-f file]\n", getprogname());
	exit(EXIT_FAILURE);
}

