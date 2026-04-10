#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>
#include <dirent.h>
#include <libgen.h>

#define INSTALL_DIR "/.localbin"
#define CONFIG_FLAG "/.localbin/.configured"
#define MAX_PATH 1024

// Obtener ruta completa del directorio de instalación
void get_install_dir(char *buf, size_t size) {
    const char *home = getenv("HOME");
    if (!home) {
        fprintf(stderr, "Error: No se pudo obtener HOME\n");
        exit(1);
    }
    snprintf(buf, size, "%s%s", home, INSTALL_DIR);
}

// Obtener ruta del flag de configuración
void get_config_flag(char *buf, size_t size) {
    const char *home = getenv("HOME");
    if (!home) {
        fprintf(stderr, "Error: No se pudo obtener HOME\n");
        exit(1);
    }
    snprintf(buf, size, "%s%s", home, CONFIG_FLAG);
}

// Verificar si ya fue configurado
int is_configured(void) {
    char flag_path[MAX_PATH];
    get_config_flag(flag_path, sizeof(flag_path));
    return access(flag_path, F_OK) == 0;
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

// Crear directorio si no existe
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

// Copiar archivo con permisos de ejecución
int copy_file(const char *src, const char *dst) {
    FILE *fsrc = fopen(src, "rb");
    if (!fsrc) {
        return -1;
    }
    
    FILE *fdst = fopen(dst, "wb");
    if (!fdst) {
        fclose(fsrc);
        return -1;
    }
    
    char buf[8192];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), fsrc)) > 0) {
        if (fwrite(buf, 1, n, fdst) != n) {
            fclose(fsrc);
            fclose(fdst);
            return -1;
        }
    }
    
    fclose(fsrc);
    fclose(fdst);
    
    // Hacer ejecutable
    chmod(dst, 0755);
    return 0;
}

// Verificar si un archivo es ejecutable (binario)
int is_executable(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) {
        return 0;
    }
    
    // Debe ser archivo regular
    if (!S_ISREG(st.st_mode)) {
        return 0;
    }
    
    // Debe tener permisos de ejecución
    if (!(st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH))) {
        return 0;
    }
    
    // Verificar que no sea archivo de sistema oculto
    const char *base = strrchr(path, '/');
    base = base ? base + 1 : path;
    if (base[0] == '.') {
        return 0;
    }
    
    return 1;
}

// Verificar si ~/.localbin está en PATH
int check_path(void) {
    const char *path_env = getenv("PATH");
    if (!path_env) {
        return 0;
    }
    
    char install_dir[MAX_PATH];
    get_install_dir(install_dir, sizeof(install_dir));
    
    char *path_copy = strdup(path_env);
    char *token = strtok(path_copy, ":");
    
    while (token != NULL) {
        if (strcmp(token, install_dir) == 0) {
            free(path_copy);
            return 1;
        }
        token = strtok(NULL, ":");
    }
    
    free(path_copy);
    return 0;
}

// Mostrar advertencia de PATH
void warn_path(void) {
    if (is_configured()) {
        return;  // Ya fue advertido antes
    }
    
    if (check_path()) {
        mark_configured();
        return;  // PATH ya está bien configurado
    }
    
    char install_dir[MAX_PATH];
    get_install_dir(install_dir, sizeof(install_dir));
    
    printf("\n  ADVERTENCIA: %s no está en tu PATH\n", install_dir);
    printf("   Los programas instalados no serán accesibles globalmente.\n");
    printf("   \n");
    printf("   Para corregir, ejecuta:\n");
    
    const char *shell = getenv("SHELL");
    if (shell && strstr(shell, "zsh")) {
        printf("   echo 'export PATH=\"$HOME/.localbin:$PATH\"' >> ~/.zshrc\n");
        printf("   source ~/.zshrc\n");
    } else if (shell && strstr(shell, "bash")) {
        printf("   echo 'export PATH=\"$HOME/.localbin:$PATH\"' >> ~/.bash_profile\n");
        printf("   source ~/.bash_profile\n");
    } else {
        printf("   export PATH=\"$HOME/.localbin:$PATH\"\n");
    }
    
    printf("\n   O ejecuta: localbin setup (para configuración automática)\n");
    
    mark_configured();
}

// Instalar programa
void install_program(const char *src_path) {
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
    
    // Copiar archivo
    if (copy_file(src_path, dest_path) != 0) {
        fprintf(stderr, "Error: No se pudo copiar '%s' a '%s'\n", src_path, dest_path);
        free(src_copy);
        return;
    }
    
    printf("Instalado: %s\n", base);
    printf("Ruta: %s\n", dest_path);
    
    free(src_copy);
    
    // Verificar PATH (solo primera vez)
    warn_path();
}

