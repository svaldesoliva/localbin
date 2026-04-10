#include "localbin/cli/commands.h"
#include "localbin/core/core.h"
#include "localbin/core/utils.h"
#include "localbin/storage/metadata.h"
#include "localbin/security/checksum.h"
#include "localbin/app/version.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <libgen.h>
#include <time.h>
#include <dirent.h>

static int is_url_path(const char *value) {
    return strncmp(value, "http://", 7) == 0 || strncmp(value, "https://", 8) == 0;
}

static int download_to_temp(const char *url, char *out_path, size_t out_size) {
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

static int run_hook_script(const char *hook_path, const char *program_name, const char *target_path, const char *source_path) {
    if (!hook_path || hook_path[0] == '\0') {
        return 0;
    }

    char cmd[MAX_PATH * 3];
    snprintf(cmd, sizeof(cmd), "LOCALBIN_NAME=\"%s\" LOCALBIN_TARGET=\"%s\" LOCALBIN_SOURCE=\"%s\" /bin/sh \"%s\"",
             program_name, target_path, source_path ? source_path : "", hook_path);
    int rc = system(cmd);
    if (rc == -1) {
        return -1;
    }
    if (WIFEXITED(rc)) {
        return WEXITSTATUS(rc);
    }
    return -1;
}

void cmd_install_with_options(const char *src_path, const char *version, const char *as_name, const char *alias, const char *pre_hook, const char *post_hook) {
    struct stat st;
    char downloaded_path[MAX_PATH] = {0};
    const char *effective_src = src_path;

    if (is_url_path(src_path)) {
        printf(" Descargando: %s\n", src_path);
        if (download_to_temp(src_path, downloaded_path, sizeof(downloaded_path)) != 0) {
            fprintf(stderr, "Error: No se pudo descargar '%s'\n", src_path);
            return;
        }
        effective_src = downloaded_path;
    }

    if (stat(effective_src, &st) != 0) {
        fprintf(stderr, "Error: El archivo '%s' no existe\n", effective_src);
        if (downloaded_path[0] != '\0') unlink(downloaded_path);
        return;
    }

    if (!S_ISREG(st.st_mode)) {
        fprintf(stderr, "Error: '%s' no es un archivo regular\n", effective_src);
        if (downloaded_path[0] != '\0') unlink(downloaded_path);
        return;
    }

    char install_dir[MAX_PATH];
    get_install_dir(install_dir, sizeof(install_dir));

    if (ensure_dir(install_dir) != 0) {
        fprintf(stderr, "Error: No se pudo crear el directorio %s\n", install_dir);
        if (downloaded_path[0] != '\0') unlink(downloaded_path);
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
        if (downloaded_path[0] != '\0') unlink(downloaded_path);
        return;
    }

    char checksum[65];
    if (checksum_calculate_sha256(effective_src, checksum, sizeof(checksum)) != 0) {
        fprintf(stderr, "Error: No se pudo calcular checksum\n");
        free(src_copy);
        if (downloaded_path[0] != '\0') unlink(downloaded_path);
        return;
    }

    if (copy_file(effective_src, dest_path) != 0) {
        fprintf(stderr, "Error: No se pudo copiar '%s' a '%s'\n", effective_src, dest_path);
        free(src_copy);
        if (downloaded_path[0] != '\0') unlink(downloaded_path);
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

// Comando: instalar programa
void cmd_install(const char *src_path, const char *version) {
    cmd_install_with_options(src_path, version, NULL, NULL, NULL, NULL);
}

// Comando: eliminar programa
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
    
    // Verificar dependencias (si otros programas dependen de este)
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

    // Eliminar archivo
    if (unlink(full_path) != 0) {
        fprintf(stderr, "Error: No se pudo eliminar '%s'\n", name);
        return;
    }

    // Eliminar alias si existe
    if (has_meta && meta.alias[0] != '\0') {
        char alias_path[MAX_PATH];
        snprintf(alias_path, sizeof(alias_path), "%s/%s", install_dir, meta.alias);
        if (file_exists(alias_path)) {
            unlink(alias_path);
        }
    }
    
    // Eliminar metadata
    metadata_delete(name);
    
    printf("  Eliminado: %s\n", name);
}

// Comparador para ordenar programas
static int compare_programs_by_name(const void *a, const void *b) {
    const ProgramInfo *pa = (const ProgramInfo *)a;
    const ProgramInfo *pb = (const ProgramInfo *)b;
    return strcmp(pa->name, pb->name);
}

static int compare_programs_by_date(const void *a, const void *b) {
    const ProgramInfo *pa = (const ProgramInfo *)a;
    const ProgramInfo *pb = (const ProgramInfo *)b;
    return (pb->timestamp > pa->timestamp) - (pb->timestamp < pa->timestamp);
}

static int compare_programs_by_size(const void *a, const void *b) {
    const ProgramInfo *pa = (const ProgramInfo *)a;
    const ProgramInfo *pb = (const ProgramInfo *)b;
    return (pb->size_bytes > pa->size_bytes) - (pb->size_bytes < pa->size_bytes);
}

// Comando: listar programas
void cmd_list(const ListOptions *opts) {
    char install_dir[MAX_PATH];
    get_install_dir(install_dir, sizeof(install_dir));
    
    DIR *dir = opendir(install_dir);
    if (!dir) {
        printf("No hay programas instalados (directorio no existe)\n");
        return;
    }
    
    // Recolectar programas
    ProgramInfo programs[256];
    int count = 0;
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && count < 256) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        char full_path[MAX_PATH];
        snprintf(full_path, sizeof(full_path), "%s/%s", install_dir, entry->d_name);
        
        if (is_executable(full_path)) {
            struct stat st;
            if (stat(full_path, &st) == 0) {
                // Aplicar filtros
                if (opts) {
                    if (opts->before_date > 0 && st.st_mtime >= opts->before_date) {
                        continue;
                    }
                    if (opts->min_size > 0 && st.st_size < opts->min_size) {
                        continue;
                    }
                    if (opts->search_term[0] != '\0' && strstr(entry->d_name, opts->search_term) == NULL) {
                        continue;
                    }
                }
                
                strncpy(programs[count].name, entry->d_name, sizeof(programs[count].name) - 1);
                format_time(st.st_mtime, programs[count].date, sizeof(programs[count].date));
                format_size(st.st_size, programs[count].size, sizeof(programs[count].size));
                programs[count].size_bytes = st.st_size;
                programs[count].timestamp = st.st_mtime;
                count++;
            }
        }
    }
    closedir(dir);
    
    if (count == 0) {
        printf("No hay programas instalados\n");
        return;
    }
    
    // Ordenar
    if (opts) {
        switch (opts->sort) {
            case SORT_DATE:
                qsort(programs, count, sizeof(ProgramInfo), compare_programs_by_date);
                break;
            case SORT_SIZE:
                qsort(programs, count, sizeof(ProgramInfo), compare_programs_by_size);
                break;
            case SORT_NAME:
            default:
                qsort(programs, count, sizeof(ProgramInfo), compare_programs_by_name);
                break;
        }
    } else {
        qsort(programs, count, sizeof(ProgramInfo), compare_programs_by_name);
    }
    
    // Salida JSON
    if (opts && opts->json_output) {
        printf("[\n");
        for (int i = 0; i < count; i++) {
            printf("  {\n");
            printf("    \"name\": \"%s\",\n", programs[i].name);
            printf("    \"date\": \"%s\",\n", programs[i].date);
            printf("    \"size\": %lld,\n", programs[i].size_bytes);
            printf("    \"timestamp\": %ld\n", programs[i].timestamp);
            printf("  }%s\n", (i < count - 1) ? "," : "");
        }
        printf("]\n");
        return;
    }
    
    // Calcular anchos de columna
    size_t max_name_len = 8;
    for (int i = 0; i < count; i++) {
        size_t len = strlen(programs[i].name);
        if (len > max_name_len) max_name_len = len;
    }
    
    // Imprimir tabla
    printf(" Programas instalados en %s:\n\n", install_dir);
    
    printf("┌");
    for (size_t i = 0; i < max_name_len + 2; i++) printf("─");
    printf("┬─────────────────────┬─────────────┐\n");
    
    printf("│ %-*s │ Fecha instalación   │ Tamaño      │\n", (int)max_name_len, "Programa");
    
    printf("├");
    for (size_t i = 0; i < max_name_len + 2; i++) printf("─");
    printf("┼─────────────────────┼─────────────┤\n");
    
    for (int i = 0; i < count; i++) {
        printf("│ %-*s │ %s │ %11s │\n", 
               (int)max_name_len, 
               programs[i].name,
               programs[i].date,
               programs[i].size);
    }
    
    printf("└");
    for (size_t i = 0; i < max_name_len + 2; i++) printf("─");
    printf("┴─────────────────────┴─────────────┘\n");
    
    printf("\nTotal: %d programa(s)\n", count);
}

