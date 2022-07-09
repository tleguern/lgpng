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

