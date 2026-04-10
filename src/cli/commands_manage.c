#include "localbin/cli/commands.h"
#include "localbin/core/core.h"
#include "localbin/core/utils.h"
#include "localbin/security/checksum.h"
#include "localbin/storage/metadata.h"
#include "commands_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

int cli_run_hook_script(const char *hook_path, const char *program_name, const char *target_path, const char *source_path) {
    if (!hook_path || hook_path[0] == '\0') {
        return 0;
    }

    char cmd[MAX_PATH * 3];
    snprintf(cmd, sizeof(cmd), "LOCALBIN_NAME=\"%s\" LOCALBIN_TARGET=\"%s\" LOCALBIN_SOURCE=\"%s\" /bin/sh \"%s\"", program_name, target_path, source_path ? source_path : "", hook_path);
    int rc = system(cmd);
    if (rc == -1) {
        return -1;
    }
    if (WIFEXITED(rc)) {
        return WEXITSTATUS(rc);
    }
    return -1;
}

void cmd_remove(const char *name) {
    char install_dir[MAX_PATH];
    get_install_dir(install_dir, sizeof(install_dir));

    char full_path[MAX_PATH];
    snprintf(full_path, sizeof(full_path), "%s/%s", install_dir, name);

    struct stat st;
    if (stat(full_path, &st) != 0) {
        fprintf(stderr, "Error: '%s' no está instalado\n", name);
        return;
    }

    ProgramMetadata *programs = NULL;
    int count = 0;

    if (metadata_list_all(&programs, &count) == 0) {
        int has_dependents = 0;
        for (int i = 0; i < count; i++) {
            for (int j = 0; j < programs[i].dep_count; j++) {
                if (strcmp(programs[i].dependencies[j], name) == 0) {
                    if (!has_dependents) {
                        printf("  Otros programas dependen de '%s':\n", name);
                        has_dependents = 1;
                    }
                    printf("   - %s\n", programs[i].name);
                }
            }
        }

        if (has_dependents) {
            printf("\n¿Continuar con la eliminación? (s/n): ");
            char response[10];
            if (fgets(response, sizeof(response), stdin) == NULL || response[0] != 's') {
                printf("Eliminación cancelada\n");
                metadata_free_list(programs);
                return;
            }
        }

        metadata_free_list(programs);
    }

    ProgramMetadata meta;
    int has_meta = metadata_load(name, &meta) == 0;

    if (unlink(full_path) != 0) {
        fprintf(stderr, "Error: No se pudo eliminar '%s'\n", name);
        return;
    }

    if (has_meta && meta.alias[0] != '\0') {
        char alias_path[MAX_PATH];
        snprintf(alias_path, sizeof(alias_path), "%s/%s", install_dir, meta.alias);
        if (file_exists(alias_path)) {
            unlink(alias_path);
        }
    }

    metadata_delete(name);

    printf("  Eliminado: %s\n", name);
}

