#ifndef CHECKSUM_H
#define CHECKSUM_H

#include <stddef.h>  // Para size_t

// Funciones de checksums y verificación
int checksum_calculate_sha256(const char *filepath, char *output, size_t output_size);
int checksum_verify_file(const char *filepath, const char *expected_sha256);
int checksum_verify_all(void);
int checksum_compare_files(const char *file1, const char *file2);

#endif // CHECKSUM_H
