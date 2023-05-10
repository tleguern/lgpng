/*
 * Copyright (c) 2018,2022-2023 Tristan Le Guern <tleguern@bouledef.eu>
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
#include COMPAT_ENDIAN_H
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "lgpng.h"

uint8_t png_sig[8] = {137, 80, 78, 71, 13, 10, 26, 10};

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

const char *dispose_opmap[DISPOSE_OP__MAX] = {
	"none",
	"background",
	"previous",
};

const char *blend_opmap[BLEND_OP__MAX] = {
	"source",
	"over",
};

const char *offsunitspecifiermap[OFFS_UNITSPECIFIER__MAX] = {
	"pixel",
	"micrometer",
};

const char *ster_mode_map[STER_MODE__MAX] = {
	"cross-fuse layout",
	"diverging-fuse layout",
};

const char *disposal_methodmap[DISPOSAL_METHOD__MAX] = {
	"none",
	"do not dispose",
	"restore to background color",
	"restore to previous",
	"reserved",
	"reserved",
	"reserved",
	"reserved",
};

const char *user_inputmap[USER_INPUT__MAX] = {
	"user input is not expected",
	"user input is expected",
};

bool
lgpng_validate_keyword(uint8_t *keyword, size_t keywordz)
{
	if (' ' == keyword[0]) {
		return(false);
	}
	if (' ' == keyword[keywordz - 1]) {
		return(false);
	}
	if (NULL != memmem(keyword, keywordz,"  ", 2)) {
		return(false);
	}
	for (size_t i = 0; i < keywordz; i++) {
		if (keyword[i] < 32
		    || (keyword[i] > 126 && keyword[i] < 161)) {
			return(false);
		}
	}
	return(true);
}

bool
lgpng_is_official_keyword(uint8_t *keyword, size_t keywordz)
{
	/* From WD-png-3-20221025/ */
	if (0 != memcmp(keyword, "Title", keywordz)
	    && 0 != memcmp(keyword, "Author", keywordz)
	    && 0 != memcmp(keyword, "Description", keywordz)
	    && 0 != memcmp(keyword, "Copyright", keywordz)
	    && 0 != memcmp(keyword, "Creation Time", keywordz)
	    && 0 != memcmp(keyword, "Software", keywordz)
	    && 0 != memcmp(keyword, "Disclaimer", keywordz)
	    && 0 != memcmp(keyword, "Warning", keywordz)
	    && 0 != memcmp(keyword, "Source", keywordz)
	    && 0 != memcmp(keyword, "Comment", keywordz)
	    && 0 != memcmp(keyword, "XML:com.adobe.xmp", keywordz)
	/* From DNOTE-pngext-20221024 */
	    && 0 != memcmp(keyword, "Collection", keywordz)) {
		return(false);
	}
	return(true);
}

int
lgpng_create_IHDR_from_data(struct IHDR *ihdr, uint8_t *data, uint32_t length)
{
	if (13 != length) {
		return(-1);
	}
	ihdr->length = length;
	(void)memcpy(&(ihdr->type), "IHDR", 4);
	(void)memcpy(&(ihdr->data.width), data, 4);
	(void)memcpy(&(ihdr->data.height), data + 4, 4);
	ihdr->data.bitdepth = data[8];
	ihdr->data.colourtype = data[9];
	ihdr->data.compression = data[10];
	ihdr->data.filter = data[11];
	ihdr->data.interlace = data[12];
	ihdr->data.width = be32toh(ihdr->data.width);
	ihdr->data.height = be32toh(ihdr->data.height);
	return(0);
}

int
lgpng_create_PLTE_from_data(struct PLTE *plte, uint8_t *data, uint32_t length)
{
	size_t		 elemz;

	plte->length = length;
	elemz = length / 3;
	if (0 != length % 3 || 256 < elemz) {
		return(-1);
	}
	(void)memcpy(&(plte->type), "PLTE", 4);
	plte->data.entries = elemz;
	for (size_t i = 0, j = 0; i < elemz; i++, j += 3) {
		plte->data.entry[i].red = data[j];
		plte->data.entry[i].green = data[j + 1];
		plte->data.entry[i].blue = data[j + 2];
	}
	return(0);
}

int
lgpng_create_IDAT_from_data(struct IDAT *idat, uint8_t *data, uint32_t length)
{
	idat->length = length;
	(void)memcpy(&(idat->type), "IDAT", 4);
	idat->data.data = data;
	return(0);
}

