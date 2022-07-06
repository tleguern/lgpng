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
		if (false == lgpng_is_stream_png(source)) {
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
		} while (false == lgpng_is_stream_png(source));
	}

	/* Write the PNG magic bytes */
	(void)fwrite(png_sig, sizeof(png_sig), 1, stdout);
	do {
		enum chunktype		 chunktype = CHUNK_TYPE__MAX;
		fpos_t			 initial_pos;
		size_t			 full_length;
		struct unknown_chunk	 unknown_chunk;
		struct PLTE		 plte;
		uint8_t			*data = NULL;
		uint8_t			*raw_chunk = NULL;

		/* Save the stream position for later */
		if (0 != fgetpos(source, &initial_pos)) {
			warn(NULL);
			loopexit = true;
			goto stop;
		}
		if (-1 == lgpng_get_next_chunk_from_stream(source, &unknown_chunk, &data)) {
			warnx("Can't get next chunk from stream");
			loopexit = true;
			goto stop;
		}
		full_length = 4 + 4 + unknown_chunk.length + 4;
		chunktype = lgpng_identify_chunk(&unknown_chunk);

		if (CHUNK_TYPE_PLTE == chunktype) {
			int		permutations;
			uint32_t	length, crc;

			if (-1 == lgpng_create_PLTE_from_data(&plte, data, unknown_chunk.length)) {
				warnx("PLTE: Invalid PLTE chunk");
				loopexit = true;
				goto stop;
			}
			permutations = unknown_chunk.length / 2;
			for (int i = 0; i < permutations; i++) {
				uint8_t		r, g, b;
				uint32_t	src, dest;

				src = arc4random_uniform(unknown_chunk.length - 3);
				dest = arc4random_uniform(unknown_chunk.length- 3);
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
			crc = lgpng_crc_init();
			crc = lgpng_crc_update(crc, (uint8_t *)"PLTE", 4);
			crc = lgpng_crc_update(crc, data, unknown_chunk.length);
			crc = lgpng_crc_finalize(crc);
			crc = htonl(crc);
			length = htonl(unknown_chunk.length);
			(void)fwrite((uint8_t *)&length, 4, 1, stdout);
			(void)fwrite("PLTE", 4, 1, stdout);
			(void)fwrite(data, unknown_chunk.length, 1, stdout);
			(void)fwrite((uint8_t *)&crc, 4, 1, stdout);
		} else {
			/* Rewind the stream at the begining of the chunk */
			if (0 != fsetpos(source, &initial_pos)) {
				warn(NULL);
				loopexit = true;
				goto stop;
			}
			if (NULL == (raw_chunk = malloc(full_length))) {
				warn("malloc(%zu)", full_length);
				loopexit = true;
				goto stop;
			}
			/* Read the raw chunk */
			if (full_length != fread(raw_chunk, 1, full_length, source)) {
				warn("Truncated chunk?");
				loopexit = true;
				goto stop;
			}
			/* Write the raw chunk in an individual file */
			(void)fwrite(raw_chunk, full_length, 1, stdout);
		}
stop:
		if (CHUNK_TYPE_IEND == chunktype) {
			loopexit = true;
		}
		free(raw_chunk);
		raw_chunk = NULL;
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

