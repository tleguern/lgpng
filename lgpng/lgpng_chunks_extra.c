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

