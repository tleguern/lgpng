#include "../config.h"

#if HAVE_ERR
# include <err.h>
#endif
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../lgpng.h"

uint8_t source[] = {
	0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d,
	0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x20,
	0x01, 0x00, 0x00, 0x00, 0x01, 0x2c, 0x06, 0x77, 0xcf, 0x00, 0x00, 0x00,
	0x04, 0x67, 0x41, 0x4d, 0x41, 0x00, 0x01, 0x86, 0xa0, 0x31, 0xe8, 0x96,
	0x5f, 0x00, 0x00, 0x00, 0x90, 0x49, 0x44, 0x41, 0x54, 0x78, 0x9c, 0x2d,
	0x8d, 0x31, 0x0e, 0xc2, 0x30, 0x0c, 0x45, 0xdf, 0xc6, 0x82, 0xc4, 0x15,
	0x18, 0x7a, 0x00, 0xa4, 0x2e, 0x19, 0x7a, 0xb8, 0x1e, 0x83, 0xb1, 0x27,
	0xe0, 0x0c, 0x56, 0x39, 0x00, 0x13, 0x63, 0xa5, 0x80, 0xd8, 0x58, 0x2c,
	0x65, 0xc9, 0x10, 0x35, 0x7c, 0x4b, 0x78, 0xb0, 0xbf, 0xbf, 0xdf, 0x4f,
	0x70, 0x16, 0x8c, 0x19, 0xe7, 0xac, 0xb9, 0x70, 0xa3, 0xf2, 0xd1, 0xde,
	0xd9, 0x69, 0x5c, 0xe5, 0xbf, 0x59, 0x63, 0xdf, 0xd9, 0x2a, 0xaf, 0x4c,
	0x9f, 0xd9, 0x27, 0xea, 0x44, 0x9e, 0x64, 0x87, 0xdf, 0x5b, 0x9c, 0x36,
	0xe7, 0x99, 0xb9, 0x1b, 0xdf, 0x08, 0x2b, 0x4d, 0x4b, 0xd4, 0x01, 0x4f,
	0xe4, 0x01, 0x4b, 0x01, 0xab, 0x7a, 0x17, 0xae, 0xe6, 0x94, 0xd2, 0x8d,
	0x32, 0x8a, 0x2d, 0x63, 0x83, 0x7a, 0x70, 0x45, 0x1e, 0x16, 0x48, 0x70,
	0x2d, 0x9a, 0x9f, 0xf4, 0xa1, 0x1d, 0x2f, 0x7a, 0x51, 0xaa, 0x21, 0xe5,
	0xa1, 0x8c, 0x7f, 0xfd, 0x00, 0x94, 0xe3, 0x51, 0x1d, 0x66, 0x18, 0x22,
	0xf2, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60,
	0x82
};

