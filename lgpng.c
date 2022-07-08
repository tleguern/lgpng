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

#include <arpa/inet.h>

#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#include "lgpng.h"

char png_sig[8] = {137, 80, 78, 71, 13, 10, 26, 10};

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
	"tIME"
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

const char *compressiontypemap[COMPRESSION_TYPE__MAX] = {
	"deflate",
};

const char *filtermethodmap[FILTER_METHOD__MAX] = {
	"adaptive",
};

const char *interlacemap[INTERLACE_METHOD__MAX] = {
	"standard",
	"adam7",
};

const char *rendering_intentmap[RENDERING_INTENT__MAX] = {
	"perceptual",
	"relative colorimetric",
	"saturation",
	"absolute colorimetric",
};

const char *unitspecifiermap[UNITSPECIFIER__MAX] = {
	"unknown",
	"metre",
};

int
lgpng_create_IHDR_from_data(struct IHDR *ihdr, uint8_t *data, size_t dataz)
{
	if (13 != dataz) {
		return(-1);
	}
	ihdr->length = dataz;
	ihdr->type = CHUNK_TYPE_IHDR;
	(void)memcpy(&(ihdr->data.width), data, 4);
	(void)memcpy(&(ihdr->data.height), data + 4, 4);
	ihdr->data.bitdepth = data[8];
	ihdr->data.colourtype = data[9];
	ihdr->data.compression = data[10];
	ihdr->data.filter = data[11];
	ihdr->data.interlace = data[12];
	ihdr->data.width = ntohl(ihdr->data.width);
	ihdr->data.height = ntohl(ihdr->data.height);
	return(0);
}

int
lgpng_create_PLTE_from_data(struct PLTE *plte, uint8_t *data, size_t dataz)
{
	size_t		 elemz;

	elemz = dataz / 3;
	if (0 != dataz % 3 || 256 < elemz) {
		return(-1);
	}
	plte->length = elemz;
	plte->type = CHUNK_TYPE_PLTE;
	plte->data.entries = elemz;
	for (size_t i = 0, j = 0; i < elemz; i++, j += 3) {
		plte->data.entry[i].red = data[j];
		plte->data.entry[i].green = data[j + 1];
		plte->data.entry[i].blue = data[j + 2];
	}
	return(0);
}

int
lgpng_create_tRNS_from_data(struct tRNS *trns, struct IHDR *ihdr, uint8_t *data, size_t dataz)
{
	if (NULL == ihdr) {
		return(-1);
	}
	trns->length = dataz;
	trns->type = CHUNK_TYPE_tRNS;
	trns->data.gray = -1;
	trns->data.red = -1;
	trns->data.green = -1;
	trns->data.blue = -1;
	trns->data.entries = -1;
	(void)memset(&(trns->data.palette), 0, sizeof(trns->data.palette));
	switch (ihdr->data.colourtype) {
	case COLOUR_TYPE_GREYSCALE:
		(void)memcpy(&(trns->data.gray), data, 2);
		trns->data.gray = ntohs(trns->data.gray);
		break;
	case COLOUR_TYPE_TRUECOLOUR:
		(void)memcpy(&(trns->data.red), data, 2);
		trns->data.red = ntohs(trns->data.red);
		(void)memcpy(&(trns->data.green), data, 2);
		trns->data.green = ntohs(trns->data.green);
		(void)memcpy(&(trns->data.blue), data, 2);
		trns->data.blue = ntohs(trns->data.blue);
		break;
	case COLOUR_TYPE_INDEXED:
		trns->data.entries = dataz;
		(void)memcpy(&(trns->data.palette), data, dataz);
		break;
	default:
		return(-1);
	}
	return(0);
}

