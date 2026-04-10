#include "localbin/cli/commands.h"
#include "localbin/core/core.h"
#include "localbin/core/utils.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

typedef int (*program_compare_fn)(const void *a, const void *b);

static int compare_programs_by_name(const void *a, const void *b) {
    const ProgramInfo *pa = (const ProgramInfo *)a;
    const ProgramInfo *pb = (const ProgramInfo *)b;
    return strcmp(pa->name, pb->name);
}

static int compare_programs_by_date(const void *a, const void *b) {
    const ProgramInfo *pa = (const ProgramInfo *)a;
    const ProgramInfo *pb = (const ProgramInfo *)b;
    return (pb->timestamp > pa->timestamp) - (pb->timestamp < pa->timestamp);
}

static int compare_programs_by_size(const void *a, const void *b) {
    const ProgramInfo *pa = (const ProgramInfo *)a;
    const ProgramInfo *pb = (const ProgramInfo *)b;
    return (pb->size_bytes > pa->size_bytes) - (pb->size_bytes < pa->size_bytes);
}

static program_compare_fn get_sort_comparator(const ListOptions *opts) {
    if (!opts) {
        return compare_programs_by_name;
    }

    switch (opts->sort) {
        case SORT_DATE:
            return compare_programs_by_date;
        case SORT_SIZE:
            return compare_programs_by_size;
        case SORT_NAME:
        default:
            return compare_programs_by_name;
    }
}

static int should_skip_program(const ListOptions *opts, const char *name, const struct stat *st) {
    if (!opts) {
        return 0;
    }
    if (opts->before_date > 0 && st->st_mtime >= opts->before_date) {
        return 1;
    }
    if (opts->min_size > 0 && st->st_size < opts->min_size) {
        return 1;
    }
    if (opts->search_term[0] != '\0' && strstr(name, opts->search_term) == NULL) {
        return 1;
    }
    return 0;
}

static int collect_programs(const char *install_dir, const ListOptions *opts, ProgramInfo *programs, int max_programs) {
    DIR *dir = opendir(install_dir);
    if (!dir) {
        return -1;
    }

    int count = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && count < max_programs) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char full_path[MAX_PATH];
        snprintf(full_path, sizeof(full_path), "%s/%s", install_dir, entry->d_name);
        if (!is_executable(full_path)) {
            continue;
        }

        struct stat st;
        if (stat(full_path, &st) != 0 || should_skip_program(opts, entry->d_name, &st)) {
            continue;
        }

        memset(&programs[count], 0, sizeof(programs[count]));
        strncpy(programs[count].name, entry->d_name, sizeof(programs[count].name) - 1);
        format_time(st.st_mtime, programs[count].date, sizeof(programs[count].date));
        format_size(st.st_size, programs[count].size, sizeof(programs[count].size));
        programs[count].size_bytes = st.st_size;
        programs[count].timestamp = st.st_mtime;
        count++;
    }

    closedir(dir);
    return count;
}

static void print_programs_json(const ProgramInfo *programs, int count) {
    printf("[\n");
    for (int i = 0; i < count; i++) {
        printf("  {\n");
        printf("    \"name\": \"%s\",\n", programs[i].name);
        printf("    \"date\": \"%s\",\n", programs[i].date);
        printf("    \"size\": %lld,\n", programs[i].size_bytes);
        printf("    \"timestamp\": %ld\n", programs[i].timestamp);
        printf("  }%s\n", (i < count - 1) ? "," : "");
    }
    printf("]\n");
}

static size_t get_max_name_len(const ProgramInfo *programs, int count) {
    size_t max_name_len = 8;
    for (int i = 0; i < count; i++) {
        size_t len = strlen(programs[i].name);
        if (len > max_name_len) {
            max_name_len = len;
        }
    }
    return max_name_len;
}

static void print_programs_table(const char *install_dir, const ProgramInfo *programs, int count) {
    size_t max_name_len = get_max_name_len(programs, count);

    printf(" Programas instalados en %s:\n\n", install_dir);

    printf("┌");
    for (size_t i = 0; i < max_name_len + 2; i++) {
        printf("─");
    }
    printf("┬─────────────────────┬─────────────┐\n");

    printf("│ %-*s │ Fecha instalación   │ Tamaño      │\n", (int)max_name_len, "Programa");

    printf("├");
    for (size_t i = 0; i < max_name_len + 2; i++) {
        printf("─");
    }
    printf("┼─────────────────────┼─────────────┤\n");

    for (int i = 0; i < count; i++) {
        printf("│ %-*s │ %s │ %11s │\n", (int)max_name_len, programs[i].name, programs[i].date, programs[i].size);
    }

    printf("└");
    for (size_t i = 0; i < max_name_len + 2; i++) {
        printf("─");
    }
    printf("┴─────────────────────┴─────────────┘\n");

    printf("\nTotal: %d programa(s)\n", count);
}

void cmd_list(const ListOptions *opts) {
    char install_dir[MAX_PATH];
    get_install_dir(install_dir, sizeof(install_dir));

    ProgramInfo programs[256];
    int count = collect_programs(install_dir, opts, programs, 256);
    if (count < 0) {
        printf("No hay programas instalados (directorio no existe)\n");
        return;
    }
    if (count == 0) {
        printf("No hay programas instalados\n");
        return;
    }

    qsort(programs, count, sizeof(ProgramInfo), get_sort_comparator(opts));

    if (opts && opts->json_output) {
        print_programs_json(programs, count);
        return;
    }

    print_programs_table(install_dir, programs, count);
}

void cmd_search(const char *term) {
    ListOptions opts = {0};
    opts.sort = SORT_NAME;
    strncpy(opts.search_term, term, sizeof(opts.search_term) - 1);

    printf(" Buscando: %s\n\n", term);
    cmd_list(&opts);
}
