#include "localbin/cli/commands.h"
#include "localbin/core/core.h"
#include "localbin/core/utils.h"
#include "localbin/security/checksum.h"
#include "localbin/storage/metadata.h"
#include "commands_internal.h"
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

int cli_is_url_path(const char *v) {
    return strncmp(v, "http://", 7) == 0 || strncmp(v, "https://", 8) == 0;
}

int cli_download_to_temp(const char *url, char *out, size_t out_size) {
    char tmp[] = "/tmp/localbin-XXXXXX";
    int fd = mkstemp(tmp);
    if (fd < 0) return -1;
    close(fd);

    char cmd[MAX_PATH * 2];
    snprintf(cmd, sizeof(cmd), "curl -fsSL \"%s\" -o \"%s\"", url, tmp);
    if (system(cmd) != 0) { unlink(tmp); return -1; }

    chmod(tmp, 0755);
    snprintf(out, out_size, "%s", tmp);
    return 0;
}

void cmd_install_with_options(const char *src_path, const char *version,
                              const char *as_name, const char *alias,
                              const char *pre_hook, const char *post_hook) {
    char downloaded[MAX_PATH] = {0};
    const char *src = src_path;

    if (cli_is_url_path(src_path)) {
        printf("  Downloading: %s\n", src_path);
        if (cli_download_to_temp(src_path, downloaded, sizeof(downloaded)) != 0) {
            fprintf(stderr, "Error: download failed: %s\n", src_path);
            return;
        }
        src = downloaded;
    }

    struct stat st;
    if (stat(src, &st) != 0 || !S_ISREG(st.st_mode)) {
        fprintf(stderr, "Error: not a regular file: %s\n", src);
        if (downloaded[0]) unlink(downloaded);
        return;
    }

    char install_dir[MAX_PATH];
    get_install_dir(install_dir, sizeof(install_dir));
    if (ensure_dir(install_dir) != 0) {
        fprintf(stderr, "Error: cannot create %s\n", install_dir);
        if (downloaded[0]) unlink(downloaded);
        return;
    }

    char *src_copy = strdup(src_path);
    const char *target = (as_name && *as_name) ? as_name : basename(src_copy);

    char dest[MAX_PATH];
    snprintf(dest, sizeof(dest), "%s/%s", install_dir, target);

    if (file_exists(dest)) {
        fprintf(stderr, "  '%s' already installed. Use 'update' to upgrade.\n", target);
        free(src_copy); if (downloaded[0]) unlink(downloaded);
        return;
    }

    char checksum[65];
    if (checksum_calculate_sha256(src, checksum, sizeof(checksum)) != 0) {
        fprintf(stderr, "Error: checksum failed\n");
        free(src_copy); if (downloaded[0]) unlink(downloaded);
        return;
    }

    if (copy_file(src, dest) != 0) {
        fprintf(stderr, "Error: copy failed: %s -> %s\n", src, dest);
        free(src_copy); if (downloaded[0]) unlink(downloaded);
        return;
    }

    ProgramMetadata meta = {0};
    strncpy(meta.name,    target,               sizeof(meta.name) - 1);
    strncpy(meta.version, version ? version : "1.0.0", sizeof(meta.version) - 1);

    char abs_src[MAX_PATH];
    const char *path_to_resolve = downloaded[0] ? downloaded : src_path;
    strncpy(meta.source_path,
            realpath(path_to_resolve, abs_src) ? abs_src : src_path,
            sizeof(meta.source_path) - 1);

    strncpy(meta.alias,            alias     ? alias     : "", sizeof(meta.alias) - 1);
    strncpy(meta.pre_update_hook,  pre_hook  ? pre_hook  : "", sizeof(meta.pre_update_hook) - 1);
    strncpy(meta.post_update_hook, post_hook ? post_hook : "", sizeof(meta.post_update_hook) - 1);
    strncpy(meta.checksum_sha256,  checksum,                   sizeof(meta.checksum_sha256) - 1);
    meta.install_date = meta.update_date = time(NULL);
    meta.size_bytes   = st.st_size;
    meta.permissions  = st.st_mode;

    if (metadata_save(&meta) != 0)
        fprintf(stderr, "  Installed but metadata could not be saved\n");

    if (alias && *alias) {
        char alias_path[MAX_PATH];
        snprintf(alias_path, sizeof(alias_path), "%s/%s", install_dir, alias);
        if (file_exists(alias_path))
            fprintf(stderr, "  Alias '%s' skipped: already exists\n", alias);
        else if (symlink(target, alias_path) != 0)
            fprintf(stderr, "  Alias '%s' could not be created\n", alias);
        else
            printf("  Alias: %s -> %s\n", alias, target);
    }

    char size_str[32];
    format_size(st.st_size, size_str, sizeof(size_str));
    printf("  Installed: %s%s%s\n", target, version ? " v" : "", version ? version : "");
    printf("  Path:   %s\n", dest);
    printf("  SHA256: %s\n", checksum);
    printf("  Size:   %s\n", size_str);
    if (pre_hook  && *pre_hook)  printf("  Pre-hook:  %s\n", pre_hook);
    if (post_hook && *post_hook) printf("  Post-hook: %s\n", post_hook);

    free(src_copy);
    if (downloaded[0]) unlink(downloaded);
    warn_path();
}

void cmd_install(const char *src_path, const char *version) {
    cmd_install_with_options(src_path, version, NULL, NULL, NULL, NULL);
}