int
lgpng_create_sBIT_from_data(struct sBIT *sbit, struct IHDR *ihdr, uint8_t *data, size_t dataz)
{
	if (NULL == ihdr) {
		return(-1);
	}
	sbit->length = dataz;
	sbit->type = CHUNK_TYPE_sBIT;
	sbit->data.sgreyscale = -1;
	sbit->data.sred = -1;
	sbit->data.sgreen = -1;
	sbit->data.sblue = -1;
	sbit->data.salpha = -1;
	/* Reject invalid data size */
	switch(ihdr->data.colourtype) {
	case COLOUR_TYPE_GREYSCALE:
		if (1 != dataz) {
			return(-1);
		}
		break;
	case COLOUR_TYPE_TRUECOLOUR:
		/* FALLTHROUGH */
	case COLOUR_TYPE_INDEXED:
		if (3 != dataz) {
			return(-1);
		}
		break;
	case COLOUR_TYPE_GREYSCALE_ALPHA:
		if (2 != dataz) {
			return(-1);
		}
		break;
	case COLOUR_TYPE_TRUECOLOUR_ALPHA:
		if (4 != dataz) {
			return(-1);
		}
		break;
	default:
		break;
	}
	/* Unpack data according to IHDR */
	if (COLOUR_TYPE_GREYSCALE == ihdr->data.colourtype
	    || COLOUR_TYPE_GREYSCALE_ALPHA == ihdr->data.colourtype) {
		sbit->data.sgreyscale = (uint8_t)data[0];
	} else if (COLOUR_TYPE_TRUECOLOUR == ihdr->data.colourtype
	    || COLOUR_TYPE_INDEXED == ihdr->data.colourtype
	    || COLOUR_TYPE_TRUECOLOUR_ALPHA == ihdr->data.colourtype) {
		sbit->data.sred = (uint8_t)data[0];
		sbit->data.sgreen = (uint8_t)data[1];
		sbit->data.sblue = (uint8_t)data[2];
	}
	if (COLOUR_TYPE_GREYSCALE_ALPHA == ihdr->data.colourtype) {
		sbit->data.salpha = (uint8_t)data[1];
	} else if (COLOUR_TYPE_TRUECOLOUR_ALPHA == ihdr->data.colourtype) {
		sbit->data.salpha = (uint8_t)data[3];
	}
	/* Reject invalid values */
	if (COLOUR_TYPE_GREYSCALE == ihdr->data.colourtype
	    || COLOUR_TYPE_GREYSCALE_ALPHA == ihdr->data.colourtype) {
		if (sbit->data.sgreyscale <= 0) {
			return(-1);
		}
		if (sbit->data.sgreyscale > ihdr->data.bitdepth) {
			return(-1);
		}
	}
	if (COLOUR_TYPE_GREYSCALE_ALPHA == ihdr->data.colourtype
	    || COLOUR_TYPE_TRUECOLOUR_ALPHA == ihdr->data.colourtype) {
		if (sbit->data.salpha <= 0) {
			return(-1);
		}
		if (sbit->data.salpha > ihdr->data.bitdepth) {
			return(-1);
		}
	}
	if (COLOUR_TYPE_TRUECOLOUR == ihdr->data.colourtype
	    || COLOUR_TYPE_TRUECOLOUR_ALPHA == ihdr->data.colourtype) {
		if (sbit->data.sred <= 0 || sbit->data.sgreen <= 0
		    || sbit->data.sblue <= 0) {
			return(-1);
		}
		if (sbit->data.sred > ihdr->data.bitdepth
		    || sbit->data.sgreen > ihdr->data.bitdepth
		    || sbit->data.sblue > ihdr->data.bitdepth) {
			return(-1);
		}
	}
	/* Palette based image are restricted to 8-bits */
	if (COLOUR_TYPE_INDEXED == ihdr->data.colourtype) {
		if (sbit->data.sred <= 0 || sbit->data.sgreen <= 0
		    || sbit->data.sblue <= 0) {
			return(-1);
		}
		if (sbit->data.sred > 8
		    || sbit->data.sgreen > 8
		    || sbit->data.sblue > 8) {
			return(-1);
		}
	}
	return(0);
}

int
lgpng_create_cHRM_from_data(struct cHRM *chrm, uint8_t *data, size_t dataz)
{
	chrm->type = CHUNK_TYPE_cHRM;
	if (32 != dataz) {
		return(-1);
	}
	(void)memcpy(&(chrm->data), data, dataz);
	chrm->data.whitex = ntohl(chrm->data.whitex);
	chrm->data.whitey = ntohl(chrm->data.whitey);
	chrm->data.redx = ntohl(chrm->data.redx);
	chrm->data.redy = ntohl(chrm->data.redy);
	chrm->data.greenx = ntohl(chrm->data.greenx);
	chrm->data.greeny = ntohl(chrm->data.greeny);
	chrm->data.bluex = ntohl(chrm->data.bluex);
	chrm->data.bluey = ntohl(chrm->data.bluey);
	return(0);
}

int
lgpng_create_gAMA_from_data(struct gAMA *gama, uint8_t *data, size_t dataz)
{
	if (4 != dataz) {
		gama->length = 4;
	}
	gama->type = CHUNK_TYPE_gAMA;
	(void)memcpy(&(gama->data.gamma), data, 4);
	gama->data.gamma = ntohl(gama->data.gamma);
	return(0);
}

