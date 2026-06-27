#include "localbin/cli/commands.h"
#include "localbin/core/core.h"
#include "localbin/core/utils.h"
#include "localbin/security/checksum.h"
#include "localbin/storage/metadata.h"
#include <stdio.h>
#include <sys/stat.h>

void cmd_info(const char *name) {
    char install_dir[MAX_PATH];
    get_install_dir(install_dir, sizeof(install_dir));
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s/%s", install_dir, name);

    struct stat st;
    if (stat(path, &st) != 0) { fprintf(stderr, "Error: '%s' not installed\n", name); return; }

    char size_str[32], date_str[64], checksum[65];
    format_size(st.st_size, size_str, sizeof(size_str));
    format_time(st.st_mtime, date_str, sizeof(date_str));
    checksum_calculate_sha256(path, checksum, sizeof(checksum));

    printf("  %s\n", name);
    printf("  %-14s %s\n", "Path:",     path);
    printf("  %-14s %s (%lld bytes)\n", "Size:",  size_str, (long long)st.st_size);
    printf("  %-14s %o\n",  "Permissions:", st.st_mode & 0777);
    printf("  %-14s %s\n",  "Modified:", date_str);
    printf("  %-14s %s\n",  "SHA256:", checksum);

    ProgramMetadata meta = {0};
    if (metadata_load(name, &meta) != 0) {
        printf("\n  No metadata (reinstall to generate)\n\n");
        return;
    }

    char idate[64];
    format_time(meta.install_date, idate, sizeof(idate));
    printf("\n  %-14s %s\n", "Version:",   meta.version);
    printf("  %-14s %s\n",   "Installed:", idate);

    if (meta.update_date != meta.install_date) {
        char udate[64];
        format_time(meta.update_date, udate, sizeof(udate));
        printf("  %-14s %s\n", "Updated:", udate);
    }
    if (meta.source_path[0]) printf("  %-14s %s\n", "Source:", meta.source_path);
    if (meta.alias[0])       printf("  %-14s %s\n", "Alias:",  meta.alias);
    if (meta.pre_update_hook[0])  printf("  %-14s %s\n", "Pre-hook:",  meta.pre_update_hook);
    if (meta.post_update_hook[0]) printf("  %-14s %s\n", "Post-hook:", meta.post_update_hook);

    if (meta.checksum_sha256[0]) {
        int v = checksum_verify_file(path, meta.checksum_sha256);
        printf("  %-14s %s\n", "Integrity:",
               v == 0 ? "✓ OK" : (v == 1 ? "✗ MODIFIED" : "? ERROR"));
        if (v == 1)
            printf("  Expected: %s\n  Actual:   %s\n", meta.checksum_sha256, checksum);
    }
    if (meta.dep_count > 0) {
        printf("\n  Dependencies:\n");
        for (int i = 0; i < meta.dep_count; i++) printf("    - %s\n", meta.dependencies[i]);
    }
    printf("\n");
}
