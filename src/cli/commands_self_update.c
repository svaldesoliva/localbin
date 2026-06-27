#include "localbin/cli/commands.h"
#include "localbin/app/version.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void cmd_self_update(int manual_mode) {
    const char *url_version =
        "https://raw.githubusercontent.com/svaldesoliva/localbin/main/include/localbin/app/version.h";
    const char *url_install =
        "https://raw.githubusercontent.com/svaldesoliva/localbin/main/install.sh";

    /* Fetch the remote version string */
    char latest[64] = {0};
    char cmd[4096];
    snprintf(cmd, sizeof(cmd),
        "curl -fsSL \"%s\" | awk -F'\"' '/^#define[[:space:]]+LOCALBIN_VERSION[[:space:]]+\"/{print $2; exit}'",
        url_version);
    FILE *f = popen(cmd, "r");
    if (f) {
        if (fgets(latest, sizeof(latest), f)) {
            /* Strip trailing whitespace */
            for (int n = strlen(latest) - 1; n >= 0 && (latest[n] == '\n' || latest[n] == '\r' || latest[n] == ' '); n--)
                latest[n] = '\0';
        }
        pclose(f);
    }

    if (!manual_mode && latest[0] && strcmp(LOCALBIN_VERSION, latest) == 0) {
        printf("  localbin %s is already up to date\n", LOCALBIN_VERSION);
        return;
    }

    if (latest[0])
        printf("  %s -> %s\n", LOCALBIN_VERSION, latest);
    else
        printf("  Updating localbin (could not fetch remote version)\n");

    snprintf(cmd, sizeof(cmd), "curl -fSL -# \"%s\" | bash", url_install);
    if (system(cmd) != 0) {
        fprintf(stderr, "  Error: update failed\n");
        return;
    }
    printf("  Updated. Restart your shell.\n");
}
