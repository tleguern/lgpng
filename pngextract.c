/*
 * Copyright (c) 2025 Tristan Le Guern <tleguern@bouledef.eu>
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
	bool		 loopexit = false;
	FILE		*source = stdin;

	while (-1 != (ch = getopt(argc, argv, "f:")))
		switch (ch) {
		case 'f':
			if (NULL == (source = fopen(optarg, "r"))) {
				err(EXIT_FAILURE, "%s", optarg);
			}
			break;
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

#if HAVE_PLEDGE
	if (source == stdin) {
		pledge("stdio", NULL);
	} else {
		pledge("stdio rpath", NULL);
	}
#endif

	/* First read the 8 first bytes for the happy case */
	uint8_t sig[8] = {  0,  0,  0,  0,  0,  0,  0,  0};
	if (sizeof(sig) != fread(sig, 1, sizeof(sig), source)) {
		fclose(source);
		errx(STDERR_FILENO, "input too small to be a PNG");
	}
	/* Read byte by byte until the signature is hopefully reached */
	while (LGPNG_OK != lgpng_data_is_png(sig, sizeof(sig))) {
		uint8_t c;

		if (1 != fread(&c, 1, 1, source)) {
			fclose(source);
			errx(STDERR_FILENO, "not a PNG");
		}
		sig[0] = sig[1];
		sig[1] = sig[2];
		sig[2] = sig[3];
		sig[3] = sig[4];
		sig[4] = sig[5];
		sig[5] = sig[6];
		sig[6] = sig[7];
		sig[7] = c;
	}

	/* Then dump the input on stdout without modifications until IEND */
	(void)lgpng_stream_write_sig(stdout);
	do {
		uint32_t	 length = 0, crc = 0;
		uint8_t		*data = NULL;
		uint8_t		 type[4] = {0, 0, 0, 0};

		if (LGPNG_OK != lgpng_stream_get_length(source, &length)) {
			break;
		}
		(void)lgpng_stream_write_integer(stdout, length);
		if (LGPNG_OK != lgpng_stream_get_type(source, type)) {
			break;
		}
		(void)fwrite(type, 1, 4, stdout);
		if (NULL == (data = malloc(length + 1))) {
			fprintf(stderr, "malloc\n");
			break;
		}
		if (LGPNG_OK != lgpng_stream_get_data(source, length, &data)) {
			loopexit = true;
			goto stop;
		}
		(void)fwrite(data, 1, length, stdout);
		if (LGPNG_OK != lgpng_stream_get_crc(source, &crc)) {
			loopexit = true;
			goto stop;
		}
		(void)lgpng_stream_write_integer(stdout, crc);
stop:
		free(data);
		if (0 == memcmp(type, "IEND", 4)) {
			loopexit = true;
		}
	} while(! loopexit);
	fclose(source);
	return(EXIT_SUCCESS);
}

void
usage(void)
{
	fprintf(stderr, "usage: %s [-f file]\n",
	    getprogname());
	exit(EXIT_FAILURE);
}