// Comando: mostrar info detallada de un programa
void cmd_info(const char *name) {
    char install_dir[MAX_PATH];
    get_install_dir(install_dir, sizeof(install_dir));
    
    char full_path[MAX_PATH];
    snprintf(full_path, sizeof(full_path), "%s/%s", install_dir, name);
    
    // Verificar que existe
    struct stat st;
    if (stat(full_path, &st) != 0) {
        fprintf(stderr, "Error: '%s' no está instalado\n", name);
        return;
    }
    
    printf(" Información de: %s\n", name);
    printf("═══════════════════════════════════════\n\n");
    
    // Información básica del archivo
    printf("Ubicación:    %s\n", full_path);
    
    char size_str[32];
    format_size(st.st_size, size_str, sizeof(size_str));
    printf("Tamaño:       %s (%lld bytes)\n", size_str, (long long)st.st_size);
    
    printf("Permisos:     %o\n", st.st_mode & 0777);
    
    char date_str[64];
    format_time(st.st_mtime, date_str, sizeof(date_str));
    printf("Modificado:   %s\n", date_str);
    
    // Calcular checksum actual
    char checksum[65];
    if (checksum_calculate_sha256(full_path, checksum, sizeof(checksum)) == 0) {
        printf("SHA256:       %s\n", checksum);
    }
    
    // Cargar metadata si existe
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
        
        // Verificar integridad
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
        
        // Dependencias
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

// Comando: actualizar programa
void cmd_update(const char *name, const char *src_path) {
    char install_dir[MAX_PATH];
    get_install_dir(install_dir, sizeof(install_dir));
    
    char dest_path[MAX_PATH];
    snprintf(dest_path, sizeof(dest_path), "%s/%s", install_dir, name);
    
    // Verificar que está instalado
    struct stat st;
    if (stat(dest_path, &st) != 0) {
        fprintf(stderr, "Error: '%s' no está instalado. Usa 'install' primero.\n", name);
        return;
    }
    
    // Verificar archivo fuente
    if (stat(src_path, &st) != 0) {
        fprintf(stderr, "Error: El archivo '%s' no existe\n", src_path);
        return;
    }
    
    printf(" Actualizando: %s\n", name);
    
    // Calcular checksums
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
        int hook_rc = run_hook_script(meta.pre_update_hook, name, dest_path, src_path);
        if (hook_rc != 0) {
            fprintf(stderr, "Error: pre-update hook falló (%d): %s\n", hook_rc, meta.pre_update_hook);
            return;
        }
    }

    // Crear backup
    char backup_dir[MAX_PATH];
    get_backups_dir(backup_dir, sizeof(backup_dir));
    ensure_dir(backup_dir);
    
    char backup_path[MAX_PATH];
    time_t now = time(NULL);
    snprintf(backup_path, sizeof(backup_path), "%s/%s.%ld.bak", backup_dir, name, now);
    
    if (copy_file(dest_path, backup_path) == 0) {
        printf("   Backup creado: %s\n", backup_path);
    }
    
    // Actualizar archivo
    if (copy_file(src_path, dest_path) != 0) {
        fprintf(stderr, "Error: No se pudo actualizar el archivo\n");
        return;
    }
    
    // Actualizar metadata
    if (has_meta) {
        strncpy(meta.checksum_sha256, new_checksum, sizeof(meta.checksum_sha256) - 1);
        meta.update_date = time(NULL);
        
        // Actualizar source_path
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
            int hook_rc = run_hook_script(meta.post_update_hook, name, dest_path, src_path);
            if (hook_rc != 0) {
                fprintf(stderr, "Advertencia: post-update hook falló (%d): %s\n", hook_rc, meta.post_update_hook);
            }
        }
    }
    
    printf(" Actualizado exitosamente\n");
    printf("   SHA256 anterior: %s\n", old_checksum);
    printf("   SHA256 nuevo:    %s\n", new_checksum);
}