int
main(void)
{
	int		 rc = EXIT_SUCCESS, test = 0, offset = 0;
	uint32_t	 length = 0, crc = 0;
	uint8_t		 type[4] = {0, 0, 0, 0};
	size_t		 sourcez = sizeof(source);
	uint8_t		*data = NULL;
	const char	*subject, *status;

	printf("lgpng_data tests\n");
	printf("TAP version 13\n");
	printf("1..22\n");

	/* PNG signature */
	subject = "%s %d - lgpng_data_is_png with data NULL\n";
	if (LGPNG_INVALID_PARAM == lgpng_data_is_png(NULL, 0)) {
		status = "ok";
	} else {
		status = "not ok";
		rc = EXIT_FAILURE;
	}
	printf(subject, status, ++test);

	subject = "%s %d - lgpng_data_is_png with short dataz\n";
	if (LGPNG_TOO_SHORT == lgpng_data_is_png(source, 4)) {
		status = "ok";
	} else {
		status = "not ok";
		rc = EXIT_FAILURE;
	}
	printf(subject, status, ++test);

	subject = "%s %d - lgpng_data_is_png\n";
	if (LGPNG_OK == lgpng_data_is_png(source, sourcez)) {
		status = "ok";
	} else {
		status = "not ok";
		rc = EXIT_FAILURE;
	}
	printf(subject, status, ++test);

	/* Length */
	offset += 8;
	sourcez -= 8;
	subject = "%s %d - lgpng_data_get_length with data NULL\n";
	if (LGPNG_INVALID_PARAM == lgpng_data_get_length(NULL, sourcez, &length)) {
		status = "ok";
	} else {
		status = "not ok";
		rc = EXIT_FAILURE;
	}
	printf(subject, status, ++test);

	subject = "%s %d - lgpng_data_get_length with length NULL\n";
	if (LGPNG_INVALID_PARAM == lgpng_data_get_length(source + offset, sourcez, NULL)) {
		status = "ok";
	} else {
		status = "not ok";
		rc = EXIT_FAILURE;
	}
	printf(subject, status, ++test);

	subject = "%s %d - lgpng_data_get_length with short dataz\n";
	if (LGPNG_TOO_SHORT == lgpng_data_get_length(source + offset, 3, &length)) {
		status = "ok";
	} else {
		status = "not ok";
		rc = EXIT_FAILURE;
	}
	printf(subject, status, ++test);

	subject = "%s %d - lgpng_data_get_length\n";
	if (LGPNG_OK == lgpng_data_get_length(source + offset, sourcez, &length)) {
		status = "ok";
	} else {
		status = "not ok";
		rc = EXIT_FAILURE;
	}
	printf(subject, status, ++test);

	subject = "%s %d - length should be 13\n";
	if (13 == length) {
		status = "ok";
	} else {
		status = "not ok";
		rc = EXIT_FAILURE;
	}
	printf(subject, status, ++test);

	/* Type */
	offset += 4;
	sourcez -= 4;
	subject = "%s %d - lgpng_data_get_type with data NULL\n";
	if (LGPNG_INVALID_PARAM == lgpng_data_get_type(NULL, 0, type)) {
		status = "ok";
	} else {
		status = "not ok";
		rc = EXIT_FAILURE;
	}
	printf(subject, status, ++test);

	subject = "%s %d - lgpng_data_get_type with short srcz\n";
	if (LGPNG_TOO_SHORT == lgpng_data_get_type(source + offset, 1, type)) {
		status = "ok";
	} else {
		status = "not ok";
		rc = EXIT_FAILURE;
	}
	printf(subject, status, ++test);

	subject = "%s %d - lgpng_data_get_type\n";
	if (LGPNG_OK == lgpng_data_get_type(source + offset, sourcez, type)) {
		status = "ok";
	} else {
		status = "not ok";
		rc = EXIT_FAILURE;
	}
	printf(subject, status, ++test);

	subject = "%s %d - chunk type should be IHDR\n";
	if (0 == memcmp(type, "IHDR", 4)) {
		status = "ok";
	} else {
		status = "not ok";
		rc = EXIT_FAILURE;
	}
	printf(subject, status, ++test);

	if (NULL == (data = malloc(length + 1))) {
		err(EXIT_FAILURE, "malloc(length + 1)");
	}

	/* Data */
	offset += length;
	sourcez -= length;
	subject = "%s %d - lgpng_data_get_data with data NULL\n";
	if (LGPNG_INVALID_PARAM == lgpng_data_get_data(NULL, 0, length, &data)) {
		status = "ok";
	} else {
		status = "not ok";
		rc = EXIT_FAILURE;
	}
	printf(subject, status, ++test);

	subject = "%s %d - lgpng_data_get_data with data NULL\n";
	if (LGPNG_INVALID_PARAM == lgpng_data_get_data(source + offset, sourcez, length, NULL)) {
		status = "ok";
	} else {
		status = "not ok";
		rc = EXIT_FAILURE;
	}
	printf(subject, status, ++test);

	subject = "%s %d - lgpng_data_get_data with short srcz\n";
	if (LGPNG_TOO_SHORT == lgpng_data_get_data(source + offset, length / 2, length, &data)) {
		status = "ok";
	} else {
		status = "not ok";
		rc = EXIT_FAILURE;
	}
	printf(subject, status, ++test);

	subject = "%s %d - lgpng_data_get_data\n";
	if (LGPNG_OK == lgpng_data_get_data(source + offset, sourcez, length, &data)) {
		status = "ok";
	} else {
		status = "not ok";
		rc = EXIT_FAILURE;
	}
	printf(subject, status, ++test);

	/* CRC */
	offset += 4;
	sourcez -= 4;
	subject = "%s %d - lgpng_data_get_crc with data NULL\n";
	if (LGPNG_INVALID_PARAM == lgpng_data_get_crc(NULL, 0, &crc)) {
		status = "ok";
	} else {
		status = "not ok";
		rc = EXIT_FAILURE;
	}
	printf(subject, status, ++test);

	subject = "%s %d - lgpng_data_get_crc with crc NULL\n";
	if (LGPNG_INVALID_PARAM == lgpng_data_get_crc(source + offset, sourcez, NULL)) {
		status = "ok";
	} else {
		status = "not ok";
		rc = EXIT_FAILURE;
	}
	printf(subject, status, ++test);

	subject = "%s %d - lgpng_data_get_crc with short dataz\n";
	if (LGPNG_TOO_SHORT == lgpng_data_get_crc(source + offset, 1, &crc)) {
		status = "ok";
	} else {
		status = "not ok";
		rc = EXIT_FAILURE;
	}
	printf(subject, status, ++test);

	subject = "%s %d - lgpng_data_get_crc\n";
	if (LGPNG_OK == lgpng_data_get_crc(source + offset, sourcez, &crc)) {
		status = "ok";
	} else {
		status = "not ok";
		rc = EXIT_FAILURE;
	}
	printf(subject, status, ++test);

	free(data);
	return(rc);
}

