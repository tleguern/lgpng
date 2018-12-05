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

#include <arpa/inet.h>

#include <err.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#include "lgpng.h"

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

const char *filtertypemap[FILTER_TYPE__MAX] = {
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

struct lgpng *
lgpng_new(void)
{
	return(calloc(1, sizeof(struct lgpng)));
}

void
lgpng_free(struct lgpng *lgpng)
{
	free(lgpng->ihdr);
	lgpng->ihdr = NULL;
	free(lgpng->plte);
	lgpng->plte = NULL;
	free(lgpng->chrm);
	lgpng->chrm = NULL;
	free(lgpng->gama);
	lgpng->gama = NULL;
	free(lgpng->sbit);
	lgpng->sbit = NULL;
	free(lgpng->srgb);
	lgpng->srgb = NULL;
	free(lgpng->bkgd);
	lgpng->bkgd = NULL;
	free(lgpng->hist);
	lgpng->hist = NULL;
	free(lgpng->phys);
	lgpng->phys = NULL;
	for (size_t i = 0; i < lgpng->spltz; i++) {
		free(lgpng->splt[i]);
		lgpng->splt[i] = NULL;
	}
	free(lgpng->splt);
	lgpng->splt = NULL;
	free(lgpng->time);
	lgpng->time = NULL;
	for (size_t i = 0; i < lgpng->textz; i++) {
		free(lgpng->text[i]);
		lgpng->text[i] = NULL;
	}
	free(lgpng->text);
	lgpng->text = NULL;
	free(lgpng);
}

void
lgpng_parse_IHDR(struct lgpng *ctx, uint8_t *data, size_t dataz)
{
	struct IHDR	*ihdr;
	(void)dataz;

	if (NULL != ctx->ihdr) {
		errx(EXIT_FAILURE, "IHDR: IHDR can not appear multiple times");
	}
	if (sizeof(struct IHDR) != dataz) {
		errx(EXIT_FAILURE, "IHDR: invalid chunk size");
	}
	ihdr = (struct IHDR *)data;
	ihdr->width = ntohl(ihdr->width);
	ihdr->height = ntohl(ihdr->height);
	if (NULL == (ctx->ihdr = calloc(1, sizeof(struct IHDR)))) {
		err(EXIT_FAILURE, "calloc");
	}
	ctx->ihdr->width = ihdr->width;
	ctx->ihdr->height = ihdr->height;
	ctx->ihdr->colourtype = ihdr->colourtype;
	ctx->ihdr->bitdepth = ihdr->bitdepth;
	ctx->ihdr->compression = ihdr->compression;
	ctx->ihdr->filter = ihdr->filter;
	ctx->ihdr->interlace = ihdr->interlace;
}

void
lgpng_parse_PLTE(struct lgpng *ctx, uint8_t *data, size_t dataz)
{
	size_t		 elemz;
	struct rgb8	*rgb8;

	if (NULL == ctx->ihdr) {
		errx(EXIT_FAILURE, "PLTE: IHDR chunk is missing");
	}
	if (NULL != ctx->plte) {
		errx(EXIT_FAILURE, "PLTE: PLTE can not appear multiple times");
	}
	if (ctx->ihdr)
	elemz = dataz / 3;
	if (0 != dataz % 3 || 256 < elemz) {
		errx(EXIT_FAILURE, "PLTE: Invalid chunk size");
	}
	rgb8 = (struct rgb8 *)data;
	if (NULL == (ctx->plte = calloc(1, sizeof(struct PLTE)))) {
		err(EXIT_FAILURE, "calloc");
	}
	for (size_t i = 0; i < elemz; i++) {
		ctx->plte->entry[i].red = rgb8[i].red;
		ctx->plte->entry[i].green = rgb8[i].green;
		ctx->plte->entry[i].blue = rgb8[i].blue;
	}
	ctx->plte->entries = elemz;
}

void
lgpng_parse_tRNS(struct lgpng *ctx, uint8_t *data, size_t dataz)
{
	struct tRNS	*trns;

	if (NULL == ctx->ihdr) {
		errx(EXIT_FAILURE, "tRNS: IHDR chunk is missing");
	}
	if (ctx->ihdr->colourtype == COLOUR_TYPE_INDEXED && NULL == ctx->plte) {
		errx(EXIT_FAILURE, "tRNS: PLTE chunk is missing");
	}
	if (NULL != ctx->trns) {
		errx(EXIT_FAILURE, "tRNS: tRNS can not appear multiple times");
	}
	if (NULL == (ctx->trns = calloc(1, sizeof(struct tRNS)))) {
		err(EXIT_FAILURE, "calloc");
	}
	trns = (struct tRNS *)data;
	switch(ctx->ihdr->colourtype) {
	case COLOUR_TYPE_GREYSCALE:
		if (2 != dataz) {
			errx(EXIT_FAILURE, "tRNS: Invalid chunk size");
		}
		ctx->trns->gray = ntohs(trns->gray);
		break;
	case COLOUR_TYPE_TRUECOLOUR:
		if (6 != dataz) {
			errx(EXIT_FAILURE, "tRNS: Invalid chunk size");
		}
		ctx->trns->red = ntohs(trns->gray); /* Yes. */
		ctx->trns->green = ntohs(trns->red);
		ctx->trns->blue = ntohs(trns->green);
		break;
	case COLOUR_TYPE_INDEXED:
		if (NULL == ctx->plte) {
			errx(EXIT_FAILURE, "tRNS: Invalid chunk position");
		}
		if (ctx->plte->entries != dataz) {
			errx(EXIT_FAILURE, "tRNS: Invalid chunk size");
		}
		memset(ctx->trns->palette, 255, sizeof(ctx->trns->palette));
		for (size_t i = 0; i < ctx->plte->entries; i++) {
			ctx->trns->palette[i] = data[i];
		}
		break;
	}
}

void
lgpng_parse_sBIT(struct lgpng *ctx, uint8_t *data, size_t dataz)
{
	if (NULL == ctx->ihdr) {
		errx(EXIT_FAILURE, "sBIT: IHDR chunk is missing");
	}
	if (NULL != ctx->sbit) {
		errx(EXIT_FAILURE, "sBIT: sBIT can not appear multiple times");
	}
	if (NULL == (ctx->sbit = calloc(1, sizeof(struct sBIT)))) {
		err(EXIT_FAILURE, "calloc");
	}
	switch(ctx->ihdr->colourtype) {
	case COLOUR_TYPE_GREYSCALE:
		if (1 != dataz) {
			errx(EXIT_FAILURE, "sBIT: Invalid chunk size");
		}
		ctx->sbit->type = sBIT_TYPE_0;
		ctx->sbit->sgreyscale = (uint8_t)data[0];
		break;
	case COLOUR_TYPE_TRUECOLOUR:
		/* FALLTHROUGH */
	case COLOUR_TYPE_INDEXED:
		if (3 != dataz) {
			errx(EXIT_FAILURE, "sBIT: Invalid chunk size");
		}
		ctx->sbit->type = sBIT_TYPE_23;
		ctx->sbit->sred = (uint8_t)data[0];
		ctx->sbit->sgreen = (uint8_t)data[1];
		ctx->sbit->sblue = (uint8_t)data[2];
		break;
	case COLOUR_TYPE_GREYSCALE_ALPHA:
		if (2 != dataz) {
			errx(EXIT_FAILURE, "sBIT: Invalid chunk size");
		}
		ctx->sbit->type = sBIT_TYPE_4;
		ctx->sbit->sgreyscale = (uint8_t)data[0];
		ctx->sbit->salpha = (uint8_t)data[1];
		break;
	case COLOUR_TYPE_TRUECOLOUR_ALPHA:
		if (4 != dataz) {
			errx(EXIT_FAILURE, "sBIT: Invalid chunk size");
		}
		ctx->sbit->type = sBIT_TYPE_6;
		ctx->sbit->sred = (uint8_t)data[0];
		ctx->sbit->sgreen = (uint8_t)data[1];
		ctx->sbit->sblue = (uint8_t)data[2];
		ctx->sbit->salpha = (uint8_t)data[3];
		break;
	}
}

void
lgpng_parse_cHRM(struct lgpng *ctx, uint8_t *data, size_t dataz)
{
	struct cHRM	*chrm;

	if (NULL != ctx->chrm) {
		errx(EXIT_FAILURE, "cHRM: cHRM can not appear multiple times");
	}
	if (sizeof(struct cHRM) != dataz) {
		errx(EXIT_FAILURE, "cHRM: invalid chunk size");
	}
	chrm = (struct cHRM *)data;
	if (NULL == (ctx->chrm = calloc(1, sizeof(struct cHRM)))) {
		err(EXIT_FAILURE, "calloc");
	}
	ctx->chrm->whitex = ntohl(chrm->whitex);
	ctx->chrm->whitey = ntohl(chrm->whitey);
	ctx->chrm->redx = ntohl(chrm->redx);
	ctx->chrm->redy = ntohl(chrm->redy);
	ctx->chrm->greenx = ntohl(chrm->greenx);
	ctx->chrm->greeny = ntohl(chrm->greeny);
	ctx->chrm->bluex = ntohl(chrm->bluex);
	ctx->chrm->bluey = ntohl(chrm->bluey);
}

void
lgpng_parse_gAMA(struct lgpng *ctx, uint8_t *data, size_t dataz)
{
	struct gAMA	*gama;

	if (NULL != ctx->gama) {
		errx(EXIT_FAILURE, "gAMA: gAMA can not appear multiple times");
	}
	if (sizeof(struct gAMA) != dataz) {
		errx(EXIT_FAILURE, "gAMA: invalid chunk size");
	}
	gama = (struct gAMA *)data;
	gama->gama = ntohl(gama->gama);
	if (NULL == (ctx->gama = calloc(1, sizeof(struct gAMA)))) {
		err(EXIT_FAILURE, "calloc");
	}
	ctx->gama->gama = gama->gama;
}

void
lgpng_parse_sRGB(struct lgpng *ctx, uint8_t *data, size_t dataz)
{
	struct sRGB	*srgb;

	if (NULL != ctx->srgb) {
		errx(EXIT_FAILURE, "sRGB: sRGB can not appear multiple times");
	}
	if (sizeof(struct sRGB) != dataz) {
		errx(EXIT_FAILURE, "sRGB: invalid chunk size");
	}
	srgb = (struct sRGB *)data;
	if (NULL == (ctx->srgb = calloc(1, sizeof(struct sRGB)))) {
		err(EXIT_FAILURE, "calloc");
	}
	ctx->srgb->intent = srgb->intent;
}

void
lgpng_parse_tEXt(struct lgpng *ctx, uint8_t *data, size_t dataz)
{
	struct tEXt	  text;
	struct tEXt	**textpp;

	(void)dataz;
	text.keyword = (char *)data;
	if (strlen(text.keyword) >= 80) {
		errx(EXIT_FAILURE, "tEXt: Invalid keyword size");
	}
	text.text = (char *)data + strlen((char *)data) + 1;
	if ('\n' == text.text[strlen(text.text) - 1]) {
		text.text[strlen(text.text) - 1] = '\0';
	}
	textpp = reallocarray(ctx->text, ctx->textz + 1, sizeof(struct tEXt *));
	if (NULL == textpp) {
		free(ctx->text);
		ctx->text = NULL;
		ctx->textz = 0;
		err(EXIT_FAILURE, "reallocarray");
	}
	ctx->text = textpp;
	if (NULL == (ctx->text[ctx->textz] = calloc(1, sizeof(struct tEXt)))) {
		err(EXIT_FAILURE, "calloc");
	}
	ctx->text[ctx->textz]->keyword = text.keyword;
	ctx->text[ctx->textz]->text = text.text;
	ctx->textz = ctx->textz + 1;
}

void
lgpng_parse_bKGD(struct lgpng *ctx, uint8_t *data, size_t dataz)
{
	uint16_t *tmp;
	struct rgb16	*rgb;

	if (NULL == ctx->ihdr) {
		errx(EXIT_FAILURE, "bKGD: IHDR chunk is missing");
	}
	if (ctx->ihdr->colourtype == COLOUR_TYPE_INDEXED && NULL == ctx->plte) {
		errx(EXIT_FAILURE, "bKGD: PLTE chunk is missing");
	}
	if (NULL != ctx->phys) {
		errx(EXIT_FAILURE, "bKGD: bKGD can not appear multiple times");
	}
	if (NULL == (ctx->bkgd = calloc(1, sizeof(struct bKGD)))) {
		err(EXIT_FAILURE, "calloc");
	}
	switch (ctx->ihdr->colourtype) {
	case COLOUR_TYPE_GREYSCALE:
		/* FALLTHROUGH */
	case COLOUR_TYPE_GREYSCALE_ALPHA:
		if (2 != dataz) {
			errx(EXIT_FAILURE, "bKGD: Invalid chunk size");
		}
		tmp = (uint16_t *)data;
		ctx->bkgd->greyscale = *tmp;
		ctx->bkgd->greyscale = ntohs(ctx->bkgd->greyscale);
		break;
	case COLOUR_TYPE_TRUECOLOUR:
		/* FALLTHROUGH */
	case COLOUR_TYPE_TRUECOLOUR_ALPHA:
		if (6 != dataz) {
			errx(EXIT_FAILURE, "bKGD: Invalid chunk size");
		}
		rgb = (struct rgb16 *)data;
		rgb->red = htons(rgb->red);
		rgb->green = htons(rgb->green);
		rgb->blue = htons(rgb->blue);
		ctx->bkgd->rgb.red = rgb->red;
		ctx->bkgd->rgb.green = rgb->green;
		ctx->bkgd->rgb.blue = rgb->blue;
		break;
	case COLOUR_TYPE_INDEXED:
		if (1 != dataz) {
			errx(EXIT_FAILURE, "bKGD: Invalid chunk size");
		}
		if (data[0] >= ctx->plte->entries) {
			errx(EXIT_FAILURE, "bKGD: Colour not in PLTE");
		}
		ctx->bkgd->paletteindex = data[0];
		break;
	}
}

void
lgpng_parse_hIST(struct lgpng *ctx, uint8_t *data, size_t dataz)
{
	uint16_t	*frequency;
	size_t		 elemz;

	if (NULL == ctx->plte) {
		errx(EXIT_FAILURE, "hIST: PLTE chunk is missing");
	}
	if (NULL != ctx->hist) {
		errx(EXIT_FAILURE, "hIST: hIST can not appear multiple times");
	}
	elemz = dataz / 2;
	if (0 != dataz % 2 || 256 < elemz) {
		errx(EXIT_FAILURE, "hIST: Invalid chunk size");
	}
	if (elemz != ctx->plte->entries) {
		errx(EXIT_FAILURE, "hIST: Number of entries differ from PLTE");
	}
	frequency = (uint16_t *)data;
	if (NULL == (ctx->hist = calloc(1, sizeof(struct hIST)))) {
		err(EXIT_FAILURE, "calloc");
	}
	for (size_t i = 0; i < elemz; i++) {
		ctx->hist->frequency[i] = frequency[i];
	}
}

void
lgpng_parse_pHYs(struct lgpng *ctx, uint8_t *data, size_t dataz)
{
	struct pHYs	*phys;

	if (NULL != ctx->phys) {
		errx(EXIT_FAILURE, "pHYs: pHYs can not appear multiple times");
	}
	if (sizeof(struct pHYs) != dataz) {
		errx(EXIT_FAILURE, "pHYs: invalid chunk size");
	}
	phys = (struct pHYs *)data;
	if (NULL == (ctx->phys = calloc(1, sizeof(struct pHYs)))) {
		err(EXIT_FAILURE, "calloc");
	}
	ctx->phys->ppux = ntohl(phys->ppux);
	ctx->phys->ppuy = ntohl(phys->ppuy);
	ctx->phys->unitspecifier = phys->unitspecifier;
}

void
lgpng_parse_sPLT(struct lgpng *ctx, uint8_t *data, size_t dataz)
{
	struct sPLT	  splt;
	struct sPLT	**spltpp;
	size_t		  palettenamez;
	int		  offset;

	/* XXX: look for a null char in the first 80 chars */
	splt.palettename = (char *)data;
	palettenamez = strlen(splt.palettename);
	if (palettenamez > 80) {
		errx(EXIT_FAILURE, "sPLT: Invalid palette name size");
	}
	offset = palettenamez + 1;
	splt.sampledepth = data[offset];
	if (8 != splt.sampledepth && 16 != splt.sampledepth) {
		errx(EXIT_FAILURE, "sPLT: Invalid sample depth");
	}
	offset += 1;
	if (8 == splt.sampledepth) {
		splt.entries = (dataz - offset) / 6;
		if ((dataz - offset) % 6 != 0) {
			errx(EXIT_FAILURE, "sPLT: Invalid palette size -- %zu",
			    splt.entries);
		}
	}
	if (16 == splt.sampledepth) {
		splt.entries = (dataz - offset) / 10;
		if ((dataz - offset) % 10 != 0) {
			errx(EXIT_FAILURE, "sPLT: Invalid palette size -- %zu",
			    splt.entries);
		}
	}

	spltpp = reallocarray(ctx->splt, ctx->spltz + 1, sizeof(struct sPLT *));
	if (NULL == spltpp) {
		free(ctx->splt);
		ctx->splt = NULL;
		ctx->spltz = 0;
		err(EXIT_FAILURE, "reallocarray");
	}
	ctx->splt = spltpp;
	if (NULL == (ctx->splt[ctx->spltz] = calloc(1, sizeof(struct sPLT)))) {
		err(EXIT_FAILURE, "calloc");
	}
	ctx->splt[ctx->spltz]->palettename = splt.palettename;
	ctx->splt[ctx->spltz]->sampledepth = splt.sampledepth;
	ctx->splt[ctx->spltz]->entries = splt.entries;

	splt.entry = calloc(splt.entries, sizeof(struct splt_entry));
	for (size_t i = 0; i < splt.entries; i++) {
		if (8 == splt.sampledepth) {
			memcpy(splt.entry[i].raw, data + offset, 6);
			offset += 6;
			splt.entry[i].depth8.frequency =
			    ntohs(splt.entry[i].depth8.frequency);
		} else {
			memcpy(splt.entry[i].raw, data + offset, 10);
			offset += 10;
			splt.entry[i].depth16.red =
			    ntohs(splt.entry[i].depth16.red);
			splt.entry[i].depth16.green =
			    ntohs(splt.entry[i].depth16.green);
			splt.entry[i].depth16.blue =
			    ntohs(splt.entry[i].depth16.blue);
			splt.entry[i].depth16.alpha =
			    ntohs(splt.entry[i].depth16.alpha);
			splt.entry[i].depth16.frequency =
			    ntohs(splt.entry[i].depth16.frequency);
		}
	}

	ctx->splt[ctx->spltz]->entry = splt.entry;
	ctx->spltz = ctx->spltz + 1;
}

void
lgpng_parse_tIME(struct lgpng *ctx, uint8_t *data, size_t dataz)
{
	struct tIME	*time;

	if (NULL != ctx->time) {
		errx(EXIT_FAILURE, "tIME: tIME can not appear multiple times");
	}
	if (sizeof(struct tIME) != dataz) {
		errx(EXIT_FAILURE, "tIME: invalid chunk size");
	}
	time = (struct tIME *)data;
	time->year = htons(time->year);
	if (NULL == (ctx->time = calloc(1, sizeof(struct tIME)))) {
		err(EXIT_FAILURE, "calloc");
	}
	ctx->time->year = time->year;
	ctx->time->month = time->month;
	ctx->time->day = time->day;
	ctx->time->hour = time->hour;
	ctx->time->minute = time->minute;
	ctx->time->second = time->second;
}

size_t
write_png_sig(uint8_t *buf)
{
	char	sig[8] = {137, 80, 78, 71, 13, 10, 26, 10};

	(void)memcpy(buf, sig, sizeof(sig));
	return(sizeof(sig));
}

size_t
write_IHDR(uint8_t *buf, size_t width, int bitdepth, enum colourtype colour)
{
	uint32_t	crc, length;
	size_t		bufw;
	uint8_t		type[4] = "IHDR";
	struct IHDR	ihdr;

	ihdr.width = htonl(width);
	ihdr.height = htonl(width);
	ihdr.bitdepth = bitdepth;
	ihdr.colourtype = colour;
	ihdr.compression = 0;
	ihdr.filter = 0;
	ihdr.interlace = INTERLACE_METHOD_STANDARD;
	length = htonl(13);
	crc = crc32(0, Z_NULL, 0);
	crc = crc32(crc, type, sizeof(type));
	crc = crc32(crc, (Bytef *)&ihdr, sizeof(ihdr));
	crc = htonl(crc);
	bufw = 0;
	(void)memcpy(buf + bufw, &length, sizeof(length));
	bufw += sizeof(length);
	(void)memcpy(buf + bufw, type, sizeof(type));
	bufw += sizeof(type);
	(void)memcpy(buf + bufw, &ihdr, sizeof(ihdr));
	bufw += sizeof(ihdr);
	(void)memcpy(buf + bufw, &crc, sizeof(crc));
	bufw += sizeof(crc);
	return(bufw);
}

size_t
write_IEND(uint8_t *buf)
{
	int		length;
	uint32_t	crc;
	size_t		bufw;
	uint8_t		type[4] = "IEND";

	length = 0;
	crc = htonl(2923585666);
	bufw = 0;
	(void)memcpy(buf + bufw, &length, sizeof(length));
	bufw += sizeof(length);
	(void)memcpy(buf + bufw, type, sizeof(type));
	bufw += sizeof(type);
	(void)memcpy(buf + bufw, &crc, sizeof(crc));
	bufw += sizeof(crc);
	return(bufw);
}

