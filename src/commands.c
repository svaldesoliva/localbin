#include "commands.h"
#include "core.h"
#include "utils.h"
#include "metadata.h"
#include "checksum.h"
#include "version.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <libgen.h>
#include <time.h>
#include <dirent.h>

// Comando: instalar programa
void cmd_install(const char *src_path, const char *version) {
    struct stat st;
    
    // Verificar que el archivo fuente existe
    if (stat(src_path, &st) != 0) {
        fprintf(stderr, "Error: El archivo '%s' no existe\n", src_path);
        return;
    }
    
    // Verificar que es un archivo regular
    if (!S_ISREG(st.st_mode)) {
        fprintf(stderr, "Error: '%s' no es un archivo regular\n", src_path);
        return;
    }
    
    char install_dir[MAX_PATH];
    get_install_dir(install_dir, sizeof(install_dir));
    
    if (ensure_dir(install_dir) != 0) {
        fprintf(stderr, "Error: No se pudo crear el directorio %s\n", install_dir);
        return;
    }
    
    // Obtener nombre base del archivo
    char *src_copy = strdup(src_path);
    char *base = basename(src_copy);
    
    char dest_path[MAX_PATH];
    snprintf(dest_path, sizeof(dest_path), "%s/%s", install_dir, base);
    
    // Verificar si ya existe
    if (file_exists(dest_path)) {
        fprintf(stderr, "⚠️  '%s' ya está instalado. Usa 'update' para actualizarlo.\n", base);
        free(src_copy);
        return;
    }
    
    // Calcular checksum del archivo fuente
    char checksum[65];
    if (checksum_calculate_sha256(src_path, checksum, sizeof(checksum)) != 0) {
        fprintf(stderr, "Error: No se pudo calcular checksum\n");
        free(src_copy);
        return;
    }
    
    // Copiar archivo
    if (copy_file(src_path, dest_path) != 0) {
        fprintf(stderr, "Error: No se pudo copiar '%s' a '%s'\n", src_path, dest_path);
        free(src_copy);
        return;
    }
    
    // Crear metadata
    ProgramMetadata meta;
    memset(&meta, 0, sizeof(meta));
    strncpy(meta.name, base, sizeof(meta.name) - 1);
    strncpy(meta.version, version ? version : "1.0.0", sizeof(meta.version) - 1);
    
    // Convertir src_path a ruta absoluta
    char abs_src_path[MAX_PATH];
    if (realpath(src_path, abs_src_path) != NULL) {
        strncpy(meta.source_path, abs_src_path, sizeof(meta.source_path) - 1);
    } else {
        strncpy(meta.source_path, src_path, sizeof(meta.source_path) - 1);
    }
    
    strncpy(meta.checksum_sha256, checksum, sizeof(meta.checksum_sha256) - 1);
    meta.install_date = time(NULL);
    meta.update_date = meta.install_date;
    meta.size_bytes = st.st_size;
    meta.permissions = st.st_mode;
    meta.dep_count = 0;
    
    // Guardar metadata
    if (metadata_save(&meta) != 0) {
        fprintf(stderr, "⚠️  Instalado pero no se pudo guardar metadata\n");
    }
    
    printf("✅ Instalado: %s", base);
    if (version) {
        printf(" (v%s)", version);
    }
    printf("\n");
    printf("   Ruta: %s\n", dest_path);
    printf("   SHA256: %s\n", checksum);
    
    char size_str[32];
    format_size(st.st_size, size_str, sizeof(size_str));
    printf("   Tamaño: %s\n", size_str);
    
    free(src_copy);
    
    // Verificar PATH (solo primera vez)
    warn_path();
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
                        printf("⚠️  Otros programas dependen de '%s':\n", name);
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
    
    // Eliminar archivo
    if (unlink(full_path) != 0) {
        fprintf(stderr, "Error: No se pudo eliminar '%s'\n", name);
        return;
    }
    
    // Eliminar metadata
    metadata_delete(name);
    
    printf("🗑️  Eliminado: %s\n", name);
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
    printf("📦 Programas instalados en %s:\n\n", install_dir);
    
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
    
    printf("📋 Información de: %s\n", name);
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
        printf("\n📊 Metadata:\n");
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
        
        // Verificar integridad
        if (meta.checksum_sha256[0] != '\0') {
            int verify = checksum_verify_file(full_path, meta.checksum_sha256);
            if (verify == 0) {
                printf("Integridad:   ✅ OK (checksum coincide)\n");
            } else {
                printf("Integridad:   ❌ MODIFICADO (checksum no coincide)\n");
                printf("  Esperado:   %s\n", meta.checksum_sha256);
                printf("  Actual:     %s\n", checksum);
            }
        }
        
        // Dependencias
        if (meta.dep_count > 0) {
            printf("\n🔗 Dependencias:\n");
            for (int i = 0; i < meta.dep_count; i++) {
                printf("  - %s\n", meta.dependencies[i]);
            }
        }
    } else {
        printf("\n⚠️  No hay metadata disponible para este programa\n");
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
    
    printf("🔄 Actualizando: %s\n", name);
    
    // Calcular checksums
    char old_checksum[65], new_checksum[65];
    checksum_calculate_sha256(dest_path, old_checksum, sizeof(old_checksum));
    checksum_calculate_sha256(src_path, new_checksum, sizeof(new_checksum));
    
    if (strcmp(old_checksum, new_checksum) == 0) {
        printf("✅ El programa ya está actualizado (mismo checksum)\n");
        return;
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
    ProgramMetadata meta;
    if (metadata_load(name, &meta) == 0) {
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
    }
    
    printf("✅ Actualizado exitosamente\n");
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
        fprintf(stderr, "❌ Error: El programa '%s' no está instalado\n", name);
        return;
    }
    
    ProgramMetadata meta;
    if (metadata_load(name, &meta) != 0) {
        fprintf(stderr, "⚠️  Advertencia: No hay metadata para '%s'\n", name);
        fprintf(stderr, "   Este programa fue instalado antes del sistema de metadata.\n");
        fprintf(stderr, "   Para generar metadata, reinstala el programa:\n");
        fprintf(stderr, "   localbin update %s /ruta/al/binario\n", name);
        
        // Mostrar el checksum actual como referencia
        char actual[65];
        if (checksum_calculate_sha256(full_path, actual, sizeof(actual)) == 0) {
            printf("\n📋 Checksum actual (SHA256):\n");
            printf("   %s\n", actual);
        }
        return;
    }
    
    printf("🔍 Verificando: %s\n", name);
    
    int result = checksum_verify_file(full_path, meta.checksum_sha256);
    
    if (result == 0) {
        printf("✅ OK - Checksum coincide\n");
    } else if (result == 1) {
        printf("❌ MODIFICADO - Checksum no coincide\n");
        
        char actual[65];
        checksum_calculate_sha256(full_path, actual, sizeof(actual));
        printf("   Esperado: %s\n", meta.checksum_sha256);
        printf("   Actual:   %s\n", actual);
    } else {
        printf("⚠️  ERROR - No se pudo verificar\n");
    }
}

