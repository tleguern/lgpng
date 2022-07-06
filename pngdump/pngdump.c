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
	int		 ch;
	long		 offset;
	bool		 sflag = false;
	bool		 loopexit = false;
	enum chunktype	 chunk = CHUNK_TYPE__MAX;
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

	if (argc != 1) {
		fclose(source);
		warnx("%s: chunk parameter is mandatory", getprogname());
		usage();
	}
	for (int i = 0; i < CHUNK_TYPE__MAX; i++) {
		if (strcmp(argv[0], chunktypemap[i]) == 0) {
			chunk = i;
			break;
		}
	}
	/* TODO: Allow arbitrary chunk ? */
	if (CHUNK_TYPE__MAX == chunk) {
		errx(EXIT_FAILURE, "%s: invalid chunk", argv[0]);
	}
	

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

	do {
		fpos_t			 initial_pos;
		enum chunktype		 chunktype;
		struct unknown_chunk	 unknown_chunk;
		uint8_t			*data = NULL;
		uint8_t			*raw_chunk = NULL;

		/* Save the stream position for later */
		if (0 != fgetpos(source, &initial_pos)) {
			warn(NULL);
			loopexit = true;
			goto stop;
		}
		if (-1 == lgpng_get_next_chunk_from_stream(source, &unknown_chunk, &data)) {
			break;
		}
		chunktype = lgpng_identify_chunk(&unknown_chunk);
		if (chunktype == chunk) {
			size_t full_length = 4 + 4 + unknown_chunk.length + 4;

			/* Rewind the stream at the begining of the chunk */
			if (0 != fsetpos(source, &initial_pos)) {
				warn(NULL);
				loopexit = true;
				goto stop;
			}
			if (NULL == (raw_chunk = malloc(full_length))) {
				fclose(source);
				free(data);
				err(EXIT_FAILURE, "malloc(%zu)", full_length);
			}
			/* Read the raw chunk */
			if (full_length != fread(raw_chunk, 1, full_length, source)) {
				fclose(source);
				free(data);
				free(raw_chunk);
				errx(EXIT_FAILURE, "Truncated chunk?");
			}
			(void)fwrite(raw_chunk, full_length, 1, stdout);
			free(raw_chunk);
			loopexit = true;
		}
		if (CHUNK_TYPE_IEND == chunktype) {
			loopexit = true;
		}
stop:
		free(data);
	} while(! loopexit);
	fclose(source);
	return(EXIT_SUCCESS);
}

void
usage(void)
{
	fprintf(stderr, "usage: %s [-s] [-f file] chunk\n", getprogname());
	exit(EXIT_FAILURE);
}

