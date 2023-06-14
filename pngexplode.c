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
	bool		 loopexit = false;
	bool		 sflag = false;
	int		 ch;
	int		 nchunk = 0;
	long		 offset;
	char		 output_file_name[25];
	FILE		*source = stdin, *output = NULL;

#if HAVE_PLEDGE
	pledge("stdio wpath rpath cpath", NULL);
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

	/* Write the PNG magic bytes in a file */
	(void)memset(output_file_name, 0, sizeof(output_file_name));
	(void)snprintf(output_file_name, sizeof(output_file_name),
	    "__pure_000_sig.dat");
	if (NULL == (output = fopen(output_file_name, "w"))) {
		err(EXIT_FAILURE, "%s", output_file_name);
	}
	(void)fwrite(png_sig, sizeof(png_sig), 1, output);
	(void)fclose(output);

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
			fprintf(stderr, "malloc(length + 1)\n");
			break;
		}
		if (LGPNG_OK != lgpng_stream_get_data(source, length, &data)) {
			goto stop;
		}
		if (LGPNG_OK != lgpng_stream_get_crc(source, &crc)) {
			goto stop;
		}
		/* Ignore invalid CRC */

		/* Write the raw chunk in an individual file */
		nchunk += 1;
		(void)memset(output_file_name, 0, sizeof(output_file_name));
		(void)snprintf(output_file_name, sizeof(output_file_name),
		    "__pure_%03d_%.4s.dat", nchunk, type);
		if (NULL == (output = fopen(output_file_name, "w"))) {
			warn("%s", output_file_name);
			fclose(source);
			free(data);
			return(EXIT_FAILURE);
		}
		(void)lgpng_stream_write_chunk(output, length, type, data, crc);
		(void)fclose(output);
		if (0 == memcmp(type, "IEND", 4)) {
			loopexit = true;
		}
stop:
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

