#include "localbin/core/core.h"
#include "localbin/cli/commands.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_help(argv[0]);
        return 1;
    }
    
    const char *cmd = argv[1];
    
    // Comandos principales
    if (strcmp(cmd, "install") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Uso: %s install <archivo|url> [--version VERSION] [--as NOMBRE] [--alias NOMBRE] [--pre-update-hook SCRIPT] [--post-update-hook SCRIPT]\n", argv[0]);
            return 1;
        }
        
        const char *version = NULL;
        const char *as_name = NULL;
        const char *alias = NULL;
        const char *pre_hook = NULL;
        const char *post_hook = NULL;

        for (int i = 3; i < argc; i++) {
            if (strcmp(argv[i], "--version") == 0 && i + 1 < argc) {
                version = argv[++i];
            } else if (strcmp(argv[i], "--as") == 0 && i + 1 < argc) {
                as_name = argv[++i];
            } else if (strcmp(argv[i], "--alias") == 0 && i + 1 < argc) {
                alias = argv[++i];
            } else if (strcmp(argv[i], "--pre-update-hook") == 0 && i + 1 < argc) {
                pre_hook = argv[++i];
            } else if (strcmp(argv[i], "--post-update-hook") == 0 && i + 1 < argc) {
                post_hook = argv[++i];
            } else {
                fprintf(stderr, "Opción inválida para install: %s\n", argv[i]);
                return 1;
            }
        }
        
        cmd_install_with_options(argv[2], version, as_name, alias, pre_hook, post_hook);
    }
    else if (strcmp(cmd, "update") == 0) {
        if (argc != 4) {
            fprintf(stderr, "Uso: %s update <nombre> <archivo>\n", argv[0]);
            return 1;
        }
        cmd_update(argv[2], argv[3]);
    }
    else if (strcmp(cmd, "remove") == 0 || strcmp(cmd, "rm") == 0) {
        if (argc != 3) {
            fprintf(stderr, "Uso: %s remove <nombre>\n", argv[0]);
            return 1;
        }
        cmd_remove(argv[2]);
    }
    else if (strcmp(cmd, "list") == 0 || strcmp(cmd, "ls") == 0) {
        ListOptions opts = {0};
        opts.sort = SORT_NAME;
        opts.json_output = 0;
        
        // Parsear opciones
        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "--sort") == 0 && i + 1 < argc) {
                i++;
                if (strcmp(argv[i], "date") == 0) opts.sort = SORT_DATE;
                else if (strcmp(argv[i], "size") == 0) opts.sort = SORT_SIZE;
                else if (strcmp(argv[i], "name") == 0) opts.sort = SORT_NAME;
            }
            else if (strcmp(argv[i], "--json") == 0) {
                opts.json_output = 1;
            }
        }
        
        cmd_list(&opts);
    }
    else if (strcmp(cmd, "info") == 0) {
        if (argc != 3) {
            fprintf(stderr, "Uso: %s info <nombre>\n", argv[0]);
            return 1;
        }
        cmd_info(argv[2]);
    }
    else if (strcmp(cmd, "search") == 0) {
        if (argc != 3) {
            fprintf(stderr, "Uso: %s search <término>\n", argv[0]);
            return 1;
        }
        cmd_search(argv[2]);
    }
    else if (strcmp(cmd, "verify") == 0) {
        if (argc == 2 || (argc == 3 && strcmp(argv[2], "--all") == 0)) {
            cmd_verify_all();
        } else if (argc == 3) {
            cmd_verify(argv[2]);
        } else {
            fprintf(stderr, "Uso: %s verify [<nombre>|--all]\n", argv[0]);
            return 1;
        }
    }
    else if (strcmp(cmd, "doctor") == 0 || strcmp(cmd, "check") == 0) {
        cmd_doctor();
    }
    else if (strcmp(cmd, "setup") == 0) {
        cmd_setup();
    }
    else if (strcmp(cmd, "self-update") == 0 || strcmp(cmd, "selfupdate") == 0) {
        int manual_mode = 0;
        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "--manual") == 0) {
                manual_mode = 1;
            } else {
                fprintf(stderr, "Opción inválida para self-update: %s\n", argv[i]);
                return 1;
            }
        }
        cmd_self_update(manual_mode);
    }
    else if (strcmp(cmd, "version") == 0 || strcmp(cmd, "-v") == 0 || strcmp(cmd, "--version") == 0) {
        print_version();
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
