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

#if HAVE_ERR
# include <err.h>
#endif
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

int
lgpng_parse_IHDR_data(struct IHDR *ihdr, uint8_t *data)
{
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

void
lgpng_init_IHDR(struct IHDR *ihdr, uint32_t crc)
{
	ihdr->length = 13;
	ihdr->type = CHUNK_TYPE_IHDR;
	ihdr->data.width = 0;
	ihdr->data.height = 0;
	ihdr->data.bitdepth = 8;
	ihdr->data.colourtype = COLOUR_TYPE_TRUECOLOUR;
	ihdr->data.compression = COMPRESSION_TYPE_DEFLATE;
	ihdr->data.filter = FILTER_METHOD_ADAPTIVE;
	ihdr->data.interlace = INTERLACE_METHOD_STANDARD;
	ihdr->crc = crc;
}
