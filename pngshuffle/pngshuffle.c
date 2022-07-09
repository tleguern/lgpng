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

#include <arpa/inet.h>

#include <ctype.h>
#include <err.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "lgpng.h"

void usage(void);

int
main(int argc, char *argv[])
{
	bool		 loopexit = false;
	bool		 sflag = false;
	int		 ch;
	long		 offset;
	FILE		*source = stdin;

#if HAVE_PLEDGE
	pledge("stdio rpath", NULL);
#endif
	while (-1 != (ch = getopt(argc, argv, "f:s")))
		switch (ch) {
		case 'f':
			if (NULL == (source = fopen(optarg, "r"))) {
				err(EXIT_FAILURE, "%s", optarg);
			}
			break;
		case 's':
			sflag = true;
			break;
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	/* Read the file byte by byte until the PNG signature is found */
	offset = 0;
	if (false == sflag) {
		if (false == lgpng_stream_is_png(source)) {
			errx(EXIT_FAILURE, "not a PNG file");
		}
	} else {
		do {
			if (0 != feof(source) || 0 != ferror(source)) {
				errx(EXIT_FAILURE, "not a PNG file");
			}
			if (0 != fseek(source, offset, SEEK_SET)) {
				errx(EXIT_FAILURE, "not a PNG file");
			}
			offset += 1;
		} while (false == lgpng_stream_is_png(source));
	}

#if HAVE_ARC4RANDOM
	srandom(0);
#endif
	/* Write the PNG magic bytes */
	(void)fwrite(png_sig, sizeof(png_sig), 1, stdout);
	do {
		int		 chunktype = CHUNK_TYPE__MAX;
		uint32_t	 length = 0, crc = 0;
		uint8_t		*data = NULL;
		uint8_t		 str_type[5] = {0, 0, 0, 0, 0};
		struct PLTE	 plte;
		uint32_t	 nlength, ncrc;

		if (false == lgpng_stream_get_length(source, &length)) {
			break;
		}
		if (false == lgpng_stream_get_type(source, &chunktype,
		    (uint8_t *)str_type)) {
			break;
		}
		if (NULL == (data = malloc(length + 1))) {
			fprintf(stderr, "malloc(length + 1)\n");
			break;
		}
		if (false == lgpng_stream_get_data(source, length, &data)) {
			goto stop;
		}
		if (false == lgpng_stream_get_crc(source, &crc)) {
			goto stop;
		}

		nlength = htonl(length);
		/* If it is PLTE shuffle it, otherwise just write it */
		if (CHUNK_TYPE_PLTE == chunktype) {
			int		permutations;

			if (-1 == lgpng_create_PLTE_from_data(&plte, data, length)) {
				warnx("PLTE: Invalid PLTE chunk");
				loopexit = true;
				goto stop;
			}
			permutations = length / 2;
			for (int i = 0; i < permutations; i++) {
				uint8_t		r, g, b;
				uint32_t	src, dest;

#if HAVE_ARC4RANDOM
				src = arc4random_uniform(length - 3);
				dest = arc4random_uniform(length - 3);
#else
				src = random() % (length - 3);
				dest = random() % (length - 3);
#endif
				if (src == dest) {
					continue;
				}
				r = data[dest];
				g = data[dest];
				b = data[dest];
				data[dest] = data[src];
				data[dest] = data[src];
				data[dest] = data[src];
				data[src] = r;
				data[src] = g;
				data[src] = b;
			}
			lgpng_chunk_crc(length, str_type, data, &ncrc);
		}
		ncrc = htonl(crc);
		(void)lgpng_stream_write_chunk(stdout, nlength, str_type,
		    data, ncrc);
stop:
		if (CHUNK_TYPE_IEND == chunktype) {
			loopexit = true;
		}
		free(data);
		data = NULL;
	} while(! loopexit);
	(void)fclose(source);
	return(EXIT_SUCCESS);
}

void
usage(void)
{
	fprintf(stderr, "usage: %s [-s] [-f file]\n", getprogname());
	exit(EXIT_FAILURE);
}

