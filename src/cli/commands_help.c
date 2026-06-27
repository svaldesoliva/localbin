#include "localbin/cli/commands.h"
#include "localbin/app/version.h"
#include <stdio.h>

static void banner(void) {
    puts("██╗      ██████╗  ██████╗ █████╗ ██╗     ██████╗ ██╗███╗   ██╗");
    puts("██║     ██╔═══██╗██╔════╝██╔══██╗██║     ██╔══██╗██║████╗  ██║");
    puts("██║     ██║   ██║██║     ███████║██║     ██████╔╝██║██╔██╗ ██║");
    puts("██║     ██║   ██║██║     ██╔══██║██║     ██╔══██╗██║██║╚██╗██║");
    puts("███████╗╚██████╔╝╚██████╗██║  ██║███████╗██████╔╝██║██║ ╚████║");
    puts("╚══════╝ ╚═════╝  ╚═════╝╚═╝  ╚═╝╚══════╝╚═════╝ ╚═╝╚═╝  ╚═══╝\n");
}

void print_help(const char *prog) {
    banner();
    printf("localbin v%s — local binary manager\n\n", LOCALBIN_VERSION);
    printf("Usage: %s <command> [args]\n\n", prog);
    puts("Commands:");
    puts("  install <file|url>  [--version V] [--as NAME] [--alias NAME]");
    puts("                      [--pre-update-hook S] [--post-update-hook S]");
    puts("  update  <name> <file>");
    puts("  remove  <name>");
    puts("  rename  <old> <new>");
    puts("  which   <name>");
    puts("  list    [--sort name|date|size] [--json]");
    puts("  info    <name>");
    puts("  search  <term>");
    puts("  verify  [<name>|--all]");
    puts("  doctor");
    puts("  setup");
    puts("  self-update [--manual]");
    puts("  version");
    puts("  help");
}

void print_version(void) { printf("localbin %s\n", LOCALBIN_VERSION); }
