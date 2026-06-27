#include "localbin/security/checksum.h"
#include "localbin/core/core.h"
#include "localbin/core/utils.h"
#include "localbin/storage/metadata.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int checksum_calculate_sha256(const char *path, char *out_hash, size_t out_size) {
    if (out_size < 65) return -1;
    
    char cmd[MAX_PATH * 2];
    /* Try shasum (macOS/Linux standard) then sha256sum (Linux) */
    snprintf(cmd, sizeof(cmd), "shasum -a 256 \"%s\" 2>/dev/null || sha256sum \"%s\" 2>/dev/null", path, path);
    
    FILE *f = popen(cmd, "r");
    if (!f) return -1;
    
    char buf[128];
    if (fgets(buf, sizeof(buf), f)) {
        char *space = strchr(buf, ' ');
        if (space) *space = '\0';
        strncpy(out_hash, buf, out_size - 1);
        out_hash[out_size - 1] = '\0';
        pclose(f);
        return 0;
    }
    pclose(f);
    return -1;
}

int checksum_verify_file(const char *path, const char *expected_hash) {
    if (!expected_hash || !*expected_hash) return -1;
    char actual[65];
    if (checksum_calculate_sha256(path, actual, sizeof(actual)) != 0) return -1;
    return strcmp(actual, expected_hash) == 0 ? 0 : 1;
}

void checksum_verify_all(void) {
    ProgramMetadata *programs = NULL;
    int count = 0;
    
    if (metadata_list_all(&programs, &count) != 0 || count == 0) {
        printf("  No programs installed or could not read metadata.\n");
        return;
    }

    printf("  Verifying %d program(s)...\n\n", count);
    int errors = 0;

    char install_dir[MAX_PATH];
    get_install_dir(install_dir, sizeof(install_dir));

    for (int i = 0; i < count; i++) {
        char path[MAX_PATH];
        snprintf(path, sizeof(path), "%s/%s", install_dir, programs[i].name);
        int r = checksum_verify_file(path, programs[i].checksum_sha256);
        if      (r == 0) printf("  " COLOR_GREEN "✓" COLOR_RESET " %s\n", programs[i].name);
        else if (r == 1) { printf("  " COLOR_RED "✗" COLOR_RESET " %s  (checksum mismatch)\n", programs[i].name); errors++; }
        else             { printf("  " COLOR_BLUE "?" COLOR_RESET " %s  (could not verify)\n",   programs[i].name); errors++; }
    }

    metadata_free_list(programs);
    
    if (errors == 0) printf("\n  All programs OK\n");
    else             printf("\n  %d program(s) failed verification\n", errors);
}