// Comando: verificar todos
void cmd_verify_all(void) {
    checksum_verify_all();
}

// Comando: doctor (verificar sistema)
void cmd_doctor(void) {
    printf("🔍 Verificando configuración de localbin...\n\n");
    
    char install_dir[MAX_PATH];
    get_install_dir(install_dir, sizeof(install_dir));
    
    // 1. Verificar directorio
    struct stat st;
    if (stat(install_dir, &st) == 0) {
        printf("✅ Directorio existe: %s\n", install_dir);
    } else {
        printf("❌ Directorio no existe: %s\n", install_dir);
    }
    
    // 2. Verificar PATH
    if (check_path()) {
        printf("✅ Directorio está en PATH\n");
    } else {
        printf("❌ Directorio NO está en PATH\n");
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
        printf("✅ Programas instalados: %d\n", count);
        
        // Verificar integridad
        printf("\n🔐 Verificando integridad...\n");
        checksum_verify_all();
    }
    
    // 4. Verificar permisos
    if (stat(install_dir, &st) == 0) {
        if (st.st_mode & S_IWUSR) {
            printf("✅ Permisos de escritura: OK\n");
        } else {
            printf("❌ Sin permisos de escritura\n");
        }
    }
    
    printf("\n");
}

// Comando: configurar PATH automáticamente
void cmd_setup(void) {
    printf("🔧 Configurando PATH automáticamente...\n\n");
    
    char config_path[MAX_PATH];
    get_shell_config_file(config_path, sizeof(config_path));
    
    if (config_path[0] == '\0') {
        fprintf(stderr, "❌ Shell no soportado\n");
        return;
    }
    
    // Verificar si ya está configurado
    FILE *f = fopen(config_path, "r");
    if (f) {
        char line[512];
        while (fgets(line, sizeof(line), f)) {
            if (strstr(line, ".localbin") && strstr(line, "PATH")) {
                printf("✅ PATH ya está configurado en %s\n", config_path);
                fclose(f);
                mark_configured();
                return;
            }
        }
        fclose(f);
    }
    
    // Agregar configuración
    f = fopen(config_path, "a");
    if (!f) {
        fprintf(stderr, "❌ Error: No se pudo abrir %s\n", config_path);
        return;
    }
    
    fprintf(f, "\n# Agregado por localbin\n");
    fprintf(f, "export PATH=\"$HOME/.localbin:$PATH\"\n");
    fclose(f);
    
    printf("✅ PATH configurado en %s\n", config_path);
    printf("\n   Para aplicar: source %s\n", config_path);
    
    mark_configured();
}

// Comando: buscar programas
void cmd_search(const char *term) {
    ListOptions opts = {0};
    opts.sort = SORT_NAME;
    strncpy(opts.search_term, term, sizeof(opts.search_term) - 1);
    
    printf("🔍 Buscando: %s\n\n", term);
    cmd_list(&opts);
}

// Ayuda
void print_help(const char *prog_name) {
    printf("localbin v%s - Administrador de paquetes local\n\n", LOCALBIN_VERSION);
    printf("Uso: %s <comando> [argumentos]\n\n", prog_name);
    printf("Comandos principales:\n");
    printf("  install <archivo> [--version V]  Instala un ejecutable\n");
    printf("  update <nombre> <archivo>        Actualiza un programa\n");
    printf("  remove <nombre>                  Elimina un programa\n");
    printf("  list [--sort name|date|size]     Lista programas instalados\n");
    printf("  info <nombre>                    Muestra info detallada\n");
    printf("  search <término>                 Busca programas\n");
    printf("  verify <nombre>                  Verifica integridad\n");
    printf("  verify --all                     Verifica todos los programas\n\n");
    printf("Gestión del sistema:\n");
    printf("  doctor                           Verifica configuración\n");
    printf("  setup                            Configura PATH automáticamente\n\n");
    printf("Otros comandos:\n");
    printf("  help                             Muestra esta ayuda\n");
    printf("  version                          Muestra la versión\n\n");
    printf("Los programas se instalan en: $HOME%s\n", INSTALL_DIR);
}

void print_version(void) {
    printf("localbin version %s\n", LOCALBIN_VERSION);
}
