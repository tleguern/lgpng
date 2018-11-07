/*
 * Copyright (c) 2018 Tristan Le Guern <tleguern@bouledef.eu>
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

#if HAVE_ERR
# include <err.h>
#endif
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

enum chunktype {
	CHUNK_TYPE_IHDR,
	CHUNK_TYPE_PLTE,
	CHUNK_TYPE_IDAT,
	CHUNK_TYPE_IEND,
	CHUNK_TYPE_tRNS,
	CHUNK_TYPE_cHRM,
	CHUNK_TYPE_gAMA,
	CHUNK_TYPE_iCCP,
	CHUNK_TYPE_sBIT,
	CHUNK_TYPE_sRGB,
	CHUNK_TYPE_iTXt,
	CHUNK_TYPE_tEXt,
	CHUNK_TYPE_zTXt,
	CHUNK_TYPE_bKGB,
	CHUNK_TYPE_hIST,
	CHUNK_TYPE_pHYs,
	CHUNK_TYPE_sPLT,
	CHUNK_TYPE_tIME,
	CHUNK_TYPE__MAX,
};

enum colourtype {
	COLOUR_TYPE_GREYSCALE,
	COLOUR_TYPE_FILLER1,
	COLOUR_TYPE_TRUECOLOUR,
	COLOUR_TYPE_INDEXED,
	COLOUR_TYPE_GREYSCALE_ALPHA,
	COLOUR_TYPE_FILLER5,
	COLOUR_TYPE_TRUECOLOUR_ALPHA,
	COLOUR_TYPE__MAX,
};

const char *colourtypemap[COLOUR_TYPE__MAX] = {
	"greyscale",
	"error",
	"truecolour",
	"indexed",
	"greyscale + alpha",
	"error",
	"truecolour + alpha",
};

enum compressiontype {
	COMPRESSION_TYPE_DEFLATE,
	COMPRESSION_TYPE__MAX,
};

const char *compressiontypemap[COMPRESSION_TYPE__MAX] = {
	"deflate",
};

enum filtertype {
	FILTER_TYPE_ADAPTIVE,
	FILTER_TYPE__MAX,
};

const char *filtertypemap[FILTER_TYPE__MAX] = {
	"adaptive",
};

enum interlace_method {
	INTERLACE_METHOD_STANDARD,
	INTERLACE_METHOD_ADAM7,
	INTERLACE_METHOD__MAX,
};

const char *interlacemap[INTERLACE_METHOD__MAX] = {
	"standard",
	"adam7",
};

struct IHDR {
	uint32_t	width;
	uint32_t	height;
	int8_t		bitdepth;
	int8_t		colourtype;
	int8_t		compression;
	int8_t		filter;
	int8_t		interlace;
} __attribute__((packed));

struct PLTE {
	struct {
		uint8_t		red;
		uint8_t		green;
		uint8_t		blue;
	} entries[256];
};

struct cHRM {
	uint32_t	whitex;
	uint32_t	whitey;
	uint32_t	redx;
	uint32_t	redy;
	uint32_t	greenx;
	uint32_t	greeny;
	uint32_t	bluex;
	uint32_t	bluey;
};

struct gAMA {
	uint32_t	gama;
};

enum rendering_intent {
	RENDERING_INTENT_PERCEPTUAL,
	RENDERING_INTENT_RELATIVE,
	RENDERING_INTENT_SATURATION,
	RENDERING_INTENT_ABSOLUTE,
	RENDERING_INTENT__MAX,
};

const char *rendering_intentmap[RENDERING_INTENT__MAX] = {
	"perceptual",
	"relative colorimetric",
	"saturation",
	"absolute colorimetric",
};

struct sRGB {
	uint8_t		intent;
};

struct hIST {
	struct {
		uint16_t	frequency;
	} entries[256];
};

enum unitspecifier {
	UNITSPECIFIER_UNKNOWN,
	UNITSPECIFIER_METRE,
	UNITSPECIFIER__MAX,
};

const char *unitspecifiermap[UNITSPECIFIER__MAX] = {
	"unknown",
	"metre",
};

struct pHYs {
	uint32_t	ppux;
	uint32_t	ppuy;
	uint8_t		unitspecifier;
} __attribute__((packed));

struct tIME {
	uint16_t	year;
	uint8_t		month;
	uint8_t		day;
	uint8_t		hour;
	uint8_t		minute;
	uint8_t		second;
} __attribute__((packed));

struct tEXt {
	char	*keyword;
	char	*text;
};

struct sPLT {
	size_t	 entriesz;
	char	*palettename;
	uint8_t	 sampledepth;
	struct {
		union {
			uint8_t raw[10];
			struct {
				uint16_t red;
				uint16_t green;
				uint16_t blue;
				uint16_t alpha;
				uint16_t frequency;
			} depth16;
			struct {
				uint8_t red;
				uint8_t green;
				uint8_t blue;
				uint8_t alpha;
				uint16_t frequency;
			} depth8;
		};
	} **entries;
};

bool		is_png(FILE *);
int32_t		read_next_chunk_len(FILE *);
enum chunktype	read_next_chunk_type(FILE *);
int		read_next_chunk_data(FILE *, uint8_t **, int32_t);
uint32_t	read_next_chunk_crc(FILE *);
void		parse_IHDR(uint8_t *, size_t);
void		parse_PLTE(uint8_t *, size_t);
void		parse_cHRM(uint8_t *, size_t);
void		parse_gAMA(uint8_t *, size_t);
void		parse_sRGB(uint8_t *, size_t);
void		parse_hIST(uint8_t *, size_t);
void		parse_pHYs(uint8_t *, size_t);
void		parse_IEND(uint8_t *, size_t);
void		parse_tIME(uint8_t *, size_t);
void		parse_tEXt(uint8_t *, size_t);
void		parse_sPLT(uint8_t *, size_t);
void		usage(void);

struct chunktypemap {
	const char *const name;
	void (*fn)(uint8_t *, size_t);
} chunktypemap[CHUNK_TYPE__MAX] = {
	{ "IHDR", parse_IHDR },
	{ "PLTE", parse_PLTE },
	{ "IDAT", NULL },
	{ "IEND", parse_IEND },
	{ "tRNS", NULL },
	{ "cHRM", parse_cHRM },
	{ "gAMA", parse_gAMA },
	{ "iCCP", NULL },
	{ "sBIT", NULL },
	{ "sRGB", parse_sRGB },
	{ "iTXt", NULL },
	{ "tEXt", parse_tEXt },
	{ "zTXt", NULL },
	{ "bKGD", NULL },
	{ "hIST", parse_hIST },
	{ "pHYs", parse_pHYs },
	{ "sPLT", parse_sPLT },
	{ "tIME", parse_tIME },
};

int
main(int argc, char *argv[])
{
	int		 ch;
	bool		 lflag;
	enum chunktype	 cflag;
	FILE		*fflag;

	lflag = true;
	fflag = stdin;
	cflag = CHUNK_TYPE__MAX;
#ifdef HAVE_PLEDGE
	pledge("stdio rpath", NULL);
#endif
	while (-1 != (ch = getopt(argc, argv, "c:lf:")))
		switch (ch) {
		case 'c':
			for (int i = 0; i < CHUNK_TYPE__MAX; i++) {
				if (strcmp(optarg, chunktypemap[i].name) == 0) {
					cflag = i;
					break;
				}
			}
			if (CHUNK_TYPE__MAX == cflag)
				errx(EXIT_FAILURE, "%s: invalid chunk", optarg);
			lflag = false;
			break;
		case 'l':
			lflag = true;
			break;
		case 'f':
			if (NULL == (fflag = fopen(optarg, "r"))) {
				err(EXIT_FAILURE, "%s", optarg);
			}
			break;
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (! is_png(fflag)) {
		errx(EXIT_FAILURE, "not a PNG file");
	}
	do {
		enum chunktype	 type;
		int32_t		 chunkz;
		uint32_t	 chunkcrc;
		uint8_t		*chunkdata;

		chunkdata = NULL;
		if (-1 == (chunkz = read_next_chunk_len(fflag)))
			errx(EXIT_FAILURE, "Invalid chunk len");
		if (CHUNK_TYPE__MAX == (type = read_next_chunk_type(fflag)))
			errx(EXIT_FAILURE, "Invalid chunk type");
		if (-1 == read_next_chunk_data(fflag, &chunkdata, chunkz))
			errx(EXIT_FAILURE, "Invalid chunk data");
		if (0 == (chunkcrc = read_next_chunk_crc(fflag)))
			errx(EXIT_FAILURE, "Invalid chunk crc");
		if (lflag) {
			printf("%s\n", chunktypemap[type].name);
		} else if (cflag == type) {
			if (NULL != chunktypemap[type].fn) {
				chunktypemap[type].fn(chunkdata, chunkz);
			}
		}
		free(chunkdata);
		if (type == CHUNK_TYPE_IEND) {
			break;
		}
	} while(1);
	fclose(fflag);
	return(EXIT_SUCCESS);
}

int
read_next_chunk_data(FILE *f, uint8_t **data, int32_t chunkz)
{
	size_t	 bufz;
	uint8_t	*buf;

	bufz = chunkz + 1;
	if (NULL == (buf = malloc(bufz)))
		return(-1);
	if (chunkz != (int32_t)fread(buf, 1, chunkz, f)) {
		free(buf);
		return(-1);
	}
	buf[chunkz] = '\0';
	(*data) = buf;
	return(0);
}

uint32_t
read_next_chunk_crc(FILE *f)
{
	uint32_t crc = 0;

	if (1 != fread(&crc, 4, 1, f))
		return(0);
	crc = ntohl(crc);
	return(crc);
}

enum chunktype
read_next_chunk_type(FILE *f)
{
	size_t	i;
	char	chunk[4] = {0, 0, 0, 0};

	if (sizeof(chunk) != fread(chunk, 1, sizeof(chunk), f)) {
		warnx("Can't read a full chunk type");
		return(CHUNK_TYPE__MAX);
	}
	for (i = 0; i < sizeof(chunk); i++) {
		if (isalpha(chunk[i]) == 0)
			return(CHUNK_TYPE__MAX);
	}
	for (i = 0; i < CHUNK_TYPE__MAX; i++) {
		if (memcmp(chunk, chunktypemap[i].name, sizeof(chunk)) == 0)
			return(i);
	}
	return(CHUNK_TYPE__MAX);
}

int32_t
read_next_chunk_len(FILE *f)
{
	uint32_t len = 0;

	if (1 != fread(&len, 4, 1, f))
		return(-1);
	len = ntohl(len);
	if (len > INT32_MAX)
		return(-1);
	return((int32_t)len);
}

bool
is_png(FILE *f)
{
	char sig[8] = {  0,  0,  0,  0,  0,  0,  0,  0};
	char png[8] = {137, 80, 78, 71, 13, 10, 26, 10};

	fread(sig, 1, sizeof(sig), f);
	if (memcmp(sig, png, sizeof(sig)) == 0)
		return(true);
	return(false);
}

void
parse_IHDR(uint8_t *data, size_t dataz)
{
	struct IHDR	*ihdr;
	(void)dataz;

	if (sizeof(struct IHDR) != dataz) {
		errx(EXIT_FAILURE, "IHDR: invalid chunk size");
	}
	ihdr = (struct IHDR *)data;
	ihdr->width = ntohl(ihdr->width);
	ihdr->height = ntohl(ihdr->height);

	if (0 == ihdr->width) {
		errx(EXIT_FAILURE, "IHDR: Invalid width 0\n");
	}
	if (0 == ihdr->height) {
		errx(EXIT_FAILURE, "IHDR: Invalid height 0\n");
	}
	switch (ihdr->colourtype) {
	case COLOUR_TYPE_GREYSCALE:
		if (1 != ihdr->bitdepth && 2 != ihdr->bitdepth
		    && 4 != ihdr->bitdepth && 8 != ihdr->bitdepth
		    && 16 != ihdr->bitdepth) {
			errx(EXIT_FAILURE,
			    "IHDR: Invalid bit depth %i, should be 1, 2, 4, 8 or 16",
			    ihdr->bitdepth);
		}
		break;
	case COLOUR_TYPE_TRUECOLOUR_ALPHA:
		/* FALLTHROUGH */
	case COLOUR_TYPE_GREYSCALE_ALPHA:
		/* FALLTHROUGH */
	case COLOUR_TYPE_TRUECOLOUR:
		if ( 8 != ihdr->bitdepth && 16 != ihdr->bitdepth) {
			errx(EXIT_FAILURE,
			    "IHDR: Invalid bit depth %i, should be 8 or 16",
			    ihdr->bitdepth);
		}
		break;
	case COLOUR_TYPE_INDEXED:
		if (1 != ihdr->bitdepth && 2 != ihdr->bitdepth
		    && 4 != ihdr->bitdepth && 8 != ihdr->bitdepth) {
			errx(EXIT_FAILURE,
			    "IHDR: Invalid bit depth %i, should be 1, 2, 4 or 8",
			    ihdr->bitdepth);
		}
		break;
	case COLOUR_TYPE_FILLER1:
	case COLOUR_TYPE_FILLER5:
	case COLOUR_TYPE__MAX:
	default:
		errx(EXIT_FAILURE, "IHDR: Invalid colour type %i\n",
		    ihdr->colourtype);
	}
	if (COMPRESSION_TYPE_DEFLATE != ihdr->compression) {
		errx(EXIT_FAILURE, "IHDR: Invalid compression type %i\n",
		    ihdr->compression);
	}
	if (FILTER_TYPE_ADAPTIVE != ihdr->filter) {
		errx(EXIT_FAILURE, "IHDR: Invalid filter type %i\n",
		    ihdr->filter);
	}
	if (INTERLACE_METHOD_STANDARD != ihdr->interlace
	    && INTERLACE_METHOD_ADAM7 != ihdr->interlace) {
		errx(EXIT_FAILURE, "IHDR: Invalid interlace method %i\n",
		    ihdr->interlace);
	}
	printf("IHDR: width: %u\n", ihdr->width);
	printf("IHDR: height: %u\n", ihdr->height);
	printf("IHDR: bitdepth: %i\n", ihdr->bitdepth);
	printf("IHDR: colourtype: %s\n", colourtypemap[ihdr->colourtype]);
	printf("IHDR: compression: %s\n",
	    compressiontypemap[ihdr->compression]);
	printf("IHDR: filter: %s\n", filtertypemap[ihdr->filter]);
	printf("IHDR: interlace method: %s\n", interlacemap[ihdr->interlace]);
}

