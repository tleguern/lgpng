#include <err.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "lgpng.h"

int
main(void)
{
	int		 chunktype = CHUNK_TYPE__MAX, test = 0;
	uint32_t	 length = 0, crc = 0;
	uint8_t		 str_type[5] = {0, 0, 0, 0, 0};
	uint8_t		*data = NULL;
	FILE		*source = NULL;
	char		*subject, *status;

	printf("lgpng_stream tests\n");
	printf("TAP version 13\n");
	printf("1..17\n");

	if (NULL == (source = fopen("./blank.png", "r"))) {
		printf("Bail out!\n");
		err(EXIT_FAILURE, "%s", optarg);
	}

	/* PNG signature */
	subject = "%s %d - lgpng_stream_is_png with source NULL\n";
	if (false == lgpng_stream_is_png(NULL)) {
		status = "ok";
	} else {
		status = "not ok";
	}
	printf(subject, status, ++test);

	subject = "%s %d - lgpng_stream_is_png\n";
	if (false != lgpng_stream_is_png(source)) {
		status = "ok";
	} else {
		status = "not ok";
	}
	printf(subject, status, ++test);

	/* Length */
	subject = "%s %d - lgpng_stream_get_length with source NULL\n";
	if (false == lgpng_stream_get_length(NULL, &length)) {
		status = "ok";
	} else {
		status = "not ok";
	}
	printf(subject, status, ++test);

	subject = "%s %d - lgpng_stream_get_length with length NULL\n";
	if (false == lgpng_stream_get_length(source, NULL)) {
		status = "ok";
	} else {
		status = "not ok";
	}
	printf(subject, status, ++test);

	subject = "%s %d - lgpng_stream_get_length\n";
	if (false != lgpng_stream_get_length(source, &length)) {
		status = "ok";
	} else {
		status = "not ok";
	}
	printf(subject, status, ++test);

	subject = "%s %d - length should be 13\n";
	if (13 == length) {
		status = "ok";
	} else {
		status = "not ok";
	}
	printf(subject, status, ++test);

	/* Type */
	subject = "%s %d - lgpng_stream_get_type with source NULL\n";
	if (false == lgpng_stream_get_type(NULL, &chunktype,
	    (uint8_t *)str_type)) {
		status = "ok";
	} else {
		status = "not ok";
	}
	printf(subject, status, ++test);

	subject = "%s %d - lgpng_stream_get_type with type NULL\n";
	if (false == lgpng_stream_get_type(source, NULL,
	    (uint8_t *)str_type)) {
		status = "ok";
	} else {
		status = "not ok";
	}
	printf(subject, status, ++test);

	subject = "%s %d - lgpng_stream_get_type\n";
	if (false != lgpng_stream_get_type(source, &chunktype,
	    (uint8_t *)str_type)) {
		status = "ok";
	} else {
		status = "not ok";
	}
	printf(subject, status, ++test);

	subject = "%s %d - chunktype should be CHUNK_TYPE_IHDR\n";
	if (CHUNK_TYPE_IHDR == chunktype) {
		status = "ok";
	} else {
		status = "not ok";
	}
	printf(subject, status, ++test);

	subject = "%s %d - str_type should be IHDR\n";
	if (0 == strncmp((char *)str_type, "IHDR", 4)) {
		status = "ok";
	} else {
		status = "not ok";
	}
	printf(subject, status, ++test);

	if (NULL == (data = malloc(length + 1))) {
		fclose(source);
		printf("Bail out!\n");
		err(EXIT_FAILURE, "malloc(length + 1)");
	}

	/* Data */
	subject = "%s %d - lgpng_stream_get_data with source NULL\n";
	if (false == lgpng_stream_get_data(NULL, length, &data)) {
		status = "ok";
	} else {
		status = "not ok";
	}
	printf(subject, status, ++test);

	subject = "%s %d - lgpng_stream_get_data with data NULL\n";
	if (false == lgpng_stream_get_data(source, length, NULL)) {
		status = "ok";
	} else {
		status = "not ok";
	}
	printf(subject, status, ++test);

	subject = "%s %d - lgpng_stream_get_data\n";
	if (false != lgpng_stream_get_data(source, length, &data)) {
		status = "ok";
	} else {
		status = "not ok";
	}
	printf(subject, status, ++test);

	/* CRC */
	subject = "%s %d - lgpng_stream_get_crc with source NULL\n";
	if (false == lgpng_stream_get_crc(NULL, &crc)) {
		status = "ok";
	} else {
		status = "not ok";
	}
	printf(subject, status, ++test);

	subject = "%s %d - lgpng_stream_get_crc with crc NULL\n";
	if (false == lgpng_stream_get_crc(source, NULL)) {
		status = "ok";
	} else {
		status = "not ok";
	}
	printf(subject, status, ++test);

	subject = "%s %d - lgpng_stream_get_crc\n";
	if (false != lgpng_stream_get_crc(source, &crc)) {
		status = "ok";
	} else {
		status = "not ok";
	}
	printf(subject, status, ++test);
	free(data);
	fclose(source);
	return(EXIT_SUCCESS);
}