int
lgpng_create_tRNS_from_data(struct tRNS *trns, struct IHDR *ihdr, uint8_t *data, uint32_t length)
{
	if (NULL == ihdr) {
		return(-1);
	}
	trns->length = length;
	(void)memcpy(&(trns->type), "tRNS", 4);
	trns->data.gray = 0;
	trns->data.red = 0;
	trns->data.green = 0;
	trns->data.blue = 0;
	trns->data.entries = 0;
	(void)memset(&(trns->data.palette), 0, sizeof(trns->data.palette));
	switch (ihdr->data.colourtype) {
	case COLOUR_TYPE_GREYSCALE:
		if (2 != length) {
			return(-1);
		}
		(void)memcpy(&(trns->data.gray), data, 2);
		trns->data.gray = be16toh(trns->data.gray);
		break;
	case COLOUR_TYPE_TRUECOLOUR:
		if (6 != length) {
			return(-1);
		}
		(void)memcpy(&(trns->data.red), data, 2);
		trns->data.red = be16toh(trns->data.red);
		(void)memcpy(&(trns->data.green), data, 2);
		trns->data.green = be16toh(trns->data.green);
		(void)memcpy(&(trns->data.blue), data, 2);
		trns->data.blue = be16toh(trns->data.blue);
		break;
	case COLOUR_TYPE_INDEXED:
		if (length > 256) {
			return(-1);
		}
		trns->data.entries = length;
		(void)memcpy(&(trns->data.palette), data, length);
		break;
	default:
		return(-1);
	}
	return(0);
}

