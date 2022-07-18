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

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#ifndef LGPNG_H__
#define LGPNG_H__

/* Global variables */
extern char	png_sig[8];

/* Generic chunk */
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

extern const char *chunktypemap[CHUNK_TYPE__MAX];

struct chunk {
	uint32_t         length;
	enum chunktype   type;
	uint32_t         crc;
	uint8_t          data[];
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
	uint32_t         length;
	enum chunktype   type;
	uint32_t         crc;
	struct {
		uint32_t	width;
		uint32_t	height;
		int8_t		bitdepth;
		int8_t		colourtype;
		int8_t		compression;
		int8_t		filter;
		int8_t		interlace;
	} __attribute__((packed)) data;
};

/* PLTE chunk */
struct rgb8 {
	uint8_t	red;
	uint8_t	green;
	uint8_t	blue;
} __attribute__((packed));

struct PLTE {
	uint32_t         length;
	enum chunktype   type;
	uint32_t         crc;
	struct {
		size_t		entries;
		struct rgb8	entry[256];
	} __attribute__((packed)) data;
};

/* tRNS chunk */
struct tRNS {
	uint32_t         length;
	enum chunktype   type;
	uint32_t         crc;
	struct {
		uint16_t	gray;
		uint16_t	red;
		uint16_t	green;
		uint16_t	blue;
		size_t		entries;
		uint8_t		palette[256];
	} __attribute__((packed)) data;
};

/* cHRM chunk */
struct cHRM {
	uint32_t         length;
	enum chunktype   type;
	uint32_t         crc;
	struct {
		uint32_t	whitex;
		uint32_t	whitey;
		uint32_t	redx;
		uint32_t	redy;
		uint32_t	greenx;
		uint32_t	greeny;
		uint32_t	bluex;
		uint32_t	bluey;
	} __attribute__((packed)) data;
};

/* gAMA chunk */
struct gAMA {
	uint32_t         length;
	enum chunktype   type;
	uint32_t         crc;
	struct {
		uint32_t	gamma;
	} __attribute__((packed)) data;
};

/* iCCP chunk */
struct iCCP {
	uint32_t         length;
	enum chunktype   type;
	uint32_t         crc;
	struct {
		size_t	 namez;
		uint8_t	 name[80];
		int8_t	 compression;
		uint8_t	*profile;
	} __attribute__((packed)) data;
};

/* sBIT chunk */
struct sBIT {
	uint32_t         length;
	enum chunktype   type;
	uint32_t         crc;
	struct {
		uint8_t	sgreyscale;
		uint8_t	sred;
		uint8_t	sgreen;
		uint8_t	sblue;
		uint8_t	salpha;
	} __attribute__((packed)) data;
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
	uint32_t         length;
	enum chunktype   type;
	uint32_t         crc;
	struct {
		uint8_t		intent;
	} __attribute__((packed)) data;
};

/* tEXt chunk */
struct tEXt {
	uint32_t         length;
	enum chunktype   type;
	uint32_t         crc;
	struct {
		char	*keyword;
		char	*text;
	} __attribute__((packed)) data;
};

/* zTXt chunk */
struct zTXt {
	uint32_t         length;
	enum chunktype   type;
	uint32_t         crc;
	struct {
		char		*keyword;
		uint8_t		 compression;
		unsigned char	*text;
	} __attribute__((packed)) data;
};

/* bKGD chunk */
struct rgb16 {
	uint16_t	red;
	uint16_t	green;
	uint16_t	blue;
};

struct bKGD {
	uint32_t         length;
	enum chunktype   type;
	uint32_t         crc;
	struct {
		uint16_t	greyscale;
		uint8_t		paletteindex;
		struct rgb16	rgb;
	} __attribute__((packed)) data;
};

/* hIST chunk */
struct hIST {
	uint32_t         length;
	enum chunktype   type;
	uint32_t         crc;
	struct {
		uint16_t	frequency[256];
	} __attribute__((packed)) data;
};

/* pHYs chunk */
enum unitspecifier {
	UNITSPECIFIER_UNKNOWN,
	UNITSPECIFIER_METRE,
	UNITSPECIFIER__MAX,
};

extern const char *unitspecifiermap[UNITSPECIFIER__MAX];

