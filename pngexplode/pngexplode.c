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
	int		 nchunk = 0;
	long		 offset;
	char		 output_file_name[25];
	char		*source_name = "stdin";
	FILE		*source = stdin, *output = NULL;

#if HAVE_PLEDGE
	pledge("stdio rpath", NULL);
#endif
	while (-1 != (ch = getopt(argc, argv, "f:s")))
		switch (ch) {
		case 'f':
			if (NULL == (source = fopen(optarg, "r"))) {
				err(EXIT_FAILURE, "%s", optarg);
			}
			source_name = optarg;
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
		enum chunktype		 chunktype;
		fpos_t			 initial_pos;
		size_t			 full_length;
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
			warnx("Can't get next chunk from stream %s", source_name);
			loopexit = true;
			goto stop;
		}
		full_length = 4 + 4 + unknown_chunk.length + 4;
		chunktype = lgpng_identify_chunk(&unknown_chunk);
		nchunk += 1;

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
		(void)memset(output_file_name, 0, sizeof(output_file_name));
		(void)snprintf(output_file_name, sizeof(output_file_name),
		    "__pure_%03d_%s.dat", nchunk, unknown_chunk.type);
		if (NULL == (output = fopen(output_file_name, "w"))) {
			warn("%s", output_file_name);
			loopexit = true;
			goto stop;
		}
		(void)fwrite(raw_chunk, full_length, 1, output);
		(void)fclose(output);
		if (CHUNK_TYPE_IEND == chunktype) {
			loopexit = true;
		}
stop:
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