int
lgpng_create_sBIT_from_data(struct sBIT *sbit, struct IHDR *ihdr, uint8_t *data, uint32_t length)
{
	if (NULL == ihdr) {
		return(-1);
	}
	sbit->length = length;
	(void)memcpy(&(sbit->type), "sBIT", 4);
	sbit->data.sgreyscale = 0;
	sbit->data.sred = 0;
	sbit->data.sgreen = 0;
	sbit->data.sblue = 0;
	sbit->data.salpha = 0;
	/* Reject invalid data size */
	switch(ihdr->data.colourtype) {
	case COLOUR_TYPE_GREYSCALE:
		if (1 != length) {
			return(-1);
		}
		break;
	case COLOUR_TYPE_TRUECOLOUR:
		/* FALLTHROUGH */
	case COLOUR_TYPE_INDEXED:
		if (3 != length) {
			return(-1);
		}
		break;
	case COLOUR_TYPE_GREYSCALE_ALPHA:
		if (2 != length) {
			return(-1);
		}
		break;
	case COLOUR_TYPE_TRUECOLOUR_ALPHA:
		if (4 != length) {
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
lgpng_create_cHRM_from_data(struct cHRM *chrm, uint8_t *data, uint32_t length)
{
	(void)memcpy(&(chrm->type), "cHRM", 4);
	if (32 != length) {
		return(-1);
	}
	(void)memcpy(&(chrm->data), data, length);
	chrm->data.whitex = be32toh(chrm->data.whitex);
	chrm->data.whitey = be32toh(chrm->data.whitey);
	chrm->data.redx = be32toh(chrm->data.redx);
	chrm->data.redy = be32toh(chrm->data.redy);
	chrm->data.greenx = be32toh(chrm->data.greenx);
	chrm->data.greeny = be32toh(chrm->data.greeny);
	chrm->data.bluex = be32toh(chrm->data.bluex);
	chrm->data.bluey = be32toh(chrm->data.bluey);
	return(0);
}

int
lgpng_create_gAMA_from_data(struct gAMA *gama, uint8_t *data, uint32_t length)
{
	if (4 != length) {
		return(-1);
	}
	gama->length = 4;
	(void)memcpy(&(gama->type), "gAMA", 4);
	(void)memcpy(&(gama->data.gamma), data, 4);
	gama->data.gamma = be32toh(gama->data.gamma);
	return(0);
}

int
lgpng_create_iCCP_from_data(struct iCCP *iccp, uint8_t *data, uint32_t length)
{
	size_t	  offset;
	uint8_t	 *nul;

	iccp->length = length;
	(void)memcpy(&(iccp->type), "iCCP", 4);
	if (NULL == (nul = memchr(data, '\0', 80))) {
		return(-1);
	}
	offset = (size_t)(nul - data);
	if (1 > offset || 79 < offset) {
		return(-1);
	}
	(void)memset(iccp->data.name, 0, sizeof(iccp->data.name));
	(void)memcpy(iccp->data.name, data, offset);
	iccp->data.namez = offset;
	if (!lgpng_validate_keyword(iccp->data.name, offset)) {
		return(-1);
	}
	/*
	 * Only one compression type allowed here too but check anyway
	 */
	if (offset + 1 > length) {
		return(-1);
	}
	iccp->data.compression = data[offset + 1];
	if (COMPRESSION_TYPE_DEFLATE != iccp->data.compression) {
		return(-1);
	}
	if (offset + 2 > length) {
		return(-1);
	}
	iccp->data.profile = data + offset + 2;
	iccp->data.profilez = length - offset - 2;
	return(0);
}

int
lgpng_create_sRGB_from_data(struct sRGB *srgb, uint8_t *data, uint32_t length)
{
	if (1 != length) {
		return(-1);
	}
	srgb->length = length;
	(void)memcpy(&(srgb->type), "sRGB", 4);
	srgb->data.intent = data[0];
	return(0);
}

int
lgpng_create_cICP_from_data(struct cICP *cicp, uint8_t *data, uint32_t length)
{
	if (4 != length) {
		return(-1);
	}
	cicp->length = length;
	(void)memcpy(&(cicp->type), "cICP", 4);
	cicp->data.colour_primaries = data[0];
	cicp->data.transfer_function = data[1];
	cicp->data.matrix_coefficients = data[2];
	cicp->data.video_full_range = data[3];
	if (0 != cicp->data.matrix_coefficients) {
		return(-1);
	}
	if (0 != cicp->data.video_full_range && 1 != cicp->data.video_full_range) {
		return(-1);
	}
	/*
	 * TODO: find the name of the transfer functions
	 * -> Tables 2, 3 & 4 of ITU-T H.273 ?
	 * */
	return(0);
}

int
lgpng_create_tEXt_from_data(struct tEXt *text, uint8_t *data, uint32_t length)
{
	size_t	 offset;
	uint8_t	*nul;

	text->length = length;
	(void)memcpy(&(text->type), "tEXt", 4);
	if (NULL == (nul = memchr(data, '\0', 80))) {
		return(-1);
	}
	offset = (size_t)(nul - data);
	(void)memset(text->data.keyword, 0, sizeof(text->data.keyword));
	(void)memcpy(text->data.keyword, data, offset);
	if (1 > offset || 79 < offset) {
		return(-1);
	}
	if (!lgpng_validate_keyword(text->data.keyword, offset)) {
		return(-1);
	}
	text->data.text = data + offset + 1;
	return(0);
}

int
lgpng_create_zTXt_from_data(struct zTXt *ztxt, uint8_t *data, uint32_t length)
{
	size_t	 offset;
	uint8_t	*nul;

	ztxt->length = length;
	(void)memcpy(&(ztxt->type), "zTXt", 4);
	if (NULL == (nul = memchr(data, '\0', 80))) {
		return(-1);
	}
	offset = (size_t)(nul - data);
	(void)memset(ztxt->data.keyword, 0, sizeof(ztxt->data.keyword));
	(void)memcpy(ztxt->data.keyword, data, offset);
	ztxt->data.keywordz = offset;
	if (1 > offset || 79 < offset) {
		return(-1);
	}
	if (!lgpng_validate_keyword(ztxt->data.keyword, offset)) {
		return(-1);
	}
	/*
	 * Only one compression type allowed here but check anyway
	 */
	if (offset + 1 > length) {
		return(-1);
	}
	ztxt->data.compression = data[offset + 1];
	if (COMPRESSION_TYPE_DEFLATE != ztxt->data.compression) {
		return(-1);
	}
	if (offset + 2 > length) {
		return(-1);
	}
	ztxt->data.text = data + offset + 2;
	ztxt->data.textz = length - offset - 2;
	return(0);
}

int
lgpng_create_bKGD_from_data(struct bKGD *bkgd, struct IHDR *ihdr, struct PLTE *plte, uint8_t *data, uint32_t length)
{
	struct rgb16	*rgb;

	/* Detect uninitialized IHDR chunk */
	if (NULL == ihdr) {
		return(-1);
	}
	/* Same with PLTE */
	if ((NULL == plte)
	    && COLOUR_TYPE_INDEXED == ihdr->data.colourtype) {
		return(-1);
	}
	bkgd->length = length;
	(void)memcpy(&(bkgd->type), "bKGD", 4);
	switch (ihdr->data.colourtype) {
	case COLOUR_TYPE_GREYSCALE:
		/* FALLTHROUGH */
	case COLOUR_TYPE_GREYSCALE_ALPHA:
		if (2 != length) {
			return(-1);
		}
		bkgd->data.greyscale = *(uint16_t *)data;
		bkgd->data.greyscale = be16toh(bkgd->data.greyscale);
		break;
	case COLOUR_TYPE_TRUECOLOUR:
		/* FALLTHROUGH */
	case COLOUR_TYPE_TRUECOLOUR_ALPHA:
		if (6 != length) {
			return(-1);
		}
		rgb = (struct rgb16 *)data;
		bkgd->data.rgb.red = be16toh(rgb->red);
		bkgd->data.rgb.green = be16toh(rgb->green);
		bkgd->data.rgb.blue = be16toh(rgb->blue);
		break;
	case COLOUR_TYPE_INDEXED:
		if (1 != length) {
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
lgpng_create_hIST_from_data(struct hIST *hist, struct PLTE *plte, uint8_t *data, uint32_t length)
{
	size_t		 elemz;
	uint16_t	*frequency;

	/* Detect uninitialized PLTE chunk */
	if (0 == plte->data.entries) {
		return(-1);
	}
	(void)memcpy(&(hist->type), "hIST", 4);
	hist->length = length;
	/* length is a series of 16-bit integers */
	elemz = length / 2;
	/* hIST mirrors the PLTE chunk so it inherits the same restrictions */
	if (0 != length % 2 || 256 < elemz) {
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
lgpng_create_pHYs_from_data(struct pHYs *phys, uint8_t *data, uint32_t length)
{
	phys->length = length;
	(void)memcpy(&(phys->type), "pHYs", 4);
	(void)memcpy(&(phys->data.ppux), data, 4);
	(void)memcpy(&(phys->data.ppuy), data + 4, 4);
	phys->data.ppux = be32toh(phys->data.ppux);
	phys->data.ppuy = be32toh(phys->data.ppuy);
	phys->data.unitspecifier = data[9];
	return(0);
}

int
lgpng_create_sPLT_from_data(struct sPLT *splt, uint8_t *data, uint32_t length)
{
	size_t offset;
	uint8_t	*nul;

	splt->length = length;
	(void)memcpy(&(splt->type), "sPLT", 4);
	if (NULL == (nul = memchr(data, '\0', 80))) {
		return(-1);
	}
	offset = (size_t)(nul - data);
	(void)memset(splt->data.palettename, 0, sizeof(splt->data.palettename));
	(void)memcpy(splt->data.palettename, data, offset);
	if (1 > offset || 79 < offset) {
		return(-1);
	}
	if (!lgpng_validate_keyword((uint8_t *)splt->data.palettename, offset)) {
		return(-1);
	}
	offset += 1;
	splt->data.sampledepth = data[offset];
	if (8 != splt->data.sampledepth && 16 != splt->data.sampledepth) {
		return(-1);
	}
	offset += 1;
	if (8 == splt->data.sampledepth) {
		splt->data.entries = (length - offset) / 6;
		if ((length - offset) % 6 != 0) {
			return(-1);
		}
	}
	if (16 == splt->data.sampledepth) {
		splt->data.entries = (length - offset) / 10;
		if ((length - offset) % 10 != 0) {
			return(-1);
		}
	}
	/* TODO: Can't do the rest without allocations */
	return(0);
}

int
lgpng_create_eXIf_from_data(struct eXIf *exif, uint8_t *data, uint32_t length)
{
	exif->length = length;
	(void)memcpy(&(exif->type), "eXIf", 4);
	exif->data.profile = data;
	return(0);
}

int
lgpng_create_tIME_from_data(struct tIME *time, uint8_t *data, uint32_t length)
{
	if (7 != length) {
		return(-1);
	}
	time->length = length;
	(void)memcpy(&(time->type), "tIME", 4);
	(void)memcpy(&(time->data.year), data, 2);
	time->data.year = be16toh(time->data.year);
	time->data.month = data[2];
	time->data.day = data[3];
	time->data.hour = data[4];
	time->data.minute = data[5];
	time->data.second = data[6];
	return(0);
}

int
lgpng_create_acTL_from_data(struct acTL *actl, uint8_t *data, uint32_t length)
{
	if (8 != length) {
		return(-1);
	}
	actl->length = length;
	(void)memcpy(&(actl->type), "acTL", 4);
	(void)memcpy(&(actl->data.num_frames), data, 4);
	(void)memcpy(&(actl->data.num_plays), data + 4, 4);
	actl->data.num_frames = be32toh(actl->data.num_frames);
	actl->data.num_plays = be32toh(actl->data.num_plays);
	if (0 == actl->data.num_frames) {
		return(-1);
	}
	return(0);
}

int
lgpng_create_fcTL_from_data(struct fcTL *fctl, uint8_t *data, uint32_t length)
{
	if (26 != length) {
		return(-1);
	}
	fctl->length = length;
	(void)memcpy(&(fctl->type), "fcTL", 4);
	(void)memcpy(&(fctl->data.sequence_number), data, 4);
	(void)memcpy(&(fctl->data.width), data + 4, 4);
	(void)memcpy(&(fctl->data.height), data + 8, 4);
	(void)memcpy(&(fctl->data.x_offset), data + 12, 4);
	(void)memcpy(&(fctl->data.y_offset), data + 16, 4);
	(void)memcpy(&(fctl->data.delay_num), data + 20, 2);
	(void)memcpy(&(fctl->data.delay_den), data + 22, 2);
	fctl->data.sequence_number = be32toh(fctl->data.sequence_number);
	fctl->data.width = be32toh(fctl->data.width);
	fctl->data.height = be32toh(fctl->data.height);
	fctl->data.x_offset = be32toh(fctl->data.x_offset);
	fctl->data.y_offset = be32toh(fctl->data.y_offset);
	fctl->data.delay_num = be16toh(fctl->data.delay_num);
	fctl->data.delay_den = be16toh(fctl->data.delay_den);
	fctl->data.dispose_op = data[24];
	fctl->data.blend_op = data[25];
	if (fctl->data.width == 0 || fctl->data.height == 0) {
		return(-1);
	}
	if (fctl->data.dispose_op >= DISPOSE_OP__MAX) {
		return(-1);
	}
	if (fctl->data.blend_op >= BLEND_OP__MAX) {
		return(-1);
	}
	return(0);
}

int
lgpng_create_fdAT_from_data(struct fdAT *fdat, uint8_t *data, uint32_t length)
{
	if (5 > length) {
		return(-1);
	}
	fdat->length = length;
	(void)memcpy(&(fdat->type), "fdAT", 4);
	(void)memcpy(&(fdat->data.sequence_number), data, 4);
	fdat->data.sequence_number = be32toh(fdat->data.sequence_number);
	fdat->data.frame_data = data + 4;
	return(0);
}

int
lgpng_create_oFFs_from_data(struct oFFs *offs, uint8_t *data, uint32_t length)
{
	if (5 > length) {
		return(-1);
	}
	offs->length = length;
	(void)memcpy(&(offs->type), "oFFs", 4);
	(void)memcpy(&(offs->data.x_position), data, 4);
	(void)memcpy(&(offs->data.y_position), data, 4);
	offs->data.x_position = (int32_t)be32toh(offs->data.x_position);
	offs->data.y_position = (int32_t)be32toh(offs->data.y_position);
	offs->data.unitspecifier = data[8];
	if (offs->data.unitspecifier >= OFFS_UNITSPECIFIER__MAX) {
		return(-1);
	}
	return(0);
}

int
lgpng_create_gIFg_from_data(struct gIFg *gifg, uint8_t *data, uint32_t length)
{
	if (4 > length) {
		return(-1);
	}
	gifg->length = length;
	(void)memcpy(&(gifg->type), "gIFg", 4);
	gifg->data.disposal_method = data[0];
	gifg->data.user_input = data[1];
	(void)memcpy(&(gifg->data.delay_time), data, 2);
	gifg->data.delay_time = be16toh(gifg->data.delay_time);
	if (gifg->data.disposal_method >= 8) {
		return(-1);
	}
	if (gifg->data.user_input >= 2) {
		return(-1);
	}
	return(0);
}

int
lgpng_create_gIFx_from_data(struct gIFx *gifx, uint8_t *data, uint32_t length)
{
	if (length < 11) {
		return(-1);
	}
	gifx->length = length;
	(void)memcpy(&(gifx->type), "gIFx", 4);
	(void)memcpy(gifx->data.identifier, data, 8);
	(void)memcpy(gifx->data.code, data + 8, 3);
	gifx->data.data = data + 11;
	return(0);
}

int
lgpng_create_sTER_from_data(struct sTER *ster, uint8_t *data, uint32_t length)
{
	if (length != 1) {
		return(-1);
	}
	ster->length = length;
	(void)memcpy(&(ster->type), "sTER", 4);
	if (data[0] >= STER_MODE__MAX) {
		return(-1);
	}
	ster->data.mode = data[0];
	return(0);
}

