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
	CHUNK_TYPE_cICP,
	CHUNK_TYPE_iTXt,
	CHUNK_TYPE_tEXt,
	CHUNK_TYPE_zTXt,
	CHUNK_TYPE_bKGD,
	CHUNK_TYPE_hIST,
	CHUNK_TYPE_pHYs,
	CHUNK_TYPE_sPLT,
	CHUNK_TYPE_eXIf,
	CHUNK_TYPE_tIME,
	CHUNK_TYPE_acTL,
	CHUNK_TYPE_fcTL,
	CHUNK_TYPE_fdAT,
	CHUNK_TYPE_oFFs,
	CHUNK_TYPE_gIFg,
	CHUNK_TYPE_gIFx,
	CHUNK_TYPE_vpAg,
	CHUNK_TYPE_caNv,
	CHUNK_TYPE_orNt,
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

/* IDAT chunk */
struct IDAT {
	uint32_t         length;
	enum chunktype   type;
	uint32_t         crc;
	struct {
		uint8_t	*data;
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
		size_t	 profilez;
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

/* cICP chunk */
struct cICP {
	uint32_t         length;
	enum chunktype   type;
	uint32_t         crc;
	struct {
		uint8_t	colour_primaries;
		uint8_t	transfer_function;
		uint8_t	matrix_coefficients;
		uint8_t	video_full_range;
	} __attribute__((packed)) data;
};

/* tEXt chunk */
struct tEXt {
	uint32_t         length;
	enum chunktype   type;
	uint32_t         crc;
	struct {
		uint8_t	 keyword[80];
		uint8_t	*text;
	} __attribute__((packed)) data;
};

/* zTXt chunk */
struct zTXt {
	uint32_t         length;
	enum chunktype   type;
	uint32_t         crc;
	struct {
		size_t		 keywordz;
		uint8_t		 keyword[80];
		uint8_t		 compression;
		size_t		 textz;
		uint8_t		*text;
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
		char			 palettename[80];
		uint8_t			 sampledepth;
		size_t			 entries;
		struct splt_entry	*entry;
	} __attribute__((packed)) data;
};

/* eXIf chunk */
struct eXIf {
	uint32_t         length;
	enum chunktype   type;
	uint32_t         crc;
	struct {
		uint8_t	*profile;
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

/* acTL chunk */
struct acTL {
	uint32_t         length;
	enum chunktype   type;
	uint32_t         crc;
	struct {
		uint32_t	num_frames;
		uint32_t	num_plays;
	} __attribute__((packed)) data;
};

/* fcTL chunk */
enum dispose_op {
	DISPOSE_OP_NONE,
	DISPOSE_OP_BACKGROUND,
	DISPOSE_OP_PREVIOUS,
	DISPOSE_OP__MAX,
};

extern const char *dispose_opmap[DISPOSE_OP__MAX];

enum blend_op {
	BLEND_OP_SOURCE,
	BLEND_OP_OVER,
	BLEND_OP__MAX,
};

extern const char *blend_opmap[BLEND_OP__MAX];

struct fcTL {
	uint32_t         length;
	enum chunktype   type;
	uint32_t         crc;
	struct {
		uint32_t	sequence_number;
		uint32_t	width;
		uint32_t	height;
		uint32_t	x_offset;
		uint32_t	y_offset;
		uint16_t	delay_num;
		uint16_t	delay_den;
		uint8_t		dispose_op;
		uint8_t		blend_op;

	} __attribute__((packed)) data;
};

/* fdAT chunk */
struct fdAT {
	uint32_t         length;
	enum chunktype   type;
	uint32_t         crc;
	struct {
		uint32_t	 sequence_number;
		uint8_t		*frame_data;
	} __attribute__((packed)) data;
};

/* oFFs chunk */
enum offs_unitspecifier {
	OFFS_UNITSPECIFIER_PIXEL,
	OFFS_UNITSPECIFIER_MICROMETER,
	OFFS_UNITSPECIFIER__MAX,
};

extern const char *offsunitspecifiermap[OFFS_UNITSPECIFIER__MAX];

struct oFFs {
	uint32_t         length;
	enum chunktype   type;
	uint32_t         crc;
	struct {
		int32_t	 x_position;
		int32_t	 y_position;
		uint8_t	 unitspecifier; /* enum offs_unitspecifier */
	} __attribute__((packed)) data;
};

/* gIFg chunk */
/* Values found un gif89a section 23 Graphic Control Extension */
enum disposal_method {
	DISPOSAL_METHOD_NONE,
	DISPOSAL_METHOD_DO_NOT,
	DISPOSAL_METHOD_RESTORE_BACKGROUND,
	DISPOSAL_METHOD_RESTORE_PREVIOUS,
	DISPOSAL_METHOD_RESERVED1,
	DISPOSAL_METHOD_RESERVED2,
	DISPOSAL_METHOD_RESERVED3,
	DISPOSAL_METHOD_RESERVED4,
	DISPOSAL_METHOD__MAX,
};

extern const char *disposal_methodmap[DISPOSAL_METHOD__MAX];

enum user_input {
	USER_INPUT_NO,
	USER_INPUT_YES,
	USER_INPUT__MAX,
};

extern const char *user_inputmap[USER_INPUT__MAX];

struct gIFg {
	uint32_t         length;
	enum chunktype   type;
	uint32_t         crc;
	struct {
		uint8_t	 	disposal_method;
		uint8_t	 	user_input;
		uint16_t	delay_time;
	} __attribute__((packed)) data;
};

/* gIFx chunk */
struct gIFx {
	uint32_t         length;
	enum chunktype   type;
	uint32_t         crc;
	struct {
		uint8_t	 identifier[8];
		uint8_t	 code[3];
		uint8_t	*data;
	} __attribute__((packed)) data;
};

/* vpAg chunk */
enum vpag_unitspecifier {
	VPAG_UNITSPECIFIER_PIXEL,
	VPAG_UNITSPECIFIER__MAX,
};

extern const char *vpagunitspecifiermap[VPAG_UNITSPECIFIER__MAX];

struct vpAg {
	uint32_t         length;
	enum chunktype   type;
	uint32_t         crc;
	struct {
		uint32_t	width;
		uint32_t	height;
		uint8_t		unitspecifier;	/* enum vpag_unitspecifier */
	} __attribute__((packed)) data;
};

/* caNv chunk */
struct caNv {
	uint32_t         length;
	enum chunktype   type;
	uint32_t         crc;
	struct {
		uint32_t	width;
		uint32_t	height;
		int32_t	 	x_position;
		int32_t		y_position;
	} __attribute__((packed)) data;
};

/* orNt chunk */
typedef enum
{
	ORIENTATION_UNDEFINED,
	ORIENTATION_TOPLEFT,
	ORIENTATION_TOPRIGHT,
	ORIENTATION_BOTTOMRIGHT,
	ORIENTATION_BOTTOMLEFT,
	ORIENTATION_LEFTTOP,
	ORIENTATION_RIGHTTOP,
	ORIENTATION_RIGHTBOTTOM,
	ORIENTATION_LEFTBOTTOM,
	ORIENTATION__MAX,
} orientation;

extern const char *orientationmap[ORIENTATION__MAX];

struct orNt {
	uint32_t         length;
	enum chunktype   type;
	uint32_t         crc;
	struct {
		uint8_t	orientation;
	} __attribute__((packed)) data;
};

/* chunks */
bool		lgpng_validate_keyword(uint8_t *, size_t);
bool		lgpng_is_official_keyword(uint8_t *, size_t);

int		lgpng_create_IHDR_from_data(struct IHDR *, uint8_t *, size_t);
int		lgpng_create_PLTE_from_data(struct PLTE *, uint8_t *, size_t);
int		lgpng_create_IDAT_from_data(struct IDAT *, uint8_t *, size_t);
int		lgpng_create_tRNS_from_data(struct tRNS *, struct IHDR *, uint8_t *, size_t);
int		lgpng_create_cHRM_from_data(struct cHRM *, uint8_t *, size_t);
int		lgpng_create_gAMA_from_data(struct gAMA *, uint8_t *, size_t);
int		lgpng_create_iCCP_from_data(struct iCCP *, uint8_t *, size_t);
int		lgpng_create_sBIT_from_data(struct sBIT *, struct IHDR *, uint8_t *, size_t);
int		lgpng_create_sRGB_from_data(struct sRGB *, uint8_t *, size_t);
int		lgpng_create_cICP_from_data(struct cICP *, uint8_t *, size_t);
/*int		lgpng_create_iTXt_from_data(struct iTXt *, uint8_t *, size_t);*/
int		lgpng_create_tEXt_from_data(struct tEXt *, uint8_t *, size_t);
int		lgpng_create_zTXt_from_data(struct zTXt *, uint8_t *, size_t);
int		lgpng_create_bKGD_from_data(struct bKGD *, struct IHDR *, struct PLTE *, uint8_t *, size_t);
int		lgpng_create_hIST_from_data(struct hIST *, struct PLTE *, uint8_t *, size_t);
int		lgpng_create_pHYs_from_data(struct pHYs *, uint8_t *, size_t);
int		lgpng_create_sPLT_from_data(struct sPLT *, uint8_t *, size_t);
int		lgpng_create_eXIf_from_data(struct eXIf *, uint8_t *, size_t);
int		lgpng_create_tIME_from_data(struct tIME *, uint8_t *, size_t);
int		lgpng_create_acTL_from_data(struct acTL *, uint8_t *, size_t);
int		lgpng_create_fcTL_from_data(struct fcTL *, uint8_t *, size_t);
int		lgpng_create_fdAT_from_data(struct fdAT *, uint8_t *, size_t);
int		lgpng_create_oFFs_from_data(struct oFFs *, uint8_t *, size_t);
int		lgpng_create_gIFg_from_data(struct gIFg *, uint8_t *, size_t);
int		lgpng_create_gIFx_from_data(struct gIFx *, uint8_t *, size_t);

/* chunks_extra */
int		lgpng_create_vpAg_from_data(struct vpAg *, uint8_t *, size_t);
int		lgpng_create_caNv_from_data(struct caNv *, uint8_t *, size_t);
int		lgpng_create_orNt_from_data(struct orNt *, uint8_t *, size_t);

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
