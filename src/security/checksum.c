#include "localbin/security/checksum.h"
#include "localbin/core/core.h"
#include "localbin/storage/metadata.h"
#include <CommonCrypto/CommonDigest.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int checksum_calculate_sha256(const char *filepath, char *out, size_t out_size) {
    if (out_size < 65) return -1;

    FILE *f = fopen(filepath, "rb");
    if (!f) return -1;

    CC_SHA256_CTX ctx;
    CC_SHA256_Init(&ctx);

    unsigned char buf[65536];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0)
        CC_SHA256_Update(&ctx, buf, (CC_LONG)n);
    fclose(f);

    unsigned char hash[CC_SHA256_DIGEST_LENGTH];
    CC_SHA256_Final(hash, &ctx);

    for (int i = 0; i < CC_SHA256_DIGEST_LENGTH; i++)
        sprintf(out + i * 2, "%02x", hash[i]);
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
        else             { printf("  ? %s  (could not verify)\n", programs[i].name);   errors++; }
    }

    metadata_free_list(programs);
    printf("\n");
    if (errors == 0) { printf("  All programs OK\n"); return 0; }
    printf("  %d error(s) found\n", errors);
    return 1;
}
