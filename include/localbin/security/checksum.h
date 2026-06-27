#ifndef CHECKSUM_H
#define CHECKSUM_H

#include <stddef.h>

int checksum_calculate_sha256(const char *filepath, char *output, size_t output_size);
int checksum_verify_file(const char *filepath, const char *expected_sha256);
int checksum_verify_all(void);

#endif // CHECKSUM_H
