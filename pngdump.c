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
#if HAVE_ERR
# include <err.h>
#endif
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
	uint8_t	 	 oflag = 0;
	long		 offset;
	bool		 dflag = false, sflag = false;
	bool		 loopexit = false;
	const char	*errstr = NULL;
	FILE		*source = stdin;

#if HAVE_PLEDGE
	pledge("stdio rpath", NULL);
#endif
	while (-1 != (ch = getopt(argc, argv, "df:o:s")))
		switch (ch) {
		case 'd':
			dflag = true;
			break;
		case 'f':
			if (NULL == (source = fopen(optarg, "r"))) {
				err(EXIT_FAILURE, "%s", optarg);
			}
			break;
		case 'o':
			if (0 == (oflag = (uint8_t)strtonum(optarg, 1, 255, &errstr))) {
				fprintf(stderr, "value is %s -- b\n", errstr);
				return(EXIT_FAILURE);
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

	if (oflag != 0 && !dflag) {
		fclose(source);
		warnx("%s: -o can only be used with -d", getprogname());
		usage();
	}

	if (argc != 1) {
		fclose(source);
		warnx("%s: chunk parameter is mandatory", getprogname());
		usage();
	}

	/* Read the file byte by byte until the PNG signature is found */
	offset = 0;
	if (false == sflag) {
		if (LGPNG_OK != lgpng_stream_is_png(source)) {
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
		} while (LGPNG_OK != lgpng_stream_is_png(source));
	}

	do {
		uint32_t	 length = 0, crc = 0;
		uint8_t		*data = NULL;
		uint8_t		 type[4] = {0, 0, 0, 0};

		if (LGPNG_OK != lgpng_stream_get_length(source, &length)) {
			break;
		}
		if (LGPNG_OK != lgpng_stream_get_type(source, type)) {
			break;
		}
		if (NULL == (data = malloc(length + 1))) {
			fprintf(stderr, "malloc\n");
			break;
		}
		if (LGPNG_OK != lgpng_stream_get_data(source, length, &data)) {
			goto stop;
		}
		if (LGPNG_OK != lgpng_stream_get_crc(source, &crc)) {
			goto stop;
		}
		/* Ignore invalid CRC */
		if (0 == memcmp(type, argv[0], 4)) {
			if (dflag) {
				(void)fwrite(data + oflag, 1, length - oflag, stdout);
			} else {
				(void)lgpng_stream_write_chunk(stdout, length,
				   type, data, crc);
			}
			loopexit = true;
		}
stop:
		if (0 == memcmp(type, "IEND", 4)) {
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
	fprintf(stderr, "usage: %s [-s] [-f file] [-d [-o offset]] chunk\n",
	    getprogname());
	exit(EXIT_FAILURE);
}

