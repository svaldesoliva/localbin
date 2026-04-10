#include "localbin/cli/commands.h"
#include "localbin/app/version.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void cmd_self_update(int manual_mode) {
    const char *repo_raw_install = "https://raw.githubusercontent.com/svaldesoliva/localbin/main/install.sh";
    const char *repo_raw_version = "https://raw.githubusercontent.com/svaldesoliva/localbin/main/include/localbin/app/version.h";
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