// Formatear timestamp
void format_time(time_t t, char *buf, size_t size) {
    struct tm *tm_info = localtime(&t);
    strftime(buf, size, "%Y-%m-%d %H:%M:%S", tm_info);
}

// Formatear tamaño de archivo de forma óptima
void format_size(long long bytes, char *buf, size_t size) {
    const double KB = 1024.0;
    const double MB = KB * 1024.0;
    const double GB = MB * 1024.0;
    
    if (bytes >= GB) {
        snprintf(buf, size, "%.2f GB", bytes / GB);
    } else if (bytes >= MB) {
        snprintf(buf, size, "%.2f MB", bytes / MB);
    } else if (bytes >= KB) {
        snprintf(buf, size, "%.2f KB", bytes / KB);
    } else {
        snprintf(buf, size, "%lld bytes", bytes);
    }
}

// Estructura para almacenar info de programas
typedef struct {
    char name[256];
    char date[64];
    char size[32];
    long long size_bytes;
} ProgramInfo;

// Comparador para ordenar por nombre
int compare_programs(const void *a, const void *b) {
    const ProgramInfo *pa = (const ProgramInfo *)a;
    const ProgramInfo *pb = (const ProgramInfo *)b;
    return strcmp(pa->name, pb->name);
}

// Listar programas instalados en formato tabla
void list_programs(void) {
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
                strncpy(programs[count].name, entry->d_name, sizeof(programs[count].name) - 1);
                format_time(st.st_mtime, programs[count].date, sizeof(programs[count].date));
                format_size(st.st_size, programs[count].size, sizeof(programs[count].size));
                programs[count].size_bytes = st.st_size;
                count++;
            }
        }
    }
    closedir(dir);
    
    if (count == 0) {
        printf("No hay programas instalados\n");
        return;
    }
    
    // Ordenar por nombre
    qsort(programs, count, sizeof(ProgramInfo), compare_programs);
    
    // Calcular anchos de columna
    size_t max_name_len = 8; // Mínimo "Programa"
    for (int i = 0; i < count; i++) {
        size_t len = strlen(programs[i].name);
        if (len > max_name_len) max_name_len = len;
    }
    
    // Imprimir header
    printf(" Programas instalados en %s:\n\n", install_dir);
    
    printf("┌");
    for (size_t i = 0; i < max_name_len + 2; i++) printf("─");
    printf("┬─────────────────────┬─────────────┐\n");
    
    printf("│ %-*s │ Fecha instalación   │ Tamaño      │\n", (int)max_name_len, "Programa");
    
    printf("├");
    for (size_t i = 0; i < max_name_len + 2; i++) printf("─");
    printf("┼─────────────────────┼─────────────┤\n");
    
    // Imprimir filas
    for (int i = 0; i < count; i++) {
        printf("│ %-*s │ %s │ %11s │\n", 
               (int)max_name_len, 
               programs[i].name,
               programs[i].date,
               programs[i].size);
    }
    
    // Footer
    printf("└");
    for (size_t i = 0; i < max_name_len + 2; i++) printf("─");
    printf("┴─────────────────────┴─────────────┘\n");
    
    printf("\nTotal: %d programa(s)\n", count);
}

// Eliminar programa
void remove_program(const char *name) {
    char install_dir[MAX_PATH];
    get_install_dir(install_dir, sizeof(install_dir));
    
    char full_path[MAX_PATH];
    snprintf(full_path, sizeof(full_path), "%s/%s", install_dir, name);
    
    struct stat st;
    if (stat(full_path, &st) != 0) {
        fprintf(stderr, "Error: '%s' no está instalado\n", name);
        return;
    }
    
    if (unlink(full_path) != 0) {
        fprintf(stderr, "Error: No se pudo eliminar '%s': %s\n", name, strerror(errno));
        return;
    }
    
    printf("  Eliminado: %s\n", name);
}

// Verificar estado del entorno
void check_environment(void) {
    printf(" Verificando configuración de localbin...\n\n");
    
    char install_dir[MAX_PATH];
    get_install_dir(install_dir, sizeof(install_dir));
    
    // 1. Verificar si el directorio existe
    struct stat st;
    if (stat(install_dir, &st) == 0) {
        printf(" Directorio existe: %s\n", install_dir);
    } else {
        printf(" Directorio no existe: %s\n", install_dir);
        printf("   Se creará automáticamente al instalar el primer programa\n");
    }
    
    // 2. Verificar PATH
    if (check_path()) {
        printf(" Directorio está en PATH\n");
    } else {
        printf(" Directorio NO está en PATH\n");
        printf("\n   Para corregir, agrega esta línea a tu shell config:\n");
        
        const char *shell = getenv("SHELL");
        if (shell && strstr(shell, "zsh")) {
            printf("   echo 'export PATH=\"$HOME/.localbin:$PATH\"' >> ~/.zshrc\n");
            printf("   source ~/.zshrc\n");
        } else if (shell && strstr(shell, "bash")) {
            printf("   echo 'export PATH=\"$HOME/.localbin:$PATH\"' >> ~/.bash_profile\n");
            printf("   source ~/.bash_profile\n");
        } else {
            printf("   export PATH=\"$HOME/.localbin:$PATH\"\n");
        }
        printf("\n   O ejecuta: localbin setup\n");
    }
    
    // 3. Contar programas instalados (solo ejecutables)
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
    }
    
    // 4. Verificar permisos del directorio
    if (stat(install_dir, &st) == 0) {
        if (st.st_mode & S_IWUSR) {
            printf(" Permisos de escritura: OK\n");
        } else {
            printf(" Sin permisos de escritura en %s\n", install_dir);
        }
    }
    
    // 5. Verificar shell
    const char *shell = getenv("SHELL");
    if (shell) {
        printf(" Shell detectado: %s\n", shell);
    } else {
        printf("  No se pudo detectar el shell\n");
    }
    
    printf("\n");
}

