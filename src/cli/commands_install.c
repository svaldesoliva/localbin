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
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

int cli_is_url_path(const char *value) {
    return strncmp(value, "http://", 7) == 0 || strncmp(value, "https://", 8) == 0;
}

int cli_download_to_temp(const char *url, char *out_path, size_t out_size) {
    char template_path[] = "/tmp/localbin-download-XXXXXX";
    int fd = mkstemp(template_path);
    if (fd < 0) {
        return -1;
    }
    close(fd);

    char cmd[MAX_PATH * 2];
    snprintf(cmd, sizeof(cmd), "curl -fsSL \"%s\" -o \"%s\"", url, template_path);
    if (system(cmd) != 0) {
        unlink(template_path);
        return -1;
    }

    chmod(template_path, 0755);
    snprintf(out_path, out_size, "%s", template_path);
    return 0;
}

void cmd_install_with_options(const char *src_path, const char *version, const char *as_name, const char *alias, const char *pre_hook, const char *post_hook) {
    struct stat st;
    char downloaded_path[MAX_PATH] = {0};
    const char *effective_src = src_path;

    if (cli_is_url_path(src_path)) {
        printf(" Descargando: %s\n", src_path);
        if (cli_download_to_temp(src_path, downloaded_path, sizeof(downloaded_path)) != 0) {
            fprintf(stderr, "Error: No se pudo descargar '%s'\n", src_path);
            return;
        }
        effective_src = downloaded_path;
    }

    if (stat(effective_src, &st) != 0) {
        fprintf(stderr, "Error: El archivo '%s' no existe\n", effective_src);
        if (downloaded_path[0] != '\0') {
            unlink(downloaded_path);
        }
        return;
    }

    if (!S_ISREG(st.st_mode)) {
        fprintf(stderr, "Error: '%s' no es un archivo regular\n", effective_src);
        if (downloaded_path[0] != '\0') {
            unlink(downloaded_path);
        }
        return;
    }

    char install_dir[MAX_PATH];
    get_install_dir(install_dir, sizeof(install_dir));

    if (ensure_dir(install_dir) != 0) {
        fprintf(stderr, "Error: No se pudo crear el directorio %s\n", install_dir);
        if (downloaded_path[0] != '\0') {
            unlink(downloaded_path);
        }
        return;
    }

    char *src_copy = strdup(src_path);
    char *base = basename(src_copy);
    const char *target_name = (as_name && as_name[0] != '\0') ? as_name : base;

    char dest_path[MAX_PATH];
    snprintf(dest_path, sizeof(dest_path), "%s/%s", install_dir, target_name);

    if (file_exists(dest_path)) {
        fprintf(stderr, "  '%s' ya está instalado. Usa 'update' para actualizarlo.\n", target_name);
        free(src_copy);
        if (downloaded_path[0] != '\0') {
            unlink(downloaded_path);
        }
        return;
    }

    char checksum[65];
    if (checksum_calculate_sha256(effective_src, checksum, sizeof(checksum)) != 0) {
        fprintf(stderr, "Error: No se pudo calcular checksum\n");
        free(src_copy);
        if (downloaded_path[0] != '\0') {
            unlink(downloaded_path);
        }
        return;
    }

    if (copy_file(effective_src, dest_path) != 0) {
        fprintf(stderr, "Error: No se pudo copiar '%s' a '%s'\n", effective_src, dest_path);
        free(src_copy);
        if (downloaded_path[0] != '\0') {
            unlink(downloaded_path);
        }
        return;
    }

    ProgramMetadata meta;
    memset(&meta, 0, sizeof(meta));
    strncpy(meta.name, target_name, sizeof(meta.name) - 1);
    strncpy(meta.version, version ? version : "1.0.0", sizeof(meta.version) - 1);

    char abs_src_path[MAX_PATH];
    if (realpath(src_path, abs_src_path) != NULL) {
        strncpy(meta.source_path, abs_src_path, sizeof(meta.source_path) - 1);
    } else {
        strncpy(meta.source_path, src_path, sizeof(meta.source_path) - 1);
    }

    strncpy(meta.alias, alias ? alias : "", sizeof(meta.alias) - 1);
    strncpy(meta.pre_update_hook, pre_hook ? pre_hook : "", sizeof(meta.pre_update_hook) - 1);
    strncpy(meta.post_update_hook, post_hook ? post_hook : "", sizeof(meta.post_update_hook) - 1);
    strncpy(meta.checksum_sha256, checksum, sizeof(meta.checksum_sha256) - 1);
    meta.install_date = time(NULL);
    meta.update_date = meta.install_date;
    meta.size_bytes = st.st_size;
    meta.permissions = st.st_mode;
    meta.dep_count = 0;

    if (metadata_save(&meta) != 0) {
        fprintf(stderr, "  Instalado pero no se pudo guardar metadata\n");
    }

    if (alias && alias[0] != '\0') {
        char alias_path[MAX_PATH];
        snprintf(alias_path, sizeof(alias_path), "%s/%s", install_dir, alias);
        if (file_exists(alias_path)) {
            fprintf(stderr, "  Alias '%s' no creado: ya existe\n", alias);
        } else if (symlink(target_name, alias_path) != 0) {
            fprintf(stderr, "  Alias '%s' no creado\n", alias);
        } else {
            printf(" Alias creado: %s -> %s\n", alias, target_name);
        }
    }

    printf(" Instalado: %s", target_name);
    if (version) {
        printf(" (v%s)", version);
    }
    printf("\n");
    printf("   Ruta: %s\n", dest_path);
    printf("   SHA256: %s\n", checksum);

    char size_str[32];
    format_size(st.st_size, size_str, sizeof(size_str));
    printf("   Tamaño: %s\n", size_str);

    if (pre_hook && pre_hook[0] != '\0') {
        printf("   Pre-update hook: %s\n", pre_hook);
    }
    if (post_hook && post_hook[0] != '\0') {
        printf("   Post-update hook: %s\n", post_hook);
    }

    free(src_copy);
    if (downloaded_path[0] != '\0') {
        unlink(downloaded_path);
    }

    warn_path();
}

void cmd_install(const char *src_path, const char *version) {
    cmd_install_with_options(src_path, version, NULL, NULL, NULL, NULL);
}
