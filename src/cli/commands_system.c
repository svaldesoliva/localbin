#include "localbin/cli/commands.h"
#include "localbin/core/core.h"
#include "localbin/core/utils.h"
#include "localbin/security/checksum.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

void cmd_doctor(void) {
    printf(" Verificando configuración de localbin...\n\n");

    char install_dir[MAX_PATH];
    get_install_dir(install_dir, sizeof(install_dir));

    struct stat st;
    if (stat(install_dir, &st) == 0) {
        printf(" Directorio existe: %s\n", install_dir);
    } else {
        printf(" Directorio no existe: %s\n", install_dir);
    }

    if (check_path()) {
        printf(" Directorio está en PATH\n");
    } else {
        printf(" Directorio NO está en PATH\n");
    }

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

        printf("\n Verificando integridad...\n");
        checksum_verify_all();
    }

    if (stat(install_dir, &st) == 0) {
        if (st.st_mode & S_IWUSR) {
            printf(" Permisos de escritura: OK\n");
        } else {
            printf(" Sin permisos de escritura\n");
        }
    }

    printf("\n");
}

void cmd_setup(void) {
    printf(" Configurando PATH automáticamente...\n\n");

    const char *shell = getenv("SHELL");
    const int is_fish = shell && strstr(shell, "fish");
    const int is_csh = shell && (strstr(shell, "csh") || strstr(shell, "tcsh"));
    char config_path[MAX_PATH];
    get_shell_config_file(config_path, sizeof(config_path));

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

    if (is_fish) {
        char fish_dir[MAX_PATH];
        snprintf(fish_dir, sizeof(fish_dir), "%s/.config/fish", get_home_dir());
        if (ensure_dir(fish_dir) != 0) {
            fprintf(stderr, " Error: No se pudo crear %s\n", fish_dir);
            return;
        }
    }

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
