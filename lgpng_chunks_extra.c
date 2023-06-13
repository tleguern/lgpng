/*
 * Copyright (c) 2022-2023 Tristan Le Guern <tleguern@bouledef.eu>
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

#include COMPAT_ENDIAN_H
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
	vpag->length = dataz;
	(void)memcpy(&(vpag->type), "vpAg", 4);
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
	canv->length = dataz;
	(void)memcpy(&(canv->type), "caNv", 4);
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
	ornt->length = dataz;
	(void)memcpy(&(ornt->type), "orNt", 4);
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
	(void)memcpy(&(skmf->type), "skMf", 4);
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
	(void)memcpy(&(skrf->type), "skRf", 4);
	(void)memcpy(&(skrf->data.header), data, 16);
	skrf->data.data = data + 16;
	return(0);
}

const char *walv_soil_textures_map[WALV_SOIL__MAX] = {
	"art",
	"cheese",
	"classic beach",
	"classic desert",
	"classic farm",
	"classic forest",
	"classic hell",
	"construction",
	"desert",
	"dungeon",
	"easter",
	"forest",
	"fruit",
	"gulf",
	"hell",
	"hospital",
	"jungle",
	"manhattan",
	"medieval",
	"music",
	"pirate",
	"snow",
	"space",
	"sports",
	"tentacle",
	"time",
	"tools",
	"tribal",
	"urban",
};

int
lgpng_create_waLV_from_data(struct waLV *walv, uint8_t *data, size_t dataz)
{
	if (dataz < 40) {
		return(-1);
	}
	walv->length = dataz;
	(void)memcpy(&(walv->type), "waLV", 4);
	(void)memcpy(&(walv->data.land_seed), data, 4);
	(void)memcpy(&(walv->data.object_seed), data + 4, 4);
	(void)memcpy(&(walv->data.cavern), data + 8, 4);
	(void)memcpy(&(walv->data.style), data + 12, 4);
	(void)memcpy(&(walv->data.borders), data + 16, 4);
	(void)memcpy(&(walv->data.object_percent), data + 20, 4);
	(void)memcpy(&(walv->data.bridge_percent), data + 24, 4);
	(void)memcpy(&(walv->data.water_level), data + 28, 4);
	(void)memcpy(&(walv->data.soil_texture_idx), data + 32, 4);
	(void)memcpy(&(walv->data.water_colour), data + 36, 4);
	walv->data.worm_places = data[40];
	/* In this chunks data is not stored in big-endian */
	return(0);
}

int
lgpng_create_msOG_from_data(struct msOG *msog, uint8_t *data, size_t dataz)
{
	if (dataz < 11) {
		return(-1);
	}
	msog->length = dataz;
	(void)memcpy(&(msog->type), "msOG", 4);
	(void)memcpy(&(msog->data.header), data, 11);
	msog->data.gifz = dataz - 11;
	msog->data.ptr = data + 11;
	return(0);
}

int
lgpng_create_tpNG_from_data(struct tpNG *tpng, uint8_t *data, size_t dataz)
{
	if (dataz != 8) {
		return(-1);
	}
	tpng->length = dataz;
	(void)memcpy(&(tpng->type), "tpNG", 4);
	(void)memcpy(&(tpng->data.version), data, 4);
	tpng->data.password = data[4];
	tpng->data.alpha256 = data[5];
	(void)memcpy(&(tpng->data.unused), data + 4, 2);
	if (tpng->data.password != 0 && tpng->data.password != 1) {
		return(-1);
	}
	if (tpng->data.alpha256 != 0 && tpng->data.alpha256 != 1) {
		return(-1);
	}
	return(0);
}

