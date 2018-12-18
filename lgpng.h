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

#ifndef LGPNG_H__
#define LGPNG_H__

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

struct lgpng {
	struct IHDR	 *ihdr;
	struct PLTE	 *plte;

	struct tRNS	 *trns;
	struct cHRM	 *chrm;
	struct gAMA	 *gama;
	struct sBIT	 *sbit;
	struct sRGB	 *srgb;
	struct bKGD	 *bkgd;
	struct hIST	 *hist;
	struct pHYs	 *phys;
	size_t		  spltz;
	struct sPLT	**splt;
	struct tIME	 *time;
	size_t		  textz;
	struct tEXt	**text;
	size_t		  ztxtz;
	struct zTXt	**ztxt;
};

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
	CHUNK_TYPE_bKGD,
	CHUNK_TYPE_hIST,
	CHUNK_TYPE_pHYs,
	CHUNK_TYPE_sPLT,
	CHUNK_TYPE_tIME,
	CHUNK_TYPE__MAX,
};

/* IHDR chunk */
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

extern const char *colourtypemap[COLOUR_TYPE__MAX];

enum compressiontype {
	COMPRESSION_TYPE_DEFLATE,
	COMPRESSION_TYPE__MAX,
};

extern const char *compressiontypemap[COMPRESSION_TYPE__MAX];

enum filtermethod {
	FILTER_METHOD_ADAPTIVE,
	FILTER_METHOD__MAX,
};

extern const char *filtermethodmap[FILTER_METHOD__MAX];

enum interlace_method {
	INTERLACE_METHOD_STANDARD,
	INTERLACE_METHOD_ADAM7,
	INTERLACE_METHOD__MAX,
};

extern const char *interlacemap[INTERLACE_METHOD__MAX];

struct IHDR {
	uint32_t	width;
	uint32_t	height;
	int8_t		bitdepth;
	int8_t		colourtype;
	int8_t		compression;
	int8_t		filter;
	int8_t		interlace;
} __attribute__((packed));

/* PLTE chunk */
struct rgb8 {
	uint8_t	red;
	uint8_t	green;
	uint8_t	blue;
} __attribute__((packed));

struct PLTE {
	size_t		entries;
	struct rgb8	entry[256];
};

/* tRNS chunk */
struct tRNS {
	uint16_t	gray;
	uint16_t	red;
	uint16_t	green;
	uint16_t	blue;
	uint8_t		palette[256];
};

/* cHRM chunk */
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

/* gAMA chunk */
struct gAMA {
	uint32_t	gama;
};

/* sBIT type */
enum sBIT_type {
	sBIT_TYPE_0,
	sBIT_TYPE_23,
	sBIT_TYPE_4,
	sBIT_TYPE_6,
};

struct sBIT {
	enum sBIT_type	type;
	uint8_t	sgreyscale;
	uint8_t	sred;
	uint8_t	sgreen;
	uint8_t	sblue;
	uint8_t	salpha;
};

/* sRGB chunk */
enum rendering_intent {
	RENDERING_INTENT_PERCEPTUAL,
	RENDERING_INTENT_RELATIVE,
	RENDERING_INTENT_SATURATION,
	RENDERING_INTENT_ABSOLUTE,
	RENDERING_INTENT__MAX,
};

extern const char *rendering_intentmap[RENDERING_INTENT__MAX];

struct sRGB {
	uint8_t		intent;
};

/* tEXt chunk */
struct tEXt {
	char	*keyword;
	char	*text;
};

/* zTXt chunk */
struct zTXt {
	char	*keyword;
	uint8_t	compression;
	char	*text;
};

/* bKGD chunk */
struct rgb16 {
	uint16_t	red;
	uint16_t	green;
	uint16_t	blue;
};

struct bKGD {
	uint16_t	greyscale;
	uint8_t		paletteindex;
	struct rgb16	rgb;
};

/* hIST chunk */
struct hIST {
	uint16_t	frequency[256];
};

/* pHYs chunk */
enum unitspecifier {
	UNITSPECIFIER_UNKNOWN,
	UNITSPECIFIER_METRE,
	UNITSPECIFIER__MAX,
};

extern const char *unitspecifiermap[UNITSPECIFIER__MAX];

struct pHYs {
	uint32_t	ppux;
	uint32_t	ppuy;
	uint8_t		unitspecifier;
} __attribute__((packed));

/* sPLT chunk */
struct splt_entry {
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
};

struct sPLT {
	char			*palettename;
	uint8_t			 sampledepth;
	size_t			 entries;
	struct splt_entry	*entry;
};

/* tIME chunk */
struct tIME {
	uint16_t	year;
	uint8_t		month;
	uint8_t		day;
	uint8_t		hour;
	uint8_t		minute;
	uint8_t		second;
} __attribute__((packed));

bool		is_png(FILE *);
bool		is_chunk_public(const char *);

struct lgpng	*lgpng_new(void);
void		lgpng_free(struct lgpng *);

int32_t		read_next_chunk_len(FILE *);
enum chunktype	read_next_chunk_type(FILE *, char **);
int		read_next_chunk_data(FILE *, uint8_t **, int32_t);
uint32_t	read_next_chunk_crc(FILE *);

int		lgpng_parse_IHDR(struct lgpng *, uint8_t *, size_t);
int		lgpng_parse_PLTE(struct lgpng *, uint8_t *, size_t);
/* int		lgpng_parse_IDAT(struct lgpng *, uint8_t *, size_t); */
int		lgpng_parse_IEND(struct lgpng *, uint8_t *, size_t);
int		lgpng_parse_tRNS(struct lgpng *, uint8_t *, size_t);
int		lgpng_parse_cHRM(struct lgpng *, uint8_t *, size_t);
int		lgpng_parse_gAMA(struct lgpng *, uint8_t *, size_t);
/* int		lgpng_parse_iCCP(struct lgpng *, uint8_t *, size_t); */
int		lgpng_parse_sBIT(struct lgpng *, uint8_t *, size_t);
int		lgpng_parse_sRGB(struct lgpng *, uint8_t *, size_t);
int		lgpng_parse_tEXt(struct lgpng *, uint8_t *, size_t);
int		lgpng_parse_zTXt(struct lgpng *, uint8_t *, size_t);
/* int		lgpng_parse_iTXt(struct lgpng *, uint8_t *, size_t); */
int		lgpng_parse_bKGD(struct lgpng *, uint8_t *, size_t);
int		lgpng_parse_hIST(struct lgpng *, uint8_t *, size_t);
int		lgpng_parse_pHYs(struct lgpng *, uint8_t *, size_t);
int		lgpng_parse_sPLT(struct lgpng *, uint8_t *, size_t);
int		lgpng_parse_tIME(struct lgpng *, uint8_t *, size_t);

size_t		write_png_sig(uint8_t *);
size_t		write_IHDR(uint8_t *, size_t, int, enum colourtype);
size_t		write_IEND(uint8_t *);

void		usage(void);

#endif
