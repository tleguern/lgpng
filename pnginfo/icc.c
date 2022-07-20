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

#include <arpa/inet.h>

#include <err.h>
#include <stdbool.h>
#include <string.h>

#include "icc.h"

const char *icc_classmap[ICC_CLASS__MAX] = {
	"Input Device profile",
	"Display Device profile",
	"Output Device profile",
	"DeviceLink profile",
	"ColorSpace Conversion profile",
	"Abstract profile",
	"Named Color profile",
};

const char *icc_colorspacemap[ICC_COLORSPACE__MAX] = {
	"XYZData",
	"labData",
	"luvData",
	"YCbCrData",
	"YxyData",
	"rgbData",
	"grayData",
	"hsvData",
	"hlsData",
	"cmykData",
	"cmyData",
	"2colorData",
	"3colorData",
	"4colorData",
	"5colorData",
	"6colorData",
	"7colorData",
	"8colorData",
	"9colorData",
	"10colorData",
	"11colorData",
	"12colorData",
	"13colorData",
	"14colorData",
	"15colorData",
};

const char *icc_platformmap[ICC_PLATFORM__MAX] = {
	"Apple Computer, Inc",
	"Microsoft Corporation",
	"Silicon Graphics, Inc",
	"Sun Microsystems, Inc",
	"Taligent, Inc",
};

const char *icc_rendering_intentmap[ICC_RENDERING_INTENT__MAX] = {
	"Perceptual",
	"Media-Relative Colorimetric",
	"Saturation",
	"ICC-Absolute Colorimetric"
};

struct icc_class_match {
	const char *const	signature;
	enum icc_class		class;
} icc_class_match[ICC_CLASS__MAX] = {
	{ "scnr", ICC_CLASS_INPUT       },
	{ "mntr", ICC_CLASS_DISPLAY     },
	{ "prtr", ICC_CLASS_OUTPUT      },
	{ "link", ICC_CLASS_DEVICELINK  },
	{ "spac", ICC_CLASS_COLORSPACE  },
	{ "abst", ICC_CLASS_ABSTRACT    },
	{ "nmcl", ICC_CLASS_NAMED_COLOR },
};

struct icc_colorspace_match {
	const char *const	signature;
	enum icc_colorspace	colorspace;
} icc_colorspace_match[ICC_COLORSPACE__MAX] = {
	{ "XYZ ",  ICC_COLORSPACE_XYZ   },
	{ "Lab ",  ICC_COLORSPACE_LAB   },
	{ "Luv ",  ICC_COLORSPACE_LUV   },
	{ "YCbr",  ICC_COLORSPACE_YCBR  },
	{ "Yxy ",  ICC_COLORSPACE_YXY   },
	{ "RGB ",  ICC_COLORSPACE_RGB   },
	{ "GRAY",  ICC_COLORSPACE_GRAY  },
	{ "HSV ",  ICC_COLORSPACE_HSV   },
	{ "HLS ",  ICC_COLORSPACE_HLS   },
	{ "CMYK",  ICC_COLORSPACE_CMYK  },
	{ "CMY ",  ICC_COLORSPACE_CMY   },
	{ "2CLR",  ICC_COLORSPACE_2CLR  },
	{ "3CLR",  ICC_COLORSPACE_3CLR  },
	{ "4CLR",  ICC_COLORSPACE_4CLR  },
	{ "5CLR",  ICC_COLORSPACE_5CLR  },
	{ "6CLR",  ICC_COLORSPACE_6CLR  },
	{ "7CLR",  ICC_COLORSPACE_7CLR  },
	{ "8CLR",  ICC_COLORSPACE_8CLR  },
	{ "9CLR",  ICC_COLORSPACE_9CLR  },
	{ "10CLR", ICC_COLORSPACE_10CLR },
	{ "11CLR", ICC_COLORSPACE_11CLR },
	{ "12CLR", ICC_COLORSPACE_12CLR },
	{ "13CLR", ICC_COLORSPACE_13CLR },
	{ "14CLR", ICC_COLORSPACE_14CLR },
	{ "15CLR", ICC_COLORSPACE_15CLR },
};

struct icc_platform_match {
	const char *const	signature;
	enum icc_platform	platform;
} icc_platform_match[ICC_PLATFORM__MAX] = {
	{ "APPL", ICC_PLATFORM_APPLE     },
	{ "MSFT", ICC_PLATFORM_MICROSOFT },
	{ "SGI ", ICC_PLATFORM_SGI       },
	{ "SUNW", ICC_PLATFORM_SUN       },
	{ "TGNT", ICC_PLATFORM_TALIGENT  },
};

