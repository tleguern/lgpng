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

