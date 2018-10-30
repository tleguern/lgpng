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

#include <arpa/inet.h>

#include <err.h>
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

const char *chunktypemap[CHUNK_TYPE__MAX] = {
	"IHDR",
	"PLTE",
	"IDAT",
	"IEND",
	"tRNS",
	"cHRM",
	"gAMA",
	"iCCP",
	"sBIT",
	"sRGB",
	"iTXt",
	"tEXt",
	"zTXt",
	"bKGD",
	"hIST",
	"pHYs",
	"sPLT",
	"tIME",
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

bool		is_png(FILE *);
int32_t		read_next_chunk_len(FILE *);
enum chunktype	read_next_chunk_type(FILE *);
int		read_next_chunk_data(FILE *, uint8_t **, int32_t);
uint32_t	read_next_chunk_crc(FILE *);
void		parse_IHDR(uint8_t *, size_t);
void		usage(void);

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
	while (-1 != (ch = getopt(argc, argv, "c:lf:")))
		switch (ch) {
		case 'c':
			for (int i = 0; i < CHUNK_TYPE__MAX; i++) {
				if (strcmp(optarg, chunktypemap[i]) == 0) {
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
			printf("%s\n", chunktypemap[type]);
		} else if (cflag == type) {
			switch (type) {
			case CHUNK_TYPE_IHDR:
				parse_IHDR(chunkdata, chunkz);
				break;
			case CHUNK_TYPE__MAX:
				/* FALLTHROUGH */
			default:
				break;
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
		if (memcmp(chunk, chunktypemap[i], sizeof(chunk)) == 0)
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
usage(void)
{
	fprintf(stderr, "usage: %s [-l] [-c chunk] [-f file]\n", getprogname());
	exit(EXIT_FAILURE);
}

