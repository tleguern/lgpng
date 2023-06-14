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
lgpng_stream_is_png(FILE *src)
{
	char sig[8] = {  0,  0,  0,  0,  0,  0,  0,  0};

	if (NULL == src) {
		return(LGPNG_INVALID_PARAM);
	}
	if (sizeof(sig) != fread(sig, 1, sizeof(sig), src)) {
		return(LGPNG_TOO_SHORT);
	}
	if (memcmp(sig, png_sig, sizeof(sig)) == 0)
		return(LGPNG_OK);
	return(LGPNG_ERROR);
}

enum lgpng_err
lgpng_stream_get_length(FILE *src, uint32_t *length)
{
	if (NULL == src || NULL == length) {
		return(LGPNG_INVALID_PARAM);
	}
	/* Read the first four bytes to gather the length of the data part */
	if (1 != fread(length, 4, 1, src)) {
		return(LGPNG_TOO_SHORT);
	}
	*length = be32toh(*length);
	if (*length > INT32_MAX) {
		return(LGPNG_INVALID_CHUNK_LENGTH);
	}
	return(LGPNG_OK);
}

enum lgpng_err
lgpng_stream_get_type(FILE *src, uint8_t name[4])
{
	uint8_t	type[4];
	int	err = LGPNG_OK;

	if (NULL == src) {
		return(LGPNG_INVALID_PARAM);
	}
	if (4 != fread(type, 1, 4, src)) {
		return(LGPNG_TOO_SHORT);
	}
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
lgpng_stream_get_data(FILE *src, uint32_t length, uint8_t **data)
{
	if (NULL == src || NULL == data) {
		return(LGPNG_INVALID_PARAM);
	}
	if (0 != length) {
		if (length != fread(*data, 1, length, src)) {
			return(LGPNG_TOO_SHORT);
		}
		(*data)[length] = '\0';
	}
	return(LGPNG_OK);
}

enum lgpng_err
lgpng_stream_skip_data(FILE *src, uint32_t length)
{
	if (NULL == src) {
		return(LGPNG_INVALID_PARAM);
	}
	if (0 != length) {
		if (-1 == fseek(src, length, SEEK_CUR)) {
			return(LGPNG_TOO_SHORT);
		}
	}
	return(LGPNG_OK);
}

enum lgpng_err
lgpng_stream_get_crc(FILE *src, uint32_t *crc)
{
	if (NULL == src || NULL == crc) {
		return(LGPNG_INVALID_PARAM);
	}
	if (1 != fread(crc, 4, 1, src)) {
		return(LGPNG_TOO_SHORT);
	}
	*crc = be32toh(*crc);
	return(LGPNG_OK);
}

enum lgpng_err
lgpng_stream_write_sig(FILE *output)
{
	if (NULL == output) {
		return(LGPNG_INVALID_PARAM);
	}
	if (8 != fwrite(png_sig, 1, 8, output)) {
		return(LGPNG_ERROR);
	}
	return(LGPNG_OK);
}

enum lgpng_err
lgpng_stream_write_chunk(FILE *output, uint32_t length, uint8_t type[4],
    uint8_t *data, uint32_t crc)
{
	uint32_t nlength = htobe32(length);
	uint32_t ncrc = htobe32(crc);

	if (NULL == output) {
		return(LGPNG_INVALID_PARAM);
	}
	if (4 != fwrite((uint8_t *)&nlength, 1, 4, output)) {
		return(LGPNG_ERROR);
	}
	if (4 != fwrite(type, 1, 4, output)) {
		return(LGPNG_ERROR);
	}
	if (length != fwrite(data, 1, length, output)) {
		return(LGPNG_ERROR);
	}
	if (4 != fwrite((uint8_t *)&ncrc, 1, 4, output)) {
		return(LGPNG_ERROR);
	}
	return(LGPNG_OK);
}