// Comando: verificar integridad de un programa
void cmd_verify(const char *name) {
    char install_dir[MAX_PATH];
    get_install_dir(install_dir, sizeof(install_dir));
    
    char full_path[MAX_PATH];
    snprintf(full_path, sizeof(full_path), "%s/%s", install_dir, name);
    
    // Verificar que el archivo existe
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
        
        // Mostrar el checksum actual como referencia
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

// Comando: verificar todos
void cmd_verify_all(void) {
    checksum_verify_all();
}

// Comando: doctor (verificar sistema)
void cmd_doctor(void) {
    printf(" Verificando configuración de localbin...\n\n");
    
    char install_dir[MAX_PATH];
    get_install_dir(install_dir, sizeof(install_dir));
    
    // 1. Verificar directorio
    struct stat st;
    if (stat(install_dir, &st) == 0) {
        printf(" Directorio existe: %s\n", install_dir);
    } else {
        printf(" Directorio no existe: %s\n", install_dir);
    }
    
    // 2. Verificar PATH
    if (check_path()) {
        printf(" Directorio está en PATH\n");
    } else {
        printf(" Directorio NO está en PATH\n");
    }
    
    // 3. Contar programas
    DIR *dir = opendir(install_dir);
    if (dir) {
        int count = 0;
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            char full_path[MAX_PATH];
            snprintf(full_path, sizeof(full_path), "%s/%s", install_dir, entry->d_name);
            if (is_executable(full_path)) {
                count++;
            }
        }
        closedir(dir);
        printf(" Programas instalados: %d\n", count);
        
        // Verificar integridad
        printf("\n Verificando integridad...\n");
        checksum_verify_all();
    }
    
    // 4. Verificar permisos
    if (stat(install_dir, &st) == 0) {
        if (st.st_mode & S_IWUSR) {
            printf(" Permisos de escritura: OK\n");
        } else {
            printf(" Sin permisos de escritura\n");
        }
    }
    
    printf("\n");
}

