/*
 * Copyright (c) 2022 Tristan Le Guern <tleguern@bouledef.eu>
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

#include <stdint.h>

#ifndef ICC_H__
#define ICC_H__

enum icc_class {
	ICC_CLASS_INPUT,
	ICC_CLASS_DISPLAY,
	ICC_CLASS_OUTPUT,
	ICC_CLASS_DEVICELINK,
	ICC_CLASS_COLORSPACE,
	ICC_CLASS_ABSTRACT,
	ICC_CLASS_NAMED_COLOR,
	ICC_CLASS__MAX,
};

extern const char *icc_classmap[ICC_CLASS__MAX];

enum icc_colorspace {
	ICC_COLORSPACE_XYZ,
	ICC_COLORSPACE_LAB,
	ICC_COLORSPACE_LUV,
	ICC_COLORSPACE_YCBR,
	ICC_COLORSPACE_YXY,
	ICC_COLORSPACE_RGB,
	ICC_COLORSPACE_GRAY,
	ICC_COLORSPACE_HSV,
	ICC_COLORSPACE_HLS,
	ICC_COLORSPACE_CMYK,
	ICC_COLORSPACE_CMY,
	ICC_COLORSPACE_2CLR,
	ICC_COLORSPACE_3CLR,
	ICC_COLORSPACE_4CLR,
	ICC_COLORSPACE_5CLR,
	ICC_COLORSPACE_6CLR,
	ICC_COLORSPACE_7CLR,
	ICC_COLORSPACE_8CLR,
	ICC_COLORSPACE_9CLR,
	ICC_COLORSPACE_10CLR,
	ICC_COLORSPACE_11CLR,
	ICC_COLORSPACE_12CLR,
	ICC_COLORSPACE_13CLR,
	ICC_COLORSPACE_14CLR,
	ICC_COLORSPACE_15CLR,
	ICC_COLORSPACE__MAX,
};

extern const char *icc_colorspacemap[ICC_COLORSPACE__MAX];

enum icc_platform {
	ICC_PLATFORM_APPLE,
	ICC_PLATFORM_MICROSOFT,
	ICC_PLATFORM_SGI,
	ICC_PLATFORM_SUN,
	ICC_PLATFORM_TALIGENT,
	ICC_PLATFORM__MAX,
};

extern const char *icc_platformmap[ICC_PLATFORM__MAX];

enum icc_rendering_intent {
	ICC_RENDERING_INTENT_PERCEPTUAL,
	ICC_RENDERING_INTENT_MEDIA_RELATIVE,
	ICC_RENDERING_INTENT_SATURATION,
	ICC_RENDERING_INTENT_ABSOLUTE,
	ICC_RENDERING_INTENT__MAX,
};

extern const char *icc_rendering_intentmap[ICC_RENDERING_INTENT__MAX];

struct icc_profile {
	uint32_t	size;
	uint8_t		major_version;
	uint8_t		minor_version;
	uint8_t		patch_version;
	uint8_t         cmm_type_sig[4];
	uint8_t		class;			/* enum icc_class */
	uint8_t		colorspace;		/* enum icc_colorspace */
	uint8_t		profile_colorspace;	/* enum icc_colorspace */
	uint16_t	year;
	uint16_t	month;
	uint16_t	day;
	uint16_t	hour;
	uint16_t	minute;
	uint16_t	second;
	uint8_t		platform;		/* enum icc_platform */
	uint32_t	flags;
	uint8_t         manufacturer[4];
	uint8_t         model[4];
	uint64_t	attributes;
	uint8_t         rendering_intent;	/* enum icc_rendering_intent */
	uint8_t         creator[4];
};

/* helper macro */
#define NIBBLE1(i) ((i >> 4) & 0x0F)
#define NIBBLE2(i) (i & 0x0F)

bool	icc_create_from_data(struct icc_profile *, uint8_t *, size_t);

#endif