void
parse_PLTE(uint8_t *data, size_t dataz)
{
	size_t		 elemz;
	struct PLTE	*plte;

	/*
	 * TODO:
	 * - colour type should only be 2, 3 or 6 ;
	 * - check if struct png doesn't already contain a PLTE ;
	 * - elemz should not be greater than pow(2, bitdepth).
	 */
	elemz = dataz / 3;
	if (0 != dataz % 3 || 256 < elemz) {
		errx(EXIT_FAILURE, "PLTE: Invalid chunk size");
	}
	plte = (struct PLTE *)data;
	printf("PLTE: %zu entries\n", elemz);
	for (size_t i = 0; i < elemz; i++) {
		printf("PLTE: entry %zu: 0x%x%x%x\n", i,
		    plte->entries[i].red,
		    plte->entries[i].green,
		    plte->entries[i].blue);
	}
}

void
parse_cHRM(uint8_t *data, size_t dataz)
{
	struct cHRM	*chrm;
	double whitex, whitey, redx, redy, greenx, greeny, bluex, bluey;

	if (sizeof(struct cHRM) != dataz) {
		errx(EXIT_FAILURE, "cHRM: invalid chunk size");
	}
	chrm = (struct cHRM *)data;
	chrm->whitex = ntohl(chrm->whitex);
	chrm->whitey = ntohl(chrm->whitey);
	chrm->redx = ntohl(chrm->redx);
	chrm->redy = ntohl(chrm->redy);
	chrm->greenx = ntohl(chrm->greenx);
	chrm->greeny = ntohl(chrm->greeny);
	chrm->bluex = ntohl(chrm->bluex);
	chrm->bluey = ntohl(chrm->bluey);
	whitex = (double)chrm->whitex / 100000.0;
	whitey = (double)chrm->whitey / 100000.0;
	redx = (double)chrm->redx / 100000.0;
	redy = (double)chrm->redy / 100000.0;
	greenx = (double)chrm->greenx / 100000.0;
	greeny = (double)chrm->greeny / 100000.0;
	bluex = (double)chrm->bluex / 100000.0;
	bluey = (double)chrm->bluey / 100000.0;
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
parse_gAMA(uint8_t *data, size_t dataz)
{
	struct gAMA	*gama;
	double		 imagegama;

	if (sizeof(struct gAMA) != dataz) {
		errx(EXIT_FAILURE, "gAMA: invalid chunk size");
	}
	gama = (struct gAMA *)data;
	gama->gama = ntohl(gama->gama);
	imagegama = (double)gama->gama / 100000.0;
	if (0 == imagegama) {
		errx(EXIT_FAILURE, "gAMA: invalid value of 0");
	}
	printf("gAMA: image gama: %f\n", imagegama);
}

void
parse_sRGB(uint8_t *data, size_t dataz)
{
	struct sRGB	*srgb;

	if (sizeof(struct sRGB) != dataz) {
		errx(EXIT_FAILURE, "sRGB: invalid chunk size");
	}
	srgb = (struct sRGB *)data;
	if (srgb->intent >= RENDERING_INTENT__MAX) {
		errx(EXIT_FAILURE, "sRGB: invalid rendering intent value");
	}
	printf("sRGB: rendering intent: %s\n",
	    rendering_intentmap[srgb->intent]);
}

void
parse_hIST(uint8_t *data, size_t dataz)
{
	size_t		 elemz;
	struct hIST	*hist;

	/*
	 * TODO:
	 * - probably the same restrictions as in PLTE
	 * - can only appear if PLTE is defined
	 * - should be of the same size a PLTE
	 */
	elemz = dataz / 3;
	if (0 != dataz % 3 || 256 < elemz) {
		errx(EXIT_FAILURE, "hIST: Invalid chunk size");
	}
	hist = (struct hIST *)data;
	printf("hIST: %zu entries\n", elemz);
	for (size_t i = 0; i < elemz; i++) {
		printf("hIST: entry %zu: %i\n", i,
		    hist->entries[i].frequency);
	}
}

void
parse_pHYs(uint8_t *data, size_t dataz)
{
	struct pHYs	*phys;

	if (sizeof(struct pHYs) != dataz) {
		errx(EXIT_FAILURE, "pHYs: invalid chunk size");
	}
	phys = (struct pHYs *)data;
	phys->ppux = ntohl(phys->ppux);
	phys->ppuy = ntohl(phys->ppuy);
	if (phys->unitspecifier >= UNITSPECIFIER__MAX ) {
		errx(EXIT_FAILURE, "pHYs: invalid unit specifier");
	}
	printf("pHYs: pixel per unit, X axis: %i\n", phys->ppux);
	printf("pHYs: pixel per unit, Y axis: %i\n", phys->ppuy);
	printf("pHYs: unit specifier: %s\n",
	    unitspecifiermap[phys->unitspecifier]);
}

void
parse_IEND(uint8_t *data, size_t dataz)
{
	(void)data;
	if (0 != dataz) {
		errx(EXIT_FAILURE, "IEND: invalid chunk size");
	}

}

void
parse_tIME(uint8_t *data, size_t dataz)
{
	struct tIME	*time;

	if (sizeof(struct tIME) != dataz) {
		errx(EXIT_FAILURE, "tIME: invalid chunk size");
	}
	time = (struct tIME *)data;
	time->year = htons(time->year);
	if (time->month <= 0 || time->month > 12) {
		errx(EXIT_FAILURE, "tIME: invalid month value");
	}
	if (time->day <= 0 || time->day > 31) {
		errx(EXIT_FAILURE, "tIME: invalid day value");
	}
	if (time->hour < 0 || time->hour > 23) {
		errx(EXIT_FAILURE, "tIME: invalid hour value");
	}
	if (time->minute < 0 || time->minute > 59) {
		errx(EXIT_FAILURE, "tIME: invalid minute value");
	}
	if (time->second < 0 || time->second > 60) {
		errx(EXIT_FAILURE, "tIME: invalid second value");
	}
	printf("tIME: %i-%i-%i %i:%i:%i\n",
	    time->year, time->month, time->day,
	    time->hour, time->minute, time->second);
}

void
parse_tEXt(uint8_t *data, size_t dataz)
{
	struct tEXt	text;

	(void)dataz;
	text.keyword = data;
	if (strlen(text.keyword) >= 80) {
		errx(EXIT_FAILURE, "tEXt: Invalid keyword size");
	}
	text.text = data + strlen(data) + 1;
	if ('\n' == text.text[strlen(text.text) - 1]) {
		text.text[strlen(text.text) - 1] = '\0';
	}
	printf("%s: %s\n", text.keyword, text.text);
}

void
parse_sPLT(uint8_t *data, size_t dataz)
{
	struct sPLT	splt;
	size_t		palettenamez;
	int		offset;

	splt.palettename = data;
	palettenamez = strlen(splt.palettename);
	if (palettenamez >= 80) {
		errx(EXIT_FAILURE, "sPLT: Invalid palette name size");
	}
	offset = palettenamez + 1;
	splt.sampledepth = data[offset];
	if (8 != splt.sampledepth && 16 != splt.sampledepth) {
		errx(EXIT_FAILURE, "sPLT: Invalid sample depth");
	}
	offset += 1;
	splt.entriesz = (dataz - offset) / splt.sampledepth;
	splt.entries = calloc(splt.entriesz, sizeof(*(splt.entries)));
	for (size_t i = 0; i < splt.entriesz; i++) {
		splt.entries[i] = calloc(1, sizeof(**(splt.entries)));
		if (8 == splt.sampledepth) {
			memcpy(splt.entries[i], data + offset, 6);
			offset += 6;
			splt.entries[i]->depth8.frequency =
			    ntohs(splt.entries[i]->depth8.frequency);
		} else {
			memcpy(splt.entries[i], data + offset, 10);
			offset += 10;
			splt.entries[i]->depth16.red =
			    ntohs(splt.entries[i]->depth16.red);
			splt.entries[i]->depth16.green =
			    ntohs(splt.entries[i]->depth16.green);
			splt.entries[i]->depth16.blue =
			    ntohs(splt.entries[i]->depth16.blue);
			splt.entries[i]->depth16.alpha =
			    ntohs(splt.entries[i]->depth16.alpha);
			splt.entries[i]->depth16.frequency =
			    ntohs(splt.entries[i]->depth16.frequency);
		}
	}
	printf("sPLT: palette name: %s\n", splt.palettename);
	printf("sPLT: sample depth: %i\n", splt.sampledepth);
	printf("sPLT: %zu entries\n", splt.entriesz);
	for (size_t i = 0; i < splt.entriesz; i++) {
		if (8 == splt.sampledepth) {
			printf("sPLT: entry %3zu: red: 0x%02X, green 0x%02X, "
			  "blue: 0x%02X, alpha: 0x%02X, frequency: %hi\n", i,
			     splt.entries[i]->depth8.red,
			     splt.entries[i]->depth8.green,
			     splt.entries[i]->depth8.blue,
			     splt.entries[i]->depth8.alpha,
			     splt.entries[i]->depth8.frequency);
		} else {
			printf("sPLT: entry %3zu: red: 0x%04X, green 0x%04X, "
			  "blue: 0x%04X, alpha: 0x%04X, frequency: %hi\n", i,
			     splt.entries[i]->depth16.red,
			     splt.entries[i]->depth16.green,
			     splt.entries[i]->depth16.blue,
			     splt.entries[i]->depth16.alpha,
			     splt.entries[i]->depth16.frequency);
		}
		free(splt.entries[i]);
		splt.entries[i] = NULL;
	}
	free(splt.entries);
	splt.entries = NULL;
}

void
usage(void)
{
	fprintf(stderr, "usage: %s [-l] [-c chunk] [-f file]\n", getprogname());
	exit(EXIT_FAILURE);
}

