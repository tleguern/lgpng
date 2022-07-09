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
	int	 ch;
	long	 offset;
	bool	 sflag = false;
	bool	 loopexit = false;
	int	 chunk = CHUNK_TYPE__MAX;
	FILE	*source = stdin;

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

	do {
		int		 chunktype = CHUNK_TYPE__MAX;
		uint32_t	 length = 0, crc = 0;
		uint8_t		*data = NULL;
		uint8_t		 str_type[5] = {0, 0, 0, 0, 0};

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
		/* Ignore invalid CRC */
		if (chunktype == chunk) {
			uint32_t nlength = htonl(length);
			uint32_t ncrc = htonl(crc);

			if (4 != fwrite((uint8_t *)&nlength, 1, 4, stdout)) {
				warnx("internal error: length is not four bytes long");
			}
			if (4 != fwrite(str_type, 1, 4, stdout)) {
				warnx("internal error: str_type is not four bytes long");
			}
			if (length != fwrite(data, 1, length, stdout)) {
				warnx("internal error: data is not %u bytes long", length);
			}
			if (4 != fwrite((uint8_t *)&ncrc, 1, 4, stdout)) {
				warnx("internal error: crc is not four bytes long");
			}
			loopexit = true;
		}
stop:
		if (CHUNK_TYPE_IEND == chunktype) {
			loopexit = true;
		}
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

