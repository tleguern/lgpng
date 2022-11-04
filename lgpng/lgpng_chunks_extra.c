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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "lgpng.h"

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
	canv->data.width = ntohl(canv->data.width);
	canv->data.height = ntohl(canv->data.height);
	canv->data.x_position = ntohl(canv->data.x_position);
	canv->data.y_position = ntohl(canv->data.y_position);
	return(0);
}