void cmd_update(const char *name, const char *src_path) {
    char install_dir[MAX_PATH];
    get_install_dir(install_dir, sizeof(install_dir));

    char dest_path[MAX_PATH];
    snprintf(dest_path, sizeof(dest_path), "%s/%s", install_dir, name);

    struct stat st;
    if (stat(dest_path, &st) != 0) {
        fprintf(stderr, "Error: '%s' no está instalado. Usa 'install' primero.\n", name);
        return;
    }

    if (stat(src_path, &st) != 0) {
        fprintf(stderr, "Error: El archivo '%s' no existe\n", src_path);
        return;
    }

    printf(" Actualizando: %s\n", name);

    char old_checksum[65], new_checksum[65];
    checksum_calculate_sha256(dest_path, old_checksum, sizeof(old_checksum));
    checksum_calculate_sha256(src_path, new_checksum, sizeof(new_checksum));

    if (strcmp(old_checksum, new_checksum) == 0) {
        printf(" El programa ya está actualizado (mismo checksum)\n");
        return;
    }

    ProgramMetadata meta;
    int has_meta = metadata_load(name, &meta) == 0;
    if (has_meta && meta.pre_update_hook[0] != '\0') {
        int hook_rc = cli_run_hook_script(meta.pre_update_hook, name, dest_path, src_path);
        if (hook_rc != 0) {
            fprintf(stderr, "Error: pre-update hook falló (%d): %s\n", hook_rc, meta.pre_update_hook);
            return;
        }
    }

    char backup_dir[MAX_PATH];
    get_backups_dir(backup_dir, sizeof(backup_dir));
    ensure_dir(backup_dir);

    char backup_path[MAX_PATH];
    time_t now = time(NULL);
    snprintf(backup_path, sizeof(backup_path), "%s/%s.%ld.bak", backup_dir, name, now);

    if (copy_file(dest_path, backup_path) == 0) {
        printf("   Backup creado: %s\n", backup_path);
    }

    if (copy_file(src_path, dest_path) != 0) {
        fprintf(stderr, "Error: No se pudo actualizar el archivo\n");
        return;
    }

    if (has_meta) {
        strncpy(meta.checksum_sha256, new_checksum, sizeof(meta.checksum_sha256) - 1);
        meta.update_date = time(NULL);

        char abs_src_path[MAX_PATH];
        if (realpath(src_path, abs_src_path) != NULL) {
            strncpy(meta.source_path, abs_src_path, sizeof(meta.source_path) - 1);
        }

        struct stat new_st;
        if (stat(src_path, &new_st) == 0) {
            meta.size_bytes = new_st.st_size;
        }

        metadata_save(&meta);

        if (meta.post_update_hook[0] != '\0') {
            int hook_rc = cli_run_hook_script(meta.post_update_hook, name, dest_path, src_path);
            if (hook_rc != 0) {
                fprintf(stderr, "Advertencia: post-update hook falló (%d): %s\n", hook_rc, meta.post_update_hook);
            }
        }
    }

    printf(" Actualizado exitosamente\n");
    printf("   SHA256 anterior: %s\n", old_checksum);
    printf("   SHA256 nuevo:    %s\n", new_checksum);
}

void cmd_verify(const char *name) {
    char install_dir[MAX_PATH];
    get_install_dir(install_dir, sizeof(install_dir));

    char full_path[MAX_PATH];
    snprintf(full_path, sizeof(full_path), "%s/%s", install_dir, name);

    if (!file_exists(full_path)) {
        fprintf(stderr, " Error: El programa '%s' no está instalado\n", name);
        return;
    }

    ProgramMetadata meta;
    if (metadata_load(name, &meta) != 0) {
        fprintf(stderr, "  Advertencia: No hay metadata para '%s'\n", name);
        fprintf(stderr, "   Este programa fue instalado antes del sistema de metadata.\n");
        fprintf(stderr, "   Para generar metadata, reinstala el programa:\n");
        fprintf(stderr, "   localbin update %s /ruta/al/binario\n", name);

        char actual[65];
        if (checksum_calculate_sha256(full_path, actual, sizeof(actual)) == 0) {
            printf("\n Checksum actual (SHA256):\n");
            printf("   %s\n", actual);
        }
        return;
    }

    printf(" Verificando: %s\n", name);

    int result = checksum_verify_file(full_path, meta.checksum_sha256);

    if (result == 0) {
        printf(" OK - Checksum coincide\n");
    } else if (result == 1) {
        printf(" MODIFICADO - Checksum no coincide\n");

        char actual[65];
        checksum_calculate_sha256(full_path, actual, sizeof(actual));
        printf("   Esperado: %s\n", meta.checksum_sha256);
        printf("   Actual:   %s\n", actual);
    } else {
        printf("  ERROR - No se pudo verificar\n");
    }
}

void cmd_verify_all(void) {
    checksum_verify_all();
}
