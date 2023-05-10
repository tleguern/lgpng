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

#include "config.h"

#include <ctype.h>
#include COMPAT_ENDIAN_H
#include <stdlib.h>
#include <string.h>

#include "lgpng.h"

enum lgpng_err
lgpng_data_is_png(uint8_t *src, size_t srcz)
{
	if (NULL == src) {
		return(LGPNG_INVALID_PARAM);
	}
	if (srcz < 8) {
		return(LGPNG_TOO_SHORT);
	}
	if (memcmp(src, png_sig, sizeof(png_sig)) == 0)
		return(LGPNG_OK);
	return(LGPNG_ERROR);
}

enum lgpng_err
lgpng_data_get_length(uint8_t *src, size_t srcz, uint32_t *length)
{
	if (NULL == src || NULL == length) {
		return(LGPNG_INVALID_PARAM);
	}
	if (srcz < 4) {
		return(LGPNG_TOO_SHORT);
	}
	/* Read the first four bytes to gather the length of the data part */
	(void)memcpy(length, src, 4);
	*length = be32toh(*length);
	if (*length > INT32_MAX) {
		return(LGPNG_INVALID_CHUNK_LENGTH);
	}
	return(LGPNG_OK);
}

enum lgpng_err
lgpng_data_get_type(uint8_t *src, size_t srcz, uint8_t name[4])
{
	uint8_t		type[4];
	unsigned int	err = LGPNG_OK;

	if (NULL == src) {
		return(LGPNG_INVALID_PARAM);
	}
	if (srcz < 4) {
		return(LGPNG_TOO_SHORT);
	}

	(void)memcpy(type, src, 4);
	for (size_t i = 0; i < 4; i++) {
		/* Copy the chunk name even if invalid */
		name[i] = type[i];
		if (isalpha(type[i]) == 0) {
			err = LGPNG_INVALID_CHUNK_NAME;
		}
	}
	return(err);
}

enum lgpng_err
lgpng_data_get_data(uint8_t *src, size_t srcz, uint32_t length, uint8_t **data)
{
	if (NULL == src || NULL == data) {
		return(LGPNG_INVALID_PARAM);
	}
	if (srcz < length) {
		return(LGPNG_TOO_SHORT);
	}

	if (0 != length) {
		(void)memcpy((*data), src, length);
		(*data)[length] = '\0';
	}
	return(LGPNG_OK);
}

enum lgpng_err
lgpng_data_get_crc(uint8_t *src, size_t srcz, uint32_t *crc)
{
	if (NULL == src || NULL == crc) {
		return(LGPNG_INVALID_PARAM);
	}
	if (srcz < 4) {
		return(LGPNG_TOO_SHORT);
	}

	(void)memcpy(crc, src, 4);
	*crc = be32toh(*crc);
	return(LGPNG_OK);
}

size_t
lgpng_data_write_sig(uint8_t *dest)
{
	(void)memcpy(dest, png_sig, sizeof(png_sig));
	return(sizeof(png_sig));
}

size_t
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