int
lgpng_create_sRGB_from_data(struct sRGB *srgb, uint8_t *data, size_t dataz)
{
	if (1 != dataz) {
		return(-1);
	}
	srgb->length = dataz;
	srgb->type = CHUNK_TYPE_sRGB;
	srgb->data.intent = data[0];
	return(0);
}

int
lgpng_create_tEXt_from_data(struct tEXt *text, uint8_t *data, size_t dataz)
{
	/* XXX: Depends on the lifetime of data */
	text->length = dataz;
	text->type = CHUNK_TYPE_tEXt;
	text->data.keyword = (char *)data;
	text->data.text = (char *)data + strlen((char *)data) + 1;
	return(0);
}

int
lgpng_create_zTXt_from_data(struct zTXt *ztxt, uint8_t *data, size_t dataz)
{
	ztxt->length = dataz;
	ztxt->type = CHUNK_TYPE_zTXt;
	fprintf(stderr, "[zTXT: TODO]\n");
	return(0);
}

int
lgpng_create_bKGD_from_data(struct bKGD *bkgd, struct IHDR *ihdr, struct PLTE *plte, uint8_t *data, size_t dataz)
{
	struct rgb16	*rgb;

	/* Detect uninitialized IHDR chunk */
	if (NULL == ihdr || CHUNK_TYPE_IHDR != ihdr->type) {
		return(-1);
	}
	/* Same with PLTE */
	if ((NULL == plte || CHUNK_TYPE_PLTE != plte->type)
	    && COLOUR_TYPE_INDEXED == ihdr->data.colourtype) {
		return(-1);
	}
	bkgd->length = dataz;
	bkgd->type = CHUNK_TYPE_bKGD;
	switch (ihdr->data.colourtype) {
	case COLOUR_TYPE_GREYSCALE:
		/* FALLTHROUGH */
	case COLOUR_TYPE_GREYSCALE_ALPHA:
		if (2 != dataz) {
			return(-1);
		}
		bkgd->data.greyscale = *(uint16_t *)data;
		bkgd->data.greyscale = ntohs(bkgd->data.greyscale);
		break;
	case COLOUR_TYPE_TRUECOLOUR:
		/* FALLTHROUGH */
	case COLOUR_TYPE_TRUECOLOUR_ALPHA:
		if (6 != dataz) {
			return(-1);
		}
		rgb = (struct rgb16 *)data;
		bkgd->data.rgb.red = ntohs(rgb->red);
		bkgd->data.rgb.green = ntohs(rgb->green);
		bkgd->data.rgb.blue = ntohs(rgb->blue);
		break;
	case COLOUR_TYPE_INDEXED:
		if (1 != dataz) {
			return(-1);
		}
		/* Colour out of PLTE index */
		if (data[0] >= plte->data.entries) {
			return(-1);
		}
		bkgd->data.paletteindex = data[0];
		break;
	}
	return(0);
}

int
lgpng_create_hIST_from_data(struct hIST *hist, struct PLTE *plte, uint8_t *data, size_t dataz)
{
	size_t		 elemz;
	uint16_t	*frequency;

	/* Detect uninitialized PLTE chunk */
	if (CHUNK_TYPE_PLTE != plte->type || 0 == plte->data.entries) {
		return(-1);
	}
	hist->type = CHUNK_TYPE_hIST;
	hist->length = dataz;
	/* dataz is a series of 16-bit integers */
	elemz = dataz / 2;
	/* hIST mirrors the PLTE chunk so it inherits the same restrictions */
	if (0 != dataz % 2 || 256 < elemz) {
		return(-1);
	}
	/* hIST should have the exact same number of entries than PLTE */
	if (elemz != plte->data.entries) {
		return(-1);
	}
	frequency = (uint16_t *)data;
	(void)memset(hist->data.frequency, 0, sizeof(hist->data.frequency));
	for (size_t i = 0; i < elemz; i++) {
		hist->data.frequency[i] = frequency[i];
	}
	return(0);
}

int
lgpng_create_pHYs_from_data(struct pHYs *phys, uint8_t *data, size_t dataz)
{
	phys->length = dataz;
	phys->type = CHUNK_TYPE_pHYs;
	(void)memcpy(&(phys->data.ppux), data, 4);
	(void)memcpy(&(phys->data.ppuy), data + 4, 4);
	phys->data.ppux = ntohl(phys->data.ppux);
	phys->data.ppuy = ntohl(phys->data.ppuy);
	phys->data.unitspecifier = data[9];
	return(0);
}

