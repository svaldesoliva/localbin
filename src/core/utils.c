#include "localbin/core/utils.h"
#include "localbin/core/core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>

// Obtener directorio HOME
char* get_home_dir(void) {
    const char *home = getenv("HOME");
    if (!home) {
        fprintf(stderr, "Error: No se pudo obtener HOME\n");
        exit(1);
    }
    return (char*)home;
}

// Verificar si un archivo existe
int file_exists(const char *path) {
    return access(path, F_OK) == 0;
}

// Obtener tamaño de archivo
long long get_file_size(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) {
        return -1;
    }
    return st.st_size;
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

// Obtener archivo de configuración del shell
void get_shell_config_file(char *buf, size_t size) {
    const char *shell = getenv("SHELL");
    const char *config_file = NULL;
    
    if (shell && strstr(shell, "zsh")) {
        config_file = "/.zshrc";
    } else if (shell && strstr(shell, "bash")) {
        config_file = "/.bash_profile";
    } else if (shell && strstr(shell, "fish")) {
        config_file = "/.config/fish/config.fish";
    } else if (shell && (strstr(shell, "csh") || strstr(shell, "tcsh"))) {
        config_file = "/.cshrc";
    } else if (shell && (strstr(shell, "ksh") || strstr(shell, "dash") || strstr(shell, "sh"))) {
        config_file = "/.profile";
    } else {
        config_file = "/.profile";
    }
    
    snprintf(buf, size, "%s%s", get_home_dir(), config_file);
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
    } else if (shell && strstr(shell, "fish")) {
        printf("   mkdir -p ~/.config/fish\n");
        printf("   echo 'set -gx PATH $HOME/.localbin $PATH' >> ~/.config/fish/config.fish\n");
        printf("   source ~/.config/fish/config.fish\n");
    } else if (shell && (strstr(shell, "csh") || strstr(shell, "tcsh"))) {
        printf("   echo 'setenv PATH \"$HOME/.localbin:$PATH\"' >> ~/.cshrc\n");
        printf("   source ~/.cshrc\n");
    } else {
        printf("   echo 'export PATH=\"$HOME/.localbin:$PATH\"' >> ~/.profile\n");
        printf("   source ~/.profile\n");
    }
    
    printf("\n   O ejecuta: localbin setup (para configuración automática)\n");
    
    mark_configured();
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
