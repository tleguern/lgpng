#include <arpa/inet.h>

#include <ctype.h>
#include <err.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lgpng.h"

void
print_position(FILE *file)
{
	fpos_t		 pos = 0;

	(void)fgetpos(file, &pos);
	fprintf(stderr, "Position is %lld\n", pos);
}

bool
is_png(FILE *f)
{
	char sig[8] = {  0,  0,  0,  0,  0,  0,  0,  0};
	char png[8] = {137, 80, 78, 71, 13, 10, 26, 10};

	fread(sig, 1, sizeof(sig), f);
	if (memcmp(sig, png, sizeof(sig)) == 0)
		return(true);
	return(false);
}

enum chunktype
lgpng_identify_chunk(struct unknown_chunk *chunk)
{
	for (int i = 0; i < CHUNK_TYPE__MAX; i++) {
		if (strncmp(chunk->type, chunktypemap[i], 4) == 0) {
			return(i);
		}
	}
	return(-1);
}

int
lgpng_get_next_chunk_from_stream(FILE *src, struct unknown_chunk *dst, uint8_t **data)
{
	size_t	i;

	/* Read the first four bytes to gather the length of the data part */
	if (1 != fread(&(dst->length), 4, 1, src)) {
		fprintf(stderr, "Not enough data to read chunk's length\n");
		return(-1);
	}
	dst->length = ntohl(dst->length);
	if (dst->length > INT32_MAX) {
		fprintf(stderr, "Chunk length is too big (%d)\n", dst->length);
		return(-1);
	}
	/* Read the chunk type */
	if (4 != fread(&(dst->type), 1, 4, src)) {
		fprintf(stderr, "Not enough data to read chunk's type\n");
		return(-1);
	}
	dst->type[4] = '\0';
	for (i = 0; i < 4; i++) {
		if (isalpha(dst->type[i]) == 0) {
			fprintf(stderr, "Invalid chunk type\n");
			return(-1);
		}
	}
	/* Read the chunk data */
	if (0 != dst->length) {
		if (NULL == ((*data) = malloc(dst->length + 1))) {
			fprintf(stderr, "malloc(dst->length)\n");
			return(-1);
		}
		if (dst->length != fread((*data), 1, dst->length, src)) {
			fprintf(stderr, "Not enough data to read chunk's data\n");
			free(*data);
			(*data) = NULL;
			return(-1);
		}
		(*data)[dst->length] = '\0';
	}
	/* Read the CRC */
	if (1 != fread(&(dst->crc), 4, 1, src)) {
		fprintf(stderr, "Not enough data to read chunk's CRC\n");
		free(*data);
		(*data) = NULL;
		return(-1);
	}
	dst->crc = ntohl(dst->crc);
	return(0);
}

int
main(int argc, char *argv[])
{
	bool	 loopexit = false;
	FILE	*file = stdin;

#if HAVE_PLEDGE
	pledge("stdio rpath", NULL);
#endif

	if (argc == 2) {
		if (NULL == (file = fopen(argv[1], "r"))) {
			err(EXIT_FAILURE, "%s", argv[1]);
		}
	}
	if (! is_png(file)) {
		fclose(file);
		err(EXIT_FAILURE, "%s is not a PNG file", argv[1]);
	}

	do {
		int			 chunktype;
		uint8_t			*data = NULL;
		struct unknown_chunk	 unknown_chunk;

		if (-1 == lgpng_get_next_chunk_from_stream(file, &unknown_chunk, &data)) {
			break;
		}
		if (-1 == (chunktype = lgpng_identify_chunk(&unknown_chunk))) {
			fprintf(stderr, "Unknown chunk, skipping\n");
			free(data);
			continue;
		}
		switch (chunktype) {
		case CHUNK_TYPE_IHDR: {
			struct IHDR ihdr;

			lgpng_init_IHDR(&ihdr, unknown_chunk.crc);
			lgpng_parse_IHDR_data(&ihdr, data);
			fprintf(stderr, "ihdr.data.width: %u\n", ihdr.data.width);
			fprintf(stderr, "ihdr.data.height: %u\n", ihdr.data.height);
		}
		break;
		case CHUNK_TYPE_IDAT:
			fprintf(stderr, "IDAT\n");
			break;
		case CHUNK_TYPE_IEND:
			fprintf(stderr, "IEND\n");
			break;
		case CHUNK_TYPE__MAX:
			/* FALLTHROUGH */
		default:
			fprintf(stderr, "WTF?\n");
			break;
		}
		if (CHUNK_TYPE_IEND == chunktype) {
			loopexit = true;
		}
		free(data);
	} while(! loopexit);
	fclose(file);
	return(EXIT_SUCCESS);
}