// Comando: configurar PATH automáticamente
void cmd_setup(void) {
    printf(" Configurando PATH automáticamente...\n\n");
    
    const char *shell = getenv("SHELL");
    const int is_fish = shell && strstr(shell, "fish");
    const int is_csh = shell && (strstr(shell, "csh") || strstr(shell, "tcsh"));
    char config_path[MAX_PATH];
    get_shell_config_file(config_path, sizeof(config_path));
    
    // Verificar si ya está configurado
    FILE *f = fopen(config_path, "r");
    if (f) {
        char line[512];
        while (fgets(line, sizeof(line), f)) {
            if (strstr(line, ".localbin") && strstr(line, "PATH")) {
                printf(" PATH ya está configurado en %s\n", config_path);
                fclose(f);
                mark_configured();
                return;
            }
        }
        fclose(f);
    }
    
    // Para fish, asegurar que exista ~/.config/fish
    if (is_fish) {
        char fish_dir[MAX_PATH];
        snprintf(fish_dir, sizeof(fish_dir), "%s/.config/fish", get_home_dir());
        if (ensure_dir(fish_dir) != 0) {
            fprintf(stderr, " Error: No se pudo crear %s\n", fish_dir);
            return;
        }
    }

    // Agregar configuración
    f = fopen(config_path, "a");
    if (!f) {
        fprintf(stderr, " Error: No se pudo abrir %s\n", config_path);
        return;
    }
    
    fprintf(f, "\n# Agregado por localbin\n");
    if (is_fish) {
        fprintf(f, "set -gx PATH $HOME/.localbin $PATH\n");
    } else if (is_csh) {
        fprintf(f, "setenv PATH \"$HOME/.localbin:$PATH\"\n");
    } else {
        fprintf(f, "export PATH=\"$HOME/.localbin:$PATH\"\n");
    }
    fclose(f);
    
    printf(" PATH configurado en %s\n", config_path);
    printf("\n   Para aplicar: source %s\n", config_path);
    
    mark_configured();
}

// Comando: buscar programas
void cmd_search(const char *term) {
    ListOptions opts = {0};
    opts.sort = SORT_NAME;
    strncpy(opts.search_term, term, sizeof(opts.search_term) - 1);
    
    printf(" Buscando: %s\n\n", term);
    cmd_list(&opts);
}

