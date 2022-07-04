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

enum chunktype
lgpng_identify_chunk(struct unknown_chunk *chunk)
{
	for (int i = 0; i < CHUNK_TYPE__MAX; i++) {
		if (strncmp(chunk->type, chunktypemap[i], 4) == 0) {
			return(i);
		}
	}
	return(CHUNK_TYPE__MAX);
}

int
lgpng_get_next_chunk_from_bytes(uint8_t *src, struct unknown_chunk *dst, uint8_t **data)
{
	int offset = 0;

	/* Read the first four bytes to gather the length of the data part */
	(void)memcpy(&(dst->length), src, 4);
	dst->length = ntohl(dst->length);
	if (dst->length > INT32_MAX) {
		fprintf(stderr, "Chunk length is too big (%d)\n", dst->length);
		return(-1);
	}
	offset += 4;
	/* Read the chunk type */
	(void)memcpy(&(dst->type), src + offset, 4);
	dst->type[4] = '\0';
	for (size_t i = 0; i < 4; i++) {
		if (isalpha(dst->type[i]) == 0) {
			fprintf(stderr, "Invalid chunk type\n");
			return(-1);
		}
	}
	offset += 4;
	/* Read the chunk data */
	if (0 != dst->length) {
		if (NULL == ((*data) = malloc(dst->length + 1))) {
			fprintf(stderr, "malloc(dst->length)\n");
			return(-1);
		}
		(void)memcpy(*data, src + offset, dst->length);
		(*data)[dst->length] = '\0';
	}
	offset += dst->length;
	/* Read the CRC */
	(void)memcpy(&(dst->crc), src + offset, 4);
	dst->crc = ntohl(dst->crc);
	return(0);
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

	plte->length = dataz;
	plte->type = CHUNK_TYPE_PLTE;
	elemz = dataz / 3;
	if (0 != dataz % 3 || 256 < elemz) {
		return(-1);
	}
	plte->data.entries = elemz;
	for (size_t i = 0, j = 0; i < elemz; i++, j += 3) {
		plte->data.entry[i].red = data[j];
		plte->data.entry[i].green = data[j + 1];
		plte->data.entry[i].blue = data[j + 2];
	}
	return(0);
}

int
lgpng_create_tRNS_from_data(struct tRNS *trns, uint8_t *data, size_t dataz)
{
	trns->length = dataz;
	trns->type = CHUNK_TYPE_tRNS;
	fprintf(stderr, "[tRNS: Needs IHDR]\n");
	return(0);
}

int
lgpng_create_sBIT_from_data(struct sBIT *sbit, uint8_t *data, size_t dataz)
{
	sbit->length = dataz;
	sbit->type = CHUNK_TYPE_sBIT;
	fprintf(stderr, "[sBIT: Needs IHDR]\n");
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

