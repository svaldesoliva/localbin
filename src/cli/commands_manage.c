#include "localbin/cli/commands.h"
#include "localbin/core/core.h"
#include "localbin/core/utils.h"
#include "localbin/security/checksum.h"
#include "localbin/storage/metadata.h"
#include "commands_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

/* Run an optional hook script with context env vars.  Returns 0 on success. */
int cli_run_hook_script(const char *hook, const char *name,
                        const char *target, const char *source) {
    if (!hook || !*hook) return 0;
    char cmd[MAX_PATH * 3];
    snprintf(cmd, sizeof(cmd),
             "LOCALBIN_NAME=\"%s\" LOCALBIN_TARGET=\"%s\" LOCALBIN_SOURCE=\"%s\" /bin/sh \"%s\"",
             name, target, source ? source : "", hook);
    int rc = system(cmd);
    if (rc == -1) return -1;
    return WIFEXITED(rc) ? WEXITSTATUS(rc) : -1;
}

void cmd_remove(const char *name) {
    char install_dir[MAX_PATH];
    get_install_dir(install_dir, sizeof(install_dir));

    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s/%s", install_dir, name);
    if (!file_exists(path)) { fprintf(stderr, "Error: '%s' not installed\n", name); return; }



    ProgramMetadata meta = {0};
    int has_meta = metadata_load(name, &meta) == 0;

    if (unlink(path) != 0) { fprintf(stderr, "Error: could not remove '%s'\n", name); return; }

    if (has_meta && meta.alias[0]) {
        char alias_path[MAX_PATH];
        snprintf(alias_path, sizeof(alias_path), "%s/%s", install_dir, meta.alias);
        if (file_exists(alias_path)) unlink(alias_path);
    }
    metadata_delete(name);
    printf("  Removed: %s\n", name);
}

void cmd_update(const char *name, const char *src_path) {
    char install_dir[MAX_PATH];
    get_install_dir(install_dir, sizeof(install_dir));

    char dest[MAX_PATH];
    snprintf(dest, sizeof(dest), "%s/%s", install_dir, name);
    if (!file_exists(dest)) {
        fprintf(stderr, "Error: '%s' not installed. Use 'install' first.\n", name);
        return;
    }
    if (!file_exists(src_path)) {
        fprintf(stderr, "Error: source '%s' does not exist\n", src_path);
        return;
    }

    printf("  Updating: %s\n", name);

    char old_sum[65], new_sum[65];
    checksum_calculate_sha256(dest,     old_sum, sizeof(old_sum));
    checksum_calculate_sha256(src_path, new_sum, sizeof(new_sum));

    if (strcmp(old_sum, new_sum) == 0) { printf("  Already up to date\n"); return; }

    ProgramMetadata meta = {0};
    int has_meta = metadata_load(name, &meta) == 0;

    if (has_meta && meta.pre_update_hook[0]) {
        int rc = cli_run_hook_script(meta.pre_update_hook, name, dest, src_path);
        if (rc != 0) { fprintf(stderr, "Error: pre-update hook failed (%d)\n", rc); return; }
    }

    if (copy_file(src_path, dest) != 0) { fprintf(stderr, "Error: update failed\n"); return; }

    if (has_meta) {
        strncpy(meta.checksum_sha256, new_sum, sizeof(meta.checksum_sha256) - 1);
        meta.update_date = time(NULL);
        char abs[MAX_PATH];
        strncpy(meta.source_path,
                realpath(src_path, abs) ? abs : src_path,
                sizeof(meta.source_path) - 1);
        struct stat st;
        if (stat(src_path, &st) == 0) meta.size_bytes = st.st_size;
        metadata_save(&meta);

        if (meta.post_update_hook[0]) {
            int rc = cli_run_hook_script(meta.post_update_hook, name, dest, src_path);
            if (rc != 0) fprintf(stderr, "Warning: post-update hook failed (%d)\n", rc);
        }
    }

    printf("  Updated\n");
    printf("  Old SHA256: %s\n", old_sum);
    printf("  New SHA256: %s\n", new_sum);
}

void cmd_verify(const char *name) {
    char install_dir[MAX_PATH];
    get_install_dir(install_dir, sizeof(install_dir));
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s/%s", install_dir, name);

    if (!file_exists(path)) { fprintf(stderr, "Error: '%s' not installed\n", name); return; }

    ProgramMetadata meta = {0};
    if (metadata_load(name, &meta) != 0) {
        fprintf(stderr, "  No metadata for '%s' (reinstall to generate)\n", name);
        char actual[65];
        if (checksum_calculate_sha256(path, actual, sizeof(actual)) == 0)
            printf("  Current SHA256: %s\n", actual);
        return;
    }

    printf("  Verifying: %s\n", name);
    int r = checksum_verify_file(path, meta.checksum_sha256);
    if (r == 0)      printf("  " COLOR_GREEN "✓ OK" COLOR_RESET "\n");
    else if (r == 1) {
        char actual[65];
        checksum_calculate_sha256(path, actual, sizeof(actual));
        printf("  " COLOR_RED "✗ MODIFIED" COLOR_RESET "\n  Expected: %s\n  Actual:   %s\n", meta.checksum_sha256, actual);
    } else           printf("  " COLOR_BLUE "? ERROR" COLOR_RESET "\n");
}

void cmd_verify_all(void) { checksum_verify_all(); }

void cmd_which(const char *name) {
    char install_dir[MAX_PATH];
    get_install_dir(install_dir, sizeof(install_dir));
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s/%s", install_dir, name);
    if (!file_exists(path)) { fprintf(stderr, "Error: '%s' not installed\n", name); return; }
    printf("%s\n", path);
}

void cmd_rename(const char *old_name, const char *new_name) {
    char install_dir[MAX_PATH];
    get_install_dir(install_dir, sizeof(install_dir));

    char old_path[MAX_PATH], new_path[MAX_PATH];
    snprintf(old_path, sizeof(old_path), "%s/%s", install_dir, old_name);
    snprintf(new_path, sizeof(new_path), "%s/%s", install_dir, new_name);

    if (!file_exists(old_path)) { fprintf(stderr, "Error: '%s' not installed\n", old_name); return; }
    if (file_exists(new_path))  { fprintf(stderr, "Error: '%s' already exists\n", new_name); return; }

    if (rename(old_path, new_path) != 0) { fprintf(stderr, "Error: rename failed\n"); return; }

    ProgramMetadata meta = {0};
    if (metadata_load(old_name, &meta) == 0) {
        metadata_delete(old_name);
        strncpy(meta.name, new_name, sizeof(meta.name) - 1);
        metadata_save(&meta);
    }
    printf("  Renamed: %s -> %s\n", old_name, new_name);
}