int
lgpng_create_sPLT_from_data(struct sPLT *splt, uint8_t *data, size_t dataz)
{
	size_t	palettenamez;
	int	offset;

	splt->length = dataz;
	splt->type = CHUNK_TYPE_sPLT;
	splt->data.palettename = (char *)data;
	palettenamez = strlen(splt->data.palettename);
	if (palettenamez > 80) {
		return(-1);
	}
	offset = palettenamez + 1;
	splt->data.sampledepth = data[offset];
	if (8 != splt->data.sampledepth && 16 != splt->data.sampledepth) {
		return(-1);
	}
	offset += 1;
	if (8 == splt->data.sampledepth) {
		splt->data.entries = (dataz - offset) / 6;
		if ((dataz - offset) % 6 != 0) {
			return(-1);
		}
	}
	if (16 == splt->data.sampledepth) {
		splt->data.entries = (dataz - offset) / 10;
		if ((dataz - offset) % 10 != 0) {
			return(-1);
		}
	}
	/* TODO: Can't do the rest without allocations */
	return(0);
}

int
lgpng_create_tIME_from_data(struct tIME *time, uint8_t *data, size_t dataz)
{
	if (7 != dataz) {
		return(-1);
	}
	time->length = dataz;
	time->type = CHUNK_TYPE_tIME;
	(void)memcpy(&(time->data.year), data, 2);
	time->data.year = ntohs(time->data.year);
	time->data.month = data[3];
	time->data.day = data[4];
	time->data.hour = data[5];
	time->data.minute = data[6];
	time->data.second = data[7];
	return(0);
}

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

#include "lgpng.h"