static void print_title_banner(void) {
    printf("██╗      ██████╗  ██████╗ █████╗ ██╗     ██████╗ ██╗███╗   ██╗\n");
    printf("██║     ██╔═══██╗██╔════╝██╔══██╗██║     ██╔══██╗██║████╗  ██║\n");
    printf("██║     ██║   ██║██║     ███████║██║     ██████╔╝██║██╔██╗ ██║\n");
    printf("██║     ██║   ██║██║     ██╔══██║██║     ██╔══██╗██║██║╚██╗██║\n");
    printf("███████╗╚██████╔╝╚██████╗██║  ██║███████╗██████╔╝██║██║ ╚████║\n");
    printf("╚══════╝ ╚═════╝  ╚═════╝╚═╝  ╚═╝╚══════╝╚═════╝ ╚═╝╚═╝  ╚═══╝\n\n");
}

// Ayuda
void print_help(const char *prog_name) {
    print_title_banner();
    printf("localbin v%s - Administrador de paquetes local\n\n", LOCALBIN_VERSION);
    printf("Uso: %s <comando> [argumentos]\n\n", prog_name);
    printf("Comandos principales:\n");
    printf("  install <archivo|url> [opciones] Instala un ejecutable\n");
    printf("  update <nombre> <archivo>        Actualiza un programa\n");
    printf("  remove <nombre>                  Elimina un programa\n");
    printf("  list [--sort name|date|size]     Lista programas instalados\n");
    printf("  info <nombre>                    Muestra info detallada\n");
    printf("  search <término>                 Busca programas\n");
    printf("  verify <nombre>                  Verifica integridad\n");
    printf("  verify --all                     Verifica todos los programas\n\n");
    printf("Opciones de install:\n");
    printf("  --version V                      Guarda versión del binario\n");
    printf("  --as NOMBRE                      Instala con otro nombre\n");
    printf("  --alias NOMBRE                   Crea symlink adicional\n");
    printf("  --pre-update-hook SCRIPT         Ejecuta script antes de update\n");
    printf("  --post-update-hook SCRIPT        Ejecuta script después de update\n\n");
    printf("Gestión del sistema:\n");
    printf("  doctor                           Verifica configuración\n");
    printf("  setup                            Configura PATH automáticamente\n\n");
    printf("Otros comandos:\n");
    printf("  help                             Muestra esta ayuda\n");
    printf("  version                          Muestra la versión\n\n");
    printf("  self-update [--manual]           Actualiza localbin desde GitHub\n\n");
    printf("Los programas se instalan en: $HOME%s\n", INSTALL_DIR);
}

void print_version(void) {
    printf("localbin version %s\n", LOCALBIN_VERSION);
}

void cmd_self_update(int manual_mode) {
    const char *repo_raw_install = "https://raw.githubusercontent.com/svaldesoliva/localbin/main/install.sh";
    const char *repo_raw_version = "https://raw.githubusercontent.com/svaldesoliva/localbin/main/include/version.h";
    char latest_version[64] = {0};
    char cmd[4096];

    snprintf(cmd, sizeof(cmd), "curl -fsSL \"%s\" | awk -F'\"' '/^#define[[:space:]]+LOCALBIN_VERSION[[:space:]]+\"/{print $2; exit}'", repo_raw_version);
    FILE *f = popen(cmd, "r");
    if (f) {
        if (fgets(latest_version, sizeof(latest_version), f)) {
            size_t n = strlen(latest_version);
            while (n > 0 && (latest_version[n - 1] == '\n' || latest_version[n - 1] == '\r' || latest_version[n - 1] == ' ')) {
                latest_version[--n] = '\0';
            }
        }
        pclose(f);
    }

    if (!manual_mode && latest_version[0] != '\0' && strcmp(LOCALBIN_VERSION, latest_version) == 0) {
        printf("localbin ya está actualizado (v%s)\n", LOCALBIN_VERSION);
        return;
    }

    if (manual_mode) {
        printf("Actualización manual forzada\n");
    } else {
        printf("Buscando actualizaciones...\n");
    }
    if (latest_version[0] != '\0') {
        printf("Versión disponible: %s\n", latest_version);
    }

    snprintf(cmd, sizeof(cmd), "curl -fsSL \"%s\" | bash", repo_raw_install);
    int rc = system(cmd);
    if (rc != 0) {
        fprintf(stderr, "Error: no se pudo actualizar localbin\n");
        return;
    }

    printf("localbin actualizado correctamente\n");
}
