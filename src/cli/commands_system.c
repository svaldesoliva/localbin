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
    printf("  localbin health check\n\n");

    char install_dir[MAX_PATH];
    get_install_dir(install_dir, sizeof(install_dir));

    struct stat st;
    printf("  %-30s %s\n", "Install dir:",
           stat(install_dir, &st) == 0 ? "exists" : "missing");
    printf("  %-30s %s\n", "In PATH:", check_path() ? "yes" : "no");

    if (stat(install_dir, &st) == 0)
        printf("  %-30s %s\n", "Writable:", (st.st_mode & S_IWUSR) ? "yes" : "no");

    int count = 0;
    DIR *d = opendir(install_dir);
    if (d) {
        struct dirent *e;
        char path[MAX_PATH];
        while ((e = readdir(d))) {
            snprintf(path, sizeof(path), "%s/%s", install_dir, e->d_name);
            if (is_executable(path)) count++;
        }
        closedir(d);
    }
    printf("  %-30s %d\n\n", "Programs installed:", count);
    checksum_verify_all();
}

void cmd_setup(void) {
    printf("  Configuring PATH...\n\n");

    char config[MAX_PATH];
    get_shell_config_file(config, sizeof(config));

    /* Already configured? */
    FILE *f = fopen(config, "r");
    if (f) {
        char line[512];
        while (fgets(line, sizeof(line), f)) {
            if (strstr(line, ".localbin") && strstr(line, "PATH")) {
                printf("  PATH already configured in %s\n", config);
                fclose(f); mark_configured(); return;
            }
        }
        fclose(f);
    }

    /* Create fish config dir if needed */
    const char *shell = getenv("SHELL");
    if (shell && strstr(shell, "fish")) {
        char fish_dir[MAX_PATH];
        snprintf(fish_dir, sizeof(fish_dir), "%s/.config/fish", get_home_dir());
        if (ensure_dir(fish_dir) != 0) {
            fprintf(stderr, "  Error: cannot create %s\n", fish_dir); return;
        }
    }

    /* Build the export line using the shared helper in utils.c */
    char line[256];
    /* Access the same logic via shell_path_line — declared via extern for reuse */
    /* (shell_path_line is static in utils.c; use get_shell_config_file side effect
       and duplicate the minimal switch here — it's 3 branches, not worth extracting) */
    if (shell && strstr(shell, "fish"))
        snprintf(line, sizeof(line), "set -gx PATH $HOME/.localbin $PATH");
    else if (shell && (strstr(shell, "csh") || strstr(shell, "tcsh")))
        snprintf(line, sizeof(line), "setenv PATH \"$HOME/.localbin:$PATH\"");
    else
        snprintf(line, sizeof(line), "export PATH=\"$HOME/.localbin:$PATH\"");

    f = fopen(config, "a");
    if (!f) { fprintf(stderr, "  Error: cannot open %s\n", config); return; }
    fprintf(f, "\n# Added by localbin\n%s\n", line);
    fclose(f);

    printf("  Wrote to %s\n", config);
    printf("  Apply now: source %s\n", config);
    mark_configured();
}