// Configuración automática del PATH
void auto_configure_path(void) {
    printf(" Configurando PATH automáticamente...\n\n");
    
    const char *shell = getenv("SHELL");
    const char *config_file = NULL;
    
    if (shell && strstr(shell, "zsh")) {
        config_file = "/.zshrc";
    } else if (shell && strstr(shell, "bash")) {
        config_file = "/.bash_profile";
    } else {
        fprintf(stderr, " Shell no soportado para configuración automática\n");
        fprintf(stderr, "   Shells soportados: zsh, bash\n");
        return;
    }
    
    char config_path[MAX_PATH];
    snprintf(config_path, sizeof(config_path), "%s%s", getenv("HOME"), config_file);
    
    // Verificar si ya está configurado
    FILE *f = fopen(config_path, "r");
    if (f) {
        char line[512];
        int found = 0;
        while (fgets(line, sizeof(line), f)) {
            if (strstr(line, ".localbin") && strstr(line, "PATH")) {
                found = 1;
                break;
            }
        }
        fclose(f);
        
        if (found) {
            printf(" PATH ya está configurado en %s\n", config_path);
            printf("   No se requiere ninguna acción adicional\n");
            mark_configured();
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
    fprintf(f, "export PATH=\"$HOME/.localbin:$PATH\"\n");
    fclose(f);
    
    printf(" PATH configurado en %s\n", config_path);
    printf("   \n");
    printf("   Para aplicar los cambios, ejecuta:\n");
    printf("   source %s\n", config_path);
    printf("   \n");
    printf("   O reinicia tu terminal\n");
    
    mark_configured();
}

// Mostrar ayuda
void print_help(const char *prog_name) {
    printf("localbin - Administrador de paquetes local\n\n");
    printf("Uso:\n");
    printf("  %s install <archivo>  Instala un ejecutable\n", prog_name);
    printf("  %s list               Lista programas instalados\n", prog_name);
    printf("  %s remove <nombre>    Elimina un programa\n", prog_name);
    printf("  %s doctor             Verifica configuración del sistema\n", prog_name);
    printf("  %s setup              Configura PATH automáticamente\n", prog_name);
    printf("  %s help               Muestra esta ayuda\n\n", prog_name);
    printf("Alias:\n");
    printf("  ls    -> list\n");
    printf("  rm    -> remove\n");
    printf("  check -> doctor\n\n");
    printf("Los programas se instalan en: $HOME%s\n", INSTALL_DIR);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_help(argv[0]);
        return 1;
    }
    
    const char *cmd = argv[1];
    
    if (strcmp(cmd, "install") == 0) {
        if (argc != 3) {
            fprintf(stderr, "Uso: %s install <archivo>\n", argv[0]);
            return 1;
        }
        install_program(argv[2]);
    } 
    else if (strcmp(cmd, "list") == 0 || strcmp(cmd, "ls") == 0) {
        list_programs();
    } 
    else if (strcmp(cmd, "remove") == 0 || strcmp(cmd, "rm") == 0) {
        if (argc != 3) {
            fprintf(stderr, "Uso: %s remove <nombre>\n", argv[0]);
            return 1;
        }
        remove_program(argv[2]);
    }
    else if (strcmp(cmd, "doctor") == 0 || strcmp(cmd, "check") == 0) {
        check_environment();
    }
    else if (strcmp(cmd, "setup") == 0) {
        auto_configure_path();
    }
    else if (strcmp(cmd, "help") == 0 || strcmp(cmd, "-h") == 0 || strcmp(cmd, "--help") == 0) {
        print_help(argv[0]);
    } 
    else {
        fprintf(stderr, "Comando desconocido: %s\n", cmd);
        fprintf(stderr, "Usa '%s help' para ver los comandos disponibles\n", argv[0]);
        return 1;
    }
    
    return 0;
}
