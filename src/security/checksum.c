#include "localbin/security/checksum.h"
#include "localbin/security/sha256.h"
#include "localbin/core/core.h"
#include "localbin/storage/metadata.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int checksum_calculate_sha256(const char *filepath, char *out, size_t out_size) {
    if (out_size < 65) return -1;

    FILE *f = fopen(filepath, "rb");
    if (!f) return -1;

    SHA256_CTX ctx;
    sha256_init(&ctx);

    uint8_t buf[65536];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0)
        sha256_update(&ctx, buf, n);
    fclose(f);

    uint8_t digest[32];
    sha256_final(&ctx, digest);
    for (int i = 0; i < 32; i++) sprintf(out + i * 2, "%02x", digest[i]);
    out[64] = '\0';
    return 0;
}

/* Returns 0 = match, 1 = mismatch, -1 = error. */
int checksum_verify_file(const char *filepath, const char *expected) {
    char actual[65];
    if (checksum_calculate_sha256(filepath, actual, sizeof(actual)) != 0) return -1;
    return strcmp(actual, expected) == 0 ? 0 : 1;
}

int checksum_verify_all(void) {
    ProgramMetadata *programs = NULL;
    int count = 0;
    if (metadata_list_all(&programs, &count) != 0) return -1;

    char install_dir[MAX_PATH];
    get_install_dir(install_dir, sizeof(install_dir));
    printf("  Verifying %d program(s)...\n\n", count);

    int errors = 0;
    for (int i = 0; i < count; i++) {
        char path[MAX_PATH];
        snprintf(path, sizeof(path), "%s/%s", install_dir, programs[i].name);
        int r = checksum_verify_file(path, programs[i].checksum_sha256);
        if      (r == 0) printf("  ✓ %s\n", programs[i].name);
        else if (r == 1) { printf("  ✗ %s  (checksum mismatch)\n", programs[i].name); errors++; }
        else             { printf("  ? %s  (could not verify)\n",   programs[i].name); errors++; }
    }

    metadata_free_list(programs);
    printf("\n");
    if (errors == 0) { printf("  All programs OK\n"); return 0; }
    printf("  %d error(s) found\n", errors);
    return 1;
}
