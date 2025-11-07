#include "core.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

// Obtener ruta completa del directorio de instalación
void get_install_dir(char *buf, size_t size) {
    const char *home = get_home_dir();
    snprintf(buf, size, "%s%s", home, INSTALL_DIR);
}

// Obtener ruta del directorio de metadata
void get_metadata_dir(char *buf, size_t size) {
    const char *home = get_home_dir();
    snprintf(buf, size, "%s%s", home, METADATA_DIR);
}

// Obtener ruta del directorio de versiones
void get_versions_dir(char *buf, size_t size) {
    const char *home = get_home_dir();
    snprintf(buf, size, "%s%s", home, VERSIONS_DIR);
}

// Obtener ruta del directorio de backups
void get_backups_dir(char *buf, size_t size) {
    const char *home = get_home_dir();
    snprintf(buf, size, "%s%s", home, BACKUPS_DIR);
}

// Obtener ruta del flag de configuración
void get_config_flag(char *buf, size_t size) {
    const char *home = get_home_dir();
    snprintf(buf, size, "%s%s", home, CONFIG_FLAG);
}

// Verificar si ya fue configurado
int is_configured(void) {
    char flag_path[MAX_PATH];
    get_config_flag(flag_path, sizeof(flag_path));
    return file_exists(flag_path);
}

// Marcar como configurado
void mark_configured(void) {
    char flag_path[MAX_PATH];
    get_config_flag(flag_path, sizeof(flag_path));
    FILE *f = fopen(flag_path, "w");
    if (f) {
        fprintf(f, "configured\n");
        fclose(f);
    }
}

// Crear directorio si no existe (recursivo)
int ensure_dir(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        return S_ISDIR(st.st_mode) ? 0 : -1;
    }
    
    char tmp[MAX_PATH];
    snprintf(tmp, sizeof(tmp), "%s", path);
    
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
                return -1;
            }
            *p = '/';
        }
    }
    
    return mkdir(tmp, 0755) == 0 || errno == EEXIST ? 0 : -1;
}