bool
icc_create_from_data(struct icc_profile *profile, uint8_t *data, size_t dataz)
{
	int		i;
	uint8_t		device_class_sig[5];
	uint8_t		color_space_sig[5];
	uint8_t 	profile_conn_space_sig[5];
	uint8_t		primary_platform_sig[5];
	uint32_t	rendering;

	/*
	 * ICC profile header
	 */

	/* Size */
	(void)memcpy((uint8_t *)&(profile->size), data, 4);
	profile->size = ntohl(profile->size);
	if (profile->size != dataz) {
		warnx("iCCP: profile size registered in the iCC profile is different than the chunk size.");
		return(false);
	}
	/* CMM Type signature */
	/*
	 * There is a registery for CMM Type here:
	 * https://www.color.org/registry/signature/TagRegistry-2021-03.pdf
	 * But it is not mandatory to register a name in it so store the 
	 * four bytes and don't do anything else.
	 */
	(void)memcpy(profile->cmm_type_sig, data + 4, 4);
	/* Version number */
	profile->major_version = data[8];
	profile->minor_version = NIBBLE1(data[9]);
	profile->patch_version = NIBBLE2(data[9]);
	if (0 != data[10] || 0 != data[11]) {
		warnx("iCCP: profile: version reserved bits are not zero");
		return(false);
	}
	/* Profile/Device Class signature */
	(void)memcpy(device_class_sig, data + 12, 4);
	device_class_sig[4] = '\0';
	for (i = 0; i < ICC_CLASS__MAX; i ++) {
		if (0 == memcmp(icc_class_match[i].signature,
		    device_class_sig, 4)) {
			profile->class = icc_class_match[i].class;
			break;
		}
	}
	if (ICC_CLASS__MAX == i) {
		warnx("iCCP: profile: unknown ICC class %s", device_class_sig);
		return(false);
	}
	/* Color space of data */
	(void)memcpy(color_space_sig, data + 16, 4);
	color_space_sig[4] = '\0';
	for (i = 0; i < ICC_COLORSPACE__MAX; i ++) {
		if (0 == memcmp(icc_colorspace_match[i].signature,
		    color_space_sig, 4)) {
			profile->colorspace = icc_colorspace_match[i].colorspace;
			break;
		}
	}
	if (ICC_COLORSPACE__MAX == i) {
		warnx("iCCP: profile: unknown ICC color space %s",
		    color_space_sig);
		return(false);
	}
	/* Connection Space */
	(void)memcpy(profile_conn_space_sig, data + 20, 4);
	profile_conn_space_sig[4] = '\0';
	for (i = 0; i < ICC_COLORSPACE__MAX; i ++) {
		if (0 == memcmp(icc_colorspace_match[i].signature,
		    profile_conn_space_sig, 4)) {
			profile->profile_colorspace = icc_colorspace_match[i].colorspace;
			break;
		}
	}
	if (ICC_COLORSPACE__MAX == i) {
		warnx("iCCP: profile: unknown ICC color space %s",
		    profile_conn_space_sig);
		return(false);
	}
	if (i >= 2 && ICC_CLASS_DEVICELINK == profile->class) {
		warnx("iCCP: profile: DeviceLink profile are limited to XYZData amd labData color spaces, not %s",
		    profile_conn_space_sig);
		return(false);
	}
	/* Date and time this profile was first created */
	(void)memcpy((uint8_t *)&(profile->year), data + 24, 2);
	(void)memcpy((uint8_t *)&(profile->month), data + 26, 2);
	(void)memcpy((uint8_t *)&(profile->day), data + 28, 2);
	(void)memcpy((uint8_t *)&(profile->hour), data + 30, 2);
	(void)memcpy((uint8_t *)&(profile->minute), data + 32, 2);
	(void)memcpy((uint8_t *)&(profile->second), data + 34, 2);
	profile->year = ntohs(profile->year);
	profile->month = ntohs(profile->month);
	profile->day = ntohs(profile->day);
	profile->hour = ntohs(profile->hour);
	profile->minute = ntohs(profile->minute);
	profile->second = ntohs(profile->second);
	/* File signature */
	if (0 != memcmp("acsp", data + 36, 4)) {
		warnx("Wrong file signature, should be 'acsp'");
		return(false);
	}
	/* Primary Platform signature */
	(void)memcpy((uint8_t *)&(primary_platform_sig), data + 40, 4);
	primary_platform_sig[4] = '\0';
	for (i = 0; i < ICC_PLATFORM__MAX; i ++) {
		if (0 == memcmp(icc_platform_match[i].signature,
		    primary_platform_sig, 4)) {
			profile->platform = icc_platform_match[i].platform;
			break;
		}
	}
	if (ICC_PLATFORM__MAX == i) {
		warnx("iCCP: profile: unknown ICC platform %s",
		    primary_platform_sig);
		return(false);
	}
	/* Flags */
	(void)memcpy((uint8_t *)&(profile->flags), data + 44, 4);
	/* TODO */
	/* Device manufacturer */
	(void)memcpy((uint8_t *)&(profile->manufacturer), data + 48, 4);
	/* Device model */
	(void)memcpy((uint8_t *)&(profile->model), data + 52, 4);
	/* Device attributes unique to the particular device */
	(void)memcpy((uint8_t *)&(profile->attributes), data + 56, 8);
	/* TODO */
	/* Rendering Intent */
	(void)memcpy((uint8_t *)&(rendering), data + 64, 4);
	rendering = ntohl(rendering);
	if (rendering > 4) {
		warnx("iCCP: profile: unknown rendering intent %d",
		    rendering);
		return(false);

	}
	profile->rendering_intent = (uint8_t)rendering;
	/* illuminant of the Profile Connection Space */
	/* TODO */
	/* Creator signature */
	(void)memcpy((uint8_t *)&(profile->creator), data + 80, 4);
	/* Skip 44 bytes */
	return(true);
}

