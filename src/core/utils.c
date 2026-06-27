#include "localbin/core/utils.h"
#include "localbin/core/core.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

char *get_home_dir(void) {
    const char *home = getenv("HOME");
    if (!home) { fputs("Error: HOME not set\n", stderr); exit(1); }
    return (char *)home;
}

int file_exists(const char *path) { return access(path, F_OK) == 0; }

int copy_file(const char *src, const char *dst) {
    FILE *in = fopen(src, "rb");
    if (!in) return -1;

    unlink(dst); // Important: remove old file first to prevent macOS code signing SIGKILL
    FILE *out = fopen(dst, "wb");
    if (!out) { fclose(in); return -1; }

    char buf[65536];
    size_t n;
    int err = 0;
    while (!err && (n = fread(buf, 1, sizeof(buf), in)) > 0)
        if (fwrite(buf, 1, n, out) != n) err = 1;

    fclose(in);
    fclose(out);
    if (!err) chmod(dst, 0755);
    return err ? -1 : 0;
}

int is_executable(const char *path) {
    if (access(path, X_OK) != 0) return 0;
    
    struct stat st;
    if (stat(path, &st) != 0 || !S_ISREG(st.st_mode)) return 0;
    
    const char *base = strrchr(path, '/');
    char first = base ? base[1] : path[0];
    return first != '.';
}

/* Returns the export snippet for the current shell (no trailing newline). */
static void shell_path_line(const char *shell, char *buf, size_t size) {
    if (shell && strstr(shell, "fish"))
        snprintf(buf, size, "set -gx PATH $HOME/.localbin $PATH");
    else if (shell && (strstr(shell, "csh") || strstr(shell, "tcsh")))
        snprintf(buf, size, "setenv PATH \"$HOME/.localbin:$PATH\"");
    else
        snprintf(buf, size, "export PATH=\"$HOME/.localbin:$PATH\"");
}

void get_shell_config_file(char *buf, size_t size) {
    const char *shell = getenv("SHELL");
    const char *rc;
    if      (shell && strstr(shell, "zsh"))               rc = "/.zshrc";
    else if (shell && strstr(shell, "bash"))              rc = "/.bash_profile";
    else if (shell && strstr(shell, "fish"))              rc = "/.config/fish/config.fish";
    else if (shell && (strstr(shell, "csh") || strstr(shell, "tcsh"))) rc = "/.cshrc";
    else                                                   rc = "/.profile";
    snprintf(buf, size, "%s%s", get_home_dir(), rc);
}

int check_path(void) {
    const char *path_env = getenv("PATH");
    if (!path_env) return 0;

    char install_dir[MAX_PATH];
    get_install_dir(install_dir, sizeof(install_dir));

    /* Walk PATH without modifying it — compare segment by segment. */
    const char *p = path_env;
    while (*p) {
        const char *end = strchr(p, ':');
        if (!end) end = p + strlen(p);
        size_t len = (size_t)(end - p);
        if (len == strlen(install_dir) && memcmp(p, install_dir, len) == 0)
            return 1;
        p = *end ? end + 1 : end;
    }
    return 0;
}

void warn_path(void) {
    if (is_configured() || check_path()) { mark_configured(); return; }

    char install_dir[MAX_PATH];
    get_install_dir(install_dir, sizeof(install_dir));

    const char *shell = getenv("SHELL");
    char line[256];
    shell_path_line(shell, line, sizeof(line));

    char config[MAX_PATH];
    get_shell_config_file(config, sizeof(config));

    printf("\n  WARNING: %s is not in your PATH\n", install_dir);
    printf("  To fix, run:\n");
    printf("    echo '%s' >> %s && source %s\n\n", line, config, config);
    printf("  Or run: localbin setup\n\n");

    mark_configured();
}

void format_time(time_t t, char *buf, size_t size) {
    struct tm *tm_info = localtime(&t);
    strftime(buf, size, "%Y-%m-%d %H:%M:%S", tm_info);
}

void format_size(long long bytes, char *buf, size_t size) {
    static const char *units[] = { "bytes", "KB", "MB", "GB" };
    double v = (double)bytes;
    int u = 0;
    while (u < 3 && v >= 1024.0) { v /= 1024.0; u++; }
    if (u == 0) snprintf(buf, size, "%lld bytes", bytes);
    else        snprintf(buf, size, "%.2f %s", v, units[u]);
}
