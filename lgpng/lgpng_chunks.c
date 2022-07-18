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
		    || (keyword[i] > 126 && keyword[i] < 161)
		    || keyword[i] > 255) {
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
		return(-1);
	}
	gama->length = 4;
	gama->type = CHUNK_TYPE_gAMA;
	(void)memcpy(&(gama->data.gamma), data, 4);
	gama->data.gamma = ntohl(gama->data.gamma);
	return(0);
}

int
lgpng_create_iCCP_from_data(struct iCCP *iccp, uint8_t *data, size_t dataz)
{
	size_t	  offset;
	uint8_t	 *nul;

	if (iccp->length <= 81) {
		return(-1);
	}
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

