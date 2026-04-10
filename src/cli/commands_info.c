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

    char full_path[MAX_PATH];
    snprintf(full_path, sizeof(full_path), "%s/%s", install_dir, name);

    struct stat st;
    if (stat(full_path, &st) != 0) {
        fprintf(stderr, "Error: '%s' no está instalado\n", name);
        return;
    }

    printf(" Información de: %s\n", name);
    printf("═══════════════════════════════════════\n\n");
    printf("Ubicación:    %s\n", full_path);

    char size_str[32];
    format_size(st.st_size, size_str, sizeof(size_str));
    printf("Tamaño:       %s (%lld bytes)\n", size_str, (long long)st.st_size);
    printf("Permisos:     %o\n", st.st_mode & 0777);

    char date_str[64];
    format_time(st.st_mtime, date_str, sizeof(date_str));
    printf("Modificado:   %s\n", date_str);

    char checksum[65];
    if (checksum_calculate_sha256(full_path, checksum, sizeof(checksum)) == 0) {
        printf("SHA256:       %s\n", checksum);
    }

    ProgramMetadata meta;
    if (metadata_load(name, &meta) == 0) {
        printf("\n Metadata:\n");
        printf("───────────────────────────────────────\n");
        printf("Versión:      %s\n", meta.version);

        char install_date_str[64];
        format_time(meta.install_date, install_date_str, sizeof(install_date_str));
        printf("Instalado:    %s\n", install_date_str);

        if (meta.update_date != meta.install_date) {
            char update_date_str[64];
            format_time(meta.update_date, update_date_str, sizeof(update_date_str));
            printf("Actualizado:  %s\n", update_date_str);
        }

        if (meta.source_path[0] != '\0') {
            printf("Origen:       %s\n", meta.source_path);
        }
        if (meta.alias[0] != '\0') {
            printf("Alias:        %s\n", meta.alias);
        }
        if (meta.pre_update_hook[0] != '\0') {
            printf("Pre-hook:     %s\n", meta.pre_update_hook);
        }
        if (meta.post_update_hook[0] != '\0') {
            printf("Post-hook:    %s\n", meta.post_update_hook);
        }

        if (meta.checksum_sha256[0] != '\0') {
            int verify = checksum_verify_file(full_path, meta.checksum_sha256);
            if (verify == 0) {
                printf("Integridad:    OK (checksum coincide)\n");
            } else {
                printf("Integridad:    MODIFICADO (checksum no coincide)\n");
                printf("  Esperado:   %s\n", meta.checksum_sha256);
                printf("  Actual:     %s\n", checksum);
            }
        }

        if (meta.dep_count > 0) {
            printf("\n Dependencias:\n");
            for (int i = 0; i < meta.dep_count; i++) {
                printf("  - %s\n", meta.dependencies[i]);
            }
        }
    } else {
        printf("\n  No hay metadata disponible para este programa\n");
    }

    printf("\n");
}