uint32_t lgpng_crc_table[256] = {
	0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
	0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
	0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
	0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
	0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
	0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
	0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
	0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
	0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
	0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
	0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
	0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
	0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
	0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
	0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
	0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
	0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
	0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
	0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
	0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
	0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
	0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
	0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
	0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
	0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
	0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
	0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
	0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
	0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
	0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
	0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
	0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
	0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
	0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
	0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
	0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
	0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
	0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
	0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
	0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
	0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
	0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
	0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

uint32_t
lgpng_crc_init(void)
{
	return(0xffffffffL);
}

uint32_t
lgpng_crc_update(uint32_t crc, uint8_t *data, size_t dataz)
{
	uint32_t	newcrc = crc;

	for (size_t i = 0; i < dataz; i++) {
		newcrc = lgpng_crc_table[(newcrc ^ data[i]) & 0xff] ^ (newcrc >> 8);
	}
	return(newcrc);
}

uint32_t
lgpng_crc_finalize(uint32_t crc)
{
	return(crc ^ 0xffffffffL);
}

uint32_t
lgpng_crc(uint8_t *data, size_t dataz)
{
	uint32_t	crc;

	crc = lgpng_crc_init();
	crc = lgpng_crc_update(crc, data, dataz);
	crc = lgpng_crc_finalize(crc);
	return(crc);
}

bool
lgpng_chunk_crc(uint32_t length, uint8_t type[4], uint8_t *data, uint32_t *crc)
{
	(*crc) = lgpng_crc_init();
	(*crc) = lgpng_crc_update(*crc, (uint8_t *)type, 4);
	if (0 != length) {
		(*crc) = lgpng_crc_update(*crc, data, length);
	}
	(*crc) = lgpng_crc_finalize(*crc);
	return(true);
}

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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lgpng.h"

bool
lgpng_data_is_png(uint8_t *src, size_t srcz)
{
	if (NULL == src) {
		return(false);
	}
	if (srcz < 8) {
		return(false);
	}
	if (memcmp(src, png_sig, sizeof(png_sig)) == 0)
		return(true);
	return(false);
}

bool
lgpng_data_get_length(uint8_t *src, size_t srcz, uint32_t *length)
{
	if (NULL == src) {
		return(false);
	}
	if (NULL == length) {
		return(false);
	}
	if (srcz < 4) {
		return(false);
	}
	/* Read the first four bytes to gather the length of the data part */
	(void)memcpy(length, src, 4);
	*length = ntohl(*length);
	if (*length > INT32_MAX) {
		fprintf(stderr, "Chunk length is too big (%u)\n", *length);
		return(false);
	}
	return(true);
}

bool
lgpng_data_get_type(uint8_t *src, size_t srcz, int *type, uint8_t *name)
{
	uint8_t	str_type[4];

	if (NULL == src) {
		return(false);
	}
	if (NULL == type) {
		return(false);
	}
	if (srcz < 4) {
		fprintf(stderr, "Not enough data to read chunk's type\n");
		return(false);
	}

	(void)memcpy(str_type, src, 4);
	for (size_t i = 0; i < 4; i++) {
		if (isalpha(str_type[i]) == 0) {
			fprintf(stderr, "Invalid chunk type\n");
			return(false);
		}
	}
	for (size_t i = 0; i < 4; i++) {
		name[i] = str_type[i];
	}
	for (int i = 0; i < CHUNK_TYPE__MAX; i++) {
		if (strncmp((char *)str_type, chunktypemap[i], 4) == 0) {
			(*type) = i;
			break;
		}
	}
	return(true);
}

bool
lgpng_data_get_data(uint8_t *src, size_t srcz, uint32_t length, uint8_t **data)
{
	if (NULL == src) {
		return(false);
	}
	if (NULL == data) {
		return(false);
	}
	if (srcz < length) {
		fprintf(stderr, "Not enough data to read chunk's data\n");
		return(false);
	}

	if (0 != length) {
		(void)memcpy((*data), src, length);
		(*data)[length] = '\0';
	}
	return(true);
}

bool
lgpng_data_get_crc(uint8_t *src, size_t srcz, uint32_t *crc)
{
	if (NULL == src) {
		return(false);
	}
	if (NULL == crc) {
		return(false);
	}
	if (srcz < 4) {
		fprintf(stderr, "Not enough data to read chunk's CRC\n");
		return(false);
	}

	(void)memcpy(crc, src, 4);
	*crc = ntohl(*crc);
	return(true);
}

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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lgpng.h"

bool
lgpng_stream_is_png(FILE *src)
{
	char sig[8] = {  0,  0,  0,  0,  0,  0,  0,  0};

	if (NULL == src) {
		return(false);
	}
	if (sizeof(sig) != fread(sig, 1, sizeof(sig), src)) {
		return(false);
	}
	if (memcmp(sig, png_sig, sizeof(sig)) == 0)
		return(true);
	return(false);
}

bool
lgpng_stream_get_length(FILE *src, uint32_t *length)
{
	if (NULL == src) {
		return(false);
	}
	if (NULL == length) {
		return(false);
	}
	/* Read the first four bytes to gather the length of the data part */
	if (1 != fread(length, 4, 1, src)) {
		fprintf(stderr, "Not enough data to read chunk's length\n");
		return(false);
	}
	*length = ntohl(*length);
	if (*length > INT32_MAX) {
		fprintf(stderr, "Chunk length is too big (%d)\n", *length);
		return(false);
	}
	return(true);
}

bool
lgpng_stream_get_type(FILE *src, int *type, uint8_t *name)
{
	uint8_t	str_type[4];

	if (NULL == src) {
		return(false);
	}
	if (NULL == type) {
		return(false);
	}
	if (4 != fread(str_type, 1, 4, src)) {
		fprintf(stderr, "Not enough data to read chunk's type\n");
		return(false);
	}
	for (size_t i = 0; i < 4; i++) {
		if (isalpha(str_type[i]) == 0) {
			fprintf(stderr, "Invalid chunk type\n");
			return(false);
		}
	}
	for (size_t i = 0; i < 4; i++) {
		name[i] = str_type[i];
	}
	for (int i = 0; i < CHUNK_TYPE__MAX; i++) {
		if (strncmp((char *)str_type, chunktypemap[i], 4) == 0) {
			(*type) = i;
			break;
		}
	}
	return(true);
}

bool
lgpng_stream_get_data(FILE *src, uint32_t length, uint8_t **data)
{
	if (NULL == src) {
		return(false);
	}
	if (NULL == data) {
		return(false);
	}
	if (0 != length) {
		if (length != fread(*data, 1, length, src)) {
			fprintf(stderr, "Not enough data to read chunk's data\n");
			return(false);
		}
		(*data)[length] = '\0';
	}
	return(true);
}

bool
lgpng_stream_get_crc(FILE *src, uint32_t *crc)
{
	if (NULL == src) {
		return(false);
	}
	if (NULL == crc) {
		return(false);
	}
	if (1 != fread(crc, 4, 1, src)) {
		fprintf(stderr, "Not enough data to read chunk's CRC\n");
		return(false);
	}
	*crc = ntohl(*crc);
	return(true);
}

bool
lgpng_stream_write_chunk(FILE *output, uint32_t length, uint8_t *type,
    uint8_t *data, uint32_t crc)
{
	uint32_t nlength = htonl(length);
	uint32_t ncrc = htonl(crc);

	if (4 != fwrite((uint8_t *)&nlength, 1, 4, output)) {
		return(false);
	}
	if (4 != fwrite(type, 1, 4, output)) {
		return(false);
	}
	if (length != fwrite(data, 1, length, output)) {
		return(false);
	}
	if (4 != fwrite((uint8_t *)&ncrc, 1, 4, output)) {
		return(false);
	}
	return(true);
}

