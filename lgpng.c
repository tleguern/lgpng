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

#include <ctype.h>
#include <endian.h>
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
	"cICP",
	"iTXt",
	"tEXt",
	"zTXt",
	"bKGD",
	"hIST",
	"pHYs",
	"sPLT",
	"eXIf",
	"tIME",
	"acTL",
	"fcTL",
	"fdAT",
	"oFFs",
	"gIFg",
	"gIFx",
	"vpAg",
	"caNv",
	"orNt",
	"skMf",
	"skRf",
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
	if (0 != memcmp(keyword, "Title", keywordz)
	    && 0 != memcmp(keyword, "Author", keywordz)
	    && 0 != memcmp(keyword, "Description", keywordz)
	    && 0 != memcmp(keyword, "Copyright", keywordz)
	    && 0 != memcmp(keyword, "Creation Time", keywordz)
	    && 0 != memcmp(keyword, "Software", keywordz)
	    && 0 != memcmp(keyword, "Disclaimer", keywordz)
	    && 0 != memcmp(keyword, "Warning", keywordz)
	    && 0 != memcmp(keyword, "Source", keywordz)
	    && 0 != memcmp(keyword, "Comment", keywordz)) {
		return(false);
	}
	return(true);
}

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
	ihdr->data.width = be32toh(ihdr->data.width);
	ihdr->data.height = be32toh(ihdr->data.height);
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
lgpng_create_IDAT_from_data(struct IDAT *idat, uint8_t *data, size_t dataz)
{
	idat->length = dataz;
	idat->type = CHUNK_TYPE_IDAT;
	idat->data.data = data;
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
		if (2 != dataz) {
			return(-1);
		}
		(void)memcpy(&(trns->data.gray), data, 2);
		trns->data.gray = be16toh(trns->data.gray);
		break;
	case COLOUR_TYPE_TRUECOLOUR:
		if (6 != dataz) {
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
		if (dataz > 256) {
			return(-1);
		}
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
lgpng_create_gAMA_from_data(struct gAMA *gama, uint8_t *data, size_t dataz)
{
	if (4 != dataz) {
		return(-1);
	}
	gama->length = 4;
	gama->type = CHUNK_TYPE_gAMA;
	(void)memcpy(&(gama->data.gamma), data, 4);
	gama->data.gamma = be32toh(gama->data.gamma);
	return(0);
}

int
lgpng_create_iCCP_from_data(struct iCCP *iccp, uint8_t *data, size_t dataz)
{
	size_t	  offset;
	uint8_t	 *nul;

	iccp->length = dataz;
	iccp->type = CHUNK_TYPE_iCCP;
	if (NULL == (nul = memchr(data, '\0', 80))) {
		return(-1);
	}
	offset = nul - data;
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
	if (offset + 1 > dataz) {
		return(-1);
	}
	iccp->data.compression = data[offset + 1];
	if (COMPRESSION_TYPE_DEFLATE != iccp->data.compression) {
		return(-1);
	}
	if (offset + 2 > dataz) {
		return(-1);
	}
	iccp->data.profile = data + offset + 2;
	iccp->data.profilez = dataz - offset - 2;
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
lgpng_create_cICP_from_data(struct cICP *cicp, uint8_t *data, size_t dataz)
{
	if (4 != dataz) {
		return(-1);
	}
	cicp->length = dataz;
	cicp->type = CHUNK_TYPE_cICP;
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
lgpng_create_tEXt_from_data(struct tEXt *text, uint8_t *data, size_t dataz)
{
	size_t	 offset;
	uint8_t	*nul;

	text->length = dataz;
	text->type = CHUNK_TYPE_tEXt;
	if (NULL == (nul = memchr(data, '\0', 80))) {
		return(-1);
	}
	offset = nul - data;
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
lgpng_create_zTXt_from_data(struct zTXt *ztxt, uint8_t *data, size_t dataz)
{
	size_t	 offset;
	uint8_t	*nul;

	ztxt->length = dataz;
	ztxt->type = CHUNK_TYPE_zTXt;
	if (NULL == (nul = memchr(data, '\0', 80))) {
		return(-1);
	}
	offset = nul - data;
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
	if (offset + 1 > dataz) {
		return(-1);
	}
	ztxt->data.compression = data[offset + 1];
	if (COMPRESSION_TYPE_DEFLATE != ztxt->data.compression) {
		return(-1);
	}
	if (offset + 2 > dataz) {
		return(-1);
	}
	ztxt->data.text = data + offset + 2;
	ztxt->data.textz = dataz - offset - 2;
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
		bkgd->data.greyscale = be16toh(bkgd->data.greyscale);
		break;
	case COLOUR_TYPE_TRUECOLOUR:
		/* FALLTHROUGH */
	case COLOUR_TYPE_TRUECOLOUR_ALPHA:
		if (6 != dataz) {
			return(-1);
		}
		rgb = (struct rgb16 *)data;
		bkgd->data.rgb.red = be16toh(rgb->red);
		bkgd->data.rgb.green = be16toh(rgb->green);
		bkgd->data.rgb.blue = be16toh(rgb->blue);
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
	phys->data.ppux = be32toh(phys->data.ppux);
	phys->data.ppuy = be32toh(phys->data.ppuy);
	phys->data.unitspecifier = data[9];
	return(0);
}

int
lgpng_create_sPLT_from_data(struct sPLT *splt, uint8_t *data, size_t dataz)
{
	int	 offset;
	uint8_t	*nul;

	splt->length = dataz;
	splt->type = CHUNK_TYPE_sPLT;
	if (NULL == (nul = memchr(data, '\0', 80))) {
		return(-1);
	}
	offset = nul - data;
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
lgpng_create_eXIf_from_data(struct eXIf *exif, uint8_t *data, size_t dataz)
{
	exif->length = dataz;
	exif->type = CHUNK_TYPE_eXIf;
	exif->data.profile = data;
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
	time->data.year = be16toh(time->data.year);
	time->data.month = data[2];
	time->data.day = data[3];
	time->data.hour = data[4];
	time->data.minute = data[5];
	time->data.second = data[6];
	return(0);
}

int
lgpng_create_acTL_from_data(struct acTL *actl, uint8_t *data, size_t dataz)
{
	if (8 != dataz) {
		return(-1);
	}
	actl->length = dataz;
	actl->type = CHUNK_TYPE_acTL;
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
lgpng_create_fcTL_from_data(struct fcTL *fctl, uint8_t *data, size_t dataz)
{
	if (26 != dataz) {
		return(-1);
	}
	fctl->length = dataz;
	fctl->type = CHUNK_TYPE_fcTL;
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
lgpng_create_fdAT_from_data(struct fdAT *fdat, uint8_t *data, size_t dataz)
{
	if (5 > dataz) {
		return(-1);
	}
	fdat->length = dataz;
	fdat->type = CHUNK_TYPE_fdAT;
	(void)memcpy(&(fdat->data.sequence_number), data, 4);
	fdat->data.sequence_number = be32toh(fdat->data.sequence_number);
	fdat->data.frame_data = data + 4;
	return(0);
}

int
lgpng_create_oFFs_from_data(struct oFFs *offs, uint8_t *data, size_t dataz)
{
	if (5 > dataz) {
		return(-1);
	}
	offs->length = dataz;
	offs->type = CHUNK_TYPE_oFFs;
	(void)memcpy(&(offs->data.x_position), data, 4);
	(void)memcpy(&(offs->data.y_position), data, 4);
	offs->data.x_position = be32toh(offs->data.x_position);
	offs->data.y_position = be32toh(offs->data.y_position);
	offs->data.unitspecifier = data[8];
	if (offs->data.unitspecifier >= OFFS_UNITSPECIFIER__MAX) {
		return(-1);
	}
	return(0);
}

int
lgpng_create_gIFg_from_data(struct gIFg *gifg, uint8_t *data, size_t dataz)
{
	if (4 > dataz) {
		return(-1);
	}
	gifg->length = dataz;
	gifg->type = CHUNK_TYPE_gIFg;
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
lgpng_create_gIFx_from_data(struct gIFx *gifx, uint8_t *data, size_t dataz)
{
	if (dataz < 11) {
		return(-1);
	}
	gifx->length = dataz;
	gifx->type = CHUNK_TYPE_gIFx;
	(void)memcpy(gifx->data.identifier, data, 8);
	(void)memcpy(gifx->data.code, data + 8, 3);
	gifx->data.data = data + 11;
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

#include <endian.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "lgpng.h"

const char *vpagunitspecifiermap[VPAG_UNITSPECIFIER__MAX] = {
	"pixel",
};

const char *orientationmap[ORIENTATION__MAX] = {
	"undefined",
	"top left",
	"top right",
	"bottom right",
	"bottom left",
	"left top",
	"right top",
	"right bottom",
	"left bottom",
};

int
lgpng_create_vpAg_from_data(struct vpAg *vpag, uint8_t *data, size_t dataz)
{
	if (9 != dataz) {
		return(-1);
	}
	(void)memcpy(&(vpag->data.width), data, 4);
	(void)memcpy(&(vpag->data.height), data + 4, 4);
	vpag->data.width = be32toh(vpag->data.width);
	vpag->data.height = be32toh(vpag->data.height);
	vpag->data.unitspecifier = data[8];
	if (0 != vpag->data.unitspecifier) {
		return(-1);
	}
	return(0);
}

int
lgpng_create_caNv_from_data(struct caNv *canv, uint8_t *data, size_t dataz)
{
	if (16 != dataz) {
		return(-1);
	}
	(void)memcpy(&(canv->data.width), data, 4);
	(void)memcpy(&(canv->data.height), data + 4, 4);
	(void)memcpy(&(canv->data.x_position), data + 8, 4);
	(void)memcpy(&(canv->data.y_position), data + 12, 4);
	canv->data.width = be32toh(canv->data.width);
	canv->data.height = be32toh(canv->data.height);
	canv->data.x_position = be32toh(canv->data.x_position);
	canv->data.y_position = be32toh(canv->data.y_position);
	return(0);
}

int
lgpng_create_orNt_from_data(struct orNt *ornt, uint8_t *data, size_t dataz)
{
	if (1 != dataz) {
		return(-1);
	}
	ornt->data.orientation = data[0];
	if (ornt->data.orientation >= ORIENTATION__MAX) {
		return(-1);
	}
	return(0);
}

int
lgpng_create_skMf_from_data(struct skMf *skmf, uint8_t *data, size_t dataz)
{
	skmf->length = dataz;
	skmf->type = CHUNK_TYPE_skMf;
	skmf->data.json = data;
	return(0);
}

int
lgpng_create_skRf_from_data(struct skRf *skrf, uint8_t *data, size_t dataz)
{
	if (dataz < 16) {
		return(-1);
	}
	skrf->length = dataz;
	skrf->type = CHUNK_TYPE_skRf;
	(void)memcpy(&(skrf->data.header), data, 16);
	skrf->data.data = data + 16;
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

#include <ctype.h>
#include <endian.h>
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
	*length = be32toh(*length);
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
	*crc = be32toh(*crc);
	return(true);
}

int
lgpng_data_write_sig(uint8_t *dest)
{
	(void)memcpy(dest, png_sig, sizeof(png_sig));
	return(sizeof(png_sig));
}

int
lgpng_data_write_chunk(uint8_t *dest, uint32_t length, uint8_t type[4],
    uint8_t *data, uint32_t crc)
{
	uint32_t nlength = htobe32(length);
	uint32_t ncrc = htobe32(crc);

	(void)memcpy(dest, (uint8_t *)&nlength, 4);
	(void)memcpy(dest + 4, type, 4);
	(void)memcpy(dest + 8, data, length);
	(void)memcpy(dest + 8 + length, (uint8_t *)&ncrc, 4);
	return(12 + length);
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

#include <ctype.h>
#include <endian.h>
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
	*length = be32toh(*length);
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
lgpng_stream_skip_data(FILE *src, uint32_t length)
{
	if (NULL == src) {
		return(false);
	}
	if (0 != length) {
		if (-1 == fseek(src, length, SEEK_CUR)) {
			fprintf(stderr, "Not enough data to skip chunk's data\n");
			return(false);
		}
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
	*crc = be32toh(*crc);
	return(true);
}

bool
lgpng_stream_write_sig(FILE *output)
{
	if (8 != fwrite(png_sig, 1, 8, output)) {
		return(false);
	}
	return(true);
}

bool
lgpng_stream_write_chunk(FILE *output, uint32_t length, uint8_t type[4],
    uint8_t *data, uint32_t crc)
{
	uint32_t nlength = htobe32(length);
	uint32_t ncrc = htobe32(crc);

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

