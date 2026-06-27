#include "localbin/cli/commands.h"
#include "localbin/core/core.h"
#include "localbin/core/utils.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static int cmp_name(const void *a, const void *b) {
    return strcmp(((const ProgramInfo *)a)->name, ((const ProgramInfo *)b)->name);
}
static int cmp_date(const void *a, const void *b) {
    time_t ta = ((const ProgramInfo *)a)->timestamp;
    time_t tb = ((const ProgramInfo *)b)->timestamp;
    return (tb > ta) - (tb < ta);
}
static int cmp_size(const void *a, const void *b) {
    long long sa = ((const ProgramInfo *)a)->size_bytes;
    long long sb = ((const ProgramInfo *)b)->size_bytes;
    return (sb > sa) - (sb < sa);
}

static int collect_programs(const char *dir, const ListOptions *opts,
                            ProgramInfo *out, int max) {
    DIR *d = opendir(dir);
    if (!d) return -1;

    int count = 0;
    struct dirent *e;
    while ((e = readdir(d)) && count < max) {
        char path[MAX_PATH];
        snprintf(path, sizeof(path), "%s/%s", dir, e->d_name);
        if (!is_executable(path)) continue;

        struct stat st;
        if (stat(path, &st) != 0) continue;

        if (opts && opts->search_term[0] != '\0'
                && strstr(e->d_name, opts->search_term) == NULL) continue;

        ProgramInfo *pi = &out[count++];
        memset(pi, 0, sizeof(*pi));
        strncpy(pi->name, e->d_name, sizeof(pi->name) - 1);
        format_time(st.st_mtime, pi->date, sizeof(pi->date));
        format_size(st.st_size,  pi->size, sizeof(pi->size));
        pi->size_bytes = st.st_size;
        pi->timestamp  = st.st_mtime;
    }
    closedir(d);
    return count;
}

static void print_json(const ProgramInfo *p, int count) {
    printf("[\n");
    for (int i = 0; i < count; i++) {
        printf("  {\"name\":\"%s\",\"date\":\"%s\",\"size\":%lld,\"timestamp\":%ld}%s\n",
               p[i].name, p[i].date, p[i].size_bytes, (long)p[i].timestamp,
               i < count - 1 ? "," : "");
    }
    printf("]\n");
}

static void print_table(const char *dir, const ProgramInfo *p, int count) {
    size_t w = 8;
    for (int i = 0; i < count; i++) {
        size_t l = strlen(p[i].name);
        if (l > w) w = l;
    }

    printf("  Programs in %s:\n\n", dir);
    printf("┌─");  for (size_t i = 0; i < w; i++) printf("─"); printf("─┬─────────────────────┬─────────────┐\n");
    printf("│ %-*s │ Installed           │ Size        │\n", (int)w, "Program");
    printf("├─");  for (size_t i = 0; i < w; i++) printf("─"); printf("─┼─────────────────────┼─────────────┤\n");
    for (int i = 0; i < count; i++)
        printf("│ %-*s │ %s │ %11s │\n", (int)w, p[i].name, p[i].date, p[i].size);
    printf("└─");  for (size_t i = 0; i < w; i++) printf("─"); printf("─┴─────────────────────┴─────────────┘\n");
    printf("\nTotal: %d program(s)\n", count);
}

void cmd_list(const ListOptions *opts) {
    char dir[MAX_PATH];
    get_install_dir(dir, sizeof(dir));

    ProgramInfo programs[256];
    int count = collect_programs(dir, opts, programs, 256);
    if (count <= 0) { printf("No programs installed\n"); return; }

    int (*cmp)(const void *, const void *) = cmp_name;
    if (opts) {
        if (opts->sort == SORT_DATE) cmp = cmp_date;
        else if (opts->sort == SORT_SIZE) cmp = cmp_size;
    }
    qsort(programs, count, sizeof(*programs), cmp);

    if (opts && opts->json_output) { print_json(programs, count); return; }
    print_table(dir, programs, count);
}

void cmd_search(const char *term) {
    ListOptions opts = {0};
    opts.sort = SORT_NAME;
    strncpy(opts.search_term, term, sizeof(opts.search_term) - 1);
    printf("  Searching: %s\n\n", term);
    cmd_list(&opts);
}
