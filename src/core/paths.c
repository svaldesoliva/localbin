#include "localbin/core/core.h"
#include "localbin/core/utils.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static void build_path(char *buf, size_t size, const char *suffix) {
    snprintf(buf, size, "%s%s", get_home_dir(), suffix);
}

void get_install_dir(char *buf, size_t size)  { build_path(buf, size, INSTALL_DIR);  }
void get_metadata_dir(char *buf, size_t size) { build_path(buf, size, METADATA_DIR); }
void get_config_flag(char *buf, size_t size)  { build_path(buf, size, CONFIG_FLAG);  }

int is_configured(void) {
    char flag[MAX_PATH];
    get_config_flag(flag, sizeof(flag));
    return file_exists(flag);
}

void mark_configured(void) {
    char flag[MAX_PATH];
    get_config_flag(flag, sizeof(flag));
    FILE *f = fopen(flag, "w");
    if (f) { fputs("configured\n", f); fclose(f); }
}

int ensure_dir(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0)
        return S_ISDIR(st.st_mode) ? 0 : -1;

    char tmp[MAX_PATH];
    snprintf(tmp, sizeof(tmp), "%s", path);
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (mkdir(tmp, 0755) != 0 && errno != EEXIST) return -1;
            *p = '/';
        }
    }
    return (mkdir(tmp, 0755) == 0 || errno == EEXIST) ? 0 : -1;
}
