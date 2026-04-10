#include "localbin/cli/commands.h"
#include "localbin/app/version.h"
#include "localbin/core/core.h"
#include <stdio.h>

static void print_title_banner(void) {
    printf("██╗      ██████╗  ██████╗ █████╗ ██╗     ██████╗ ██╗███╗   ██╗\n");
    printf("██║     ██╔═══██╗██╔════╝██╔══██╗██║     ██╔══██╗██║████╗  ██║\n");
    printf("██║     ██║   ██║██║     ███████║██║     ██████╔╝██║██╔██╗ ██║\n");
    printf("██║     ██║   ██║██║     ██╔══██║██║     ██╔══██╗██║██║╚██╗██║\n");
    printf("███████╗╚██████╔╝╚██████╗██║  ██║███████╗██████╔╝██║██║ ╚████║\n");
    printf("╚══════╝ ╚═════╝  ╚═════╝╚═╝  ╚═╝╚══════╝╚═════╝ ╚═╝╚═╝  ╚═══╝\n\n");
}

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