struct pHYs {
	uint32_t         length;
	enum chunktype   type;
	uint32_t         crc;
	struct {
		uint32_t	ppux;
		uint32_t	ppuy;
		uint8_t		unitspecifier;
	} __attribute__((packed)) data;
};

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
	uint32_t         length;
	enum chunktype   type;
	uint32_t         crc;
	struct {
		char			*palettename;
		uint8_t			 sampledepth;
		size_t			 entries;
		struct splt_entry	*entry;
	} __attribute__((packed)) data;
};

/* tIME chunk */
struct tIME {
	uint32_t         length;
	enum chunktype   type;
	uint32_t         crc;
	struct {
		uint16_t	year;
		uint8_t		month;
		uint8_t		day;
		uint8_t		hour;
		uint8_t		minute;
		uint8_t		second;
	} __attribute__((packed)) data;
};

/* chunks extra */
bool		lgpng_validate_keyword(uint8_t *, size_t);

/* chunks */
int		lgpng_create_IHDR_from_data(struct IHDR *, uint8_t *, size_t);
int		lgpng_create_PLTE_from_data(struct PLTE *, uint8_t *, size_t);
/*int		lgpng_create_IDAT_from_data(struct IDAT *, uint8_t *, size_t);*/
int		lgpng_create_tRNS_from_data(struct tRNS *, struct IHDR *, uint8_t *, size_t);
int		lgpng_create_cHRM_from_data(struct cHRM *, uint8_t *, size_t);
int		lgpng_create_gAMA_from_data(struct gAMA *, uint8_t *, size_t);
int		lgpng_create_iCCP_from_data(struct iCCP *, uint8_t *, size_t);
int		lgpng_create_sBIT_from_data(struct sBIT *, struct IHDR *, uint8_t *, size_t);
int		lgpng_create_sRGB_from_data(struct sRGB *, uint8_t *, size_t);
/*int		lgpng_create_iTXt_from_data(struct iTXt *, uint8_t *, size_t);*/
int		lgpng_create_tEXt_from_data(struct tEXt *, uint8_t *, size_t);
int		lgpng_create_zTXt_from_data(struct zTXt *, uint8_t *, size_t);
int		lgpng_create_bKGD_from_data(struct bKGD *, struct IHDR *, struct PLTE *, uint8_t *, size_t);
int		lgpng_create_hIST_from_data(struct hIST *, struct PLTE *, uint8_t *, size_t);
int		lgpng_create_pHYs_from_data(struct pHYs *, uint8_t *, size_t);
int		lgpng_create_sPLT_from_data(struct sPLT *, uint8_t *, size_t);
int		lgpng_create_tIME_from_data(struct tIME *, uint8_t *, size_t);

/* data */
bool	lgpng_data_is_png(uint8_t *, size_t);
bool	lgpng_data_get_length(uint8_t *, size_t, uint32_t *);
bool	lgpng_data_get_type(uint8_t *, size_t, int *, uint8_t *);
bool	lgpng_data_get_data(uint8_t *, size_t, uint32_t, uint8_t **);
bool	lgpng_data_get_crc(uint8_t *, size_t, uint32_t *);

/* stream */
bool	lgpng_stream_is_png(FILE *);
bool	lgpng_stream_get_length(FILE *, uint32_t *);
bool	lgpng_stream_get_type(FILE *, int *, uint8_t *);
bool	lgpng_stream_get_data(FILE *, uint32_t, uint8_t **);
bool	lgpng_stream_skip_data(FILE *, uint32_t);
bool	lgpng_stream_get_crc(FILE *, uint32_t *);
bool	lgpng_stream_write_chunk(FILE *, uint32_t, uint8_t *, uint8_t *, uint32_t);

/* crc */
extern uint32_t	lgpng_crc_table[256];
uint32_t	lgpng_crc_init(void);
uint32_t	lgpng_crc_update(uint32_t, uint8_t *, size_t);
uint32_t	lgpng_crc_finalize(uint32_t);
uint32_t	lgpng_crc(uint8_t *, size_t);
bool		lgpng_chunk_crc(uint32_t, uint8_t [4], uint8_t *, uint32_t *);

/* helper macro */
#define MSB16(i) (i & 0xFF00)
#define LSB16(i) (i & 0x00FF)

#endif
