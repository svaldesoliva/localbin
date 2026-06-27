#include "localbin/storage/metadata.h"
#include "localbin/core/utils.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void metadata_path(const char *name, char *buf, size_t size) {
    char dir[MAX_PATH];
    get_metadata_dir(dir, sizeof(dir));
    snprintf(buf, size, "%s/%s.json", dir, name);
}

/* --- JSON serialization (hand-rolled; no external dependency) --- */

static char *to_json(const ProgramMetadata *m) {
    /* Upper bound: fixed fields + dep array. 4 KB covers any realistic value. */
    size_t cap = 4096 + (size_t)m->dep_count * 260;
    char *j = malloc(cap);
    if (!j) return NULL;

    int n = 0;
    n += snprintf(j + n, cap - n,
        "{\n"
        "  \"name\": \"%s\",\n"
        "  \"version\": \"%s\",\n"
        "  \"source_path\": \"%s\",\n"
        "  \"alias\": \"%s\",\n"
        "  \"pre_update_hook\": \"%s\",\n"
        "  \"post_update_hook\": \"%s\",\n"
        "  \"checksum_sha256\": \"%s\",\n"
        "  \"install_date\": %ld,\n"
        "  \"update_date\": %ld,\n"
        "  \"size_bytes\": %lld,\n"
        "  \"permissions\": %o,\n"
        "  \"dep_count\": %d,\n"
        "  \"dependencies\": [",
        m->name, m->version, m->source_path, m->alias,
        m->pre_update_hook, m->post_update_hook, m->checksum_sha256,
        (long)m->install_date, (long)m->update_date,
        m->size_bytes, m->permissions, m->dep_count);

    for (int i = 0; i < m->dep_count; i++)
        n += snprintf(j + n, cap - n, "%s\"%s\"", i ? "," : "", m->dependencies[i]);

    snprintf(j + n, cap - n, "]\n}\n");
    return j;
}

static int from_json(const char *j, ProgramMetadata *m) {
    memset(m, 0, sizeof(*m));

    struct { const char *key; void *dst; char fmt; size_t size; } fields[] = {
        { "\"name\":",             m->name,             's', sizeof(m->name)             },
        { "\"version\":",          m->version,          's', sizeof(m->version)          },
        { "\"source_path\":",      m->source_path,      's', sizeof(m->source_path)      },
        { "\"alias\":",            m->alias,            's', sizeof(m->alias)            },
        { "\"pre_update_hook\":",  m->pre_update_hook,  's', sizeof(m->pre_update_hook)  },
        { "\"post_update_hook\":", m->post_update_hook, 's', sizeof(m->post_update_hook) },
        { "\"checksum_sha256\":",  m->checksum_sha256,  's', sizeof(m->checksum_sha256)  },
    };

    for (size_t i = 0; i < sizeof(fields)/sizeof(*fields); i++) {
        const char *p = strstr(j, fields[i].key);
        if (!p) continue;
        p += strlen(fields[i].key);             /* skip key */
        p = strchr(p, '"'); if (!p) continue;   /* opening quote of value */
        p++;
        char fmt[32];
        snprintf(fmt, sizeof(fmt), "%%%zu[^\"]", fields[i].size - 1);
        sscanf(p, fmt, (char *)fields[i].dst);
    }

    /* Numeric fields */
    const char *p;
    long tmp_l;
    if ((p = strstr(j, "\"install_date\":"))) { sscanf(p + 15, " %ld", &tmp_l); m->install_date = tmp_l; }
    if ((p = strstr(j, "\"update_date\":")))  { sscanf(p + 14, " %ld", &tmp_l); m->update_date  = tmp_l; }
    if ((p = strstr(j, "\"size_bytes\":")))   sscanf(p + 13, " %lld", &m->size_bytes);
    if ((p = strstr(j, "\"permissions\":"))) { unsigned int tmp; sscanf(p + 14, " %o", &tmp); m->permissions = (mode_t)tmp; }
    if ((p = strstr(j, "\"dep_count\":")))    sscanf(p + 12, " %d",   &m->dep_count);

    /* Parse dependencies array */
    if ((p = strstr(j, "\"dependencies\":"))) {
        p = strchr(p, '[');
        for (int i = 0; i < m->dep_count && p; i++) {
            p = strchr(p, '"'); if (!p) break;
            p++;
            sscanf(p, "%255[^\"]", m->dependencies[i]);
            p = strchr(p, '"');
        }
    }

    return 0;
}

/* --- Public API --- */

int metadata_save(const ProgramMetadata *meta) {
    char dir[MAX_PATH];
    get_metadata_dir(dir, sizeof(dir));
    if (ensure_dir(dir) != 0) return -1;

    char path[MAX_PATH];
    metadata_path(meta->name, path, sizeof(path));

    char *json = to_json(meta);
    if (!json) return -1;

    FILE *f = fopen(path, "w");
    if (!f) { free(json); return -1; }
    fputs(json, f);
    fclose(f);
    free(json);
    return 0;
}

int metadata_load(const char *name, ProgramMetadata *meta) {
    char path[MAX_PATH];
    metadata_path(name, path, sizeof(path));

    FILE *f = fopen(path, "r");
    if (!f) return -1;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    char *json = malloc(size + 1);
    if (!json) { fclose(f); return -1; }
    fread(json, 1, size, f);
    json[size] = '\0';
    fclose(f);

    int rc = from_json(json, meta);
    free(json);
    return rc;
}

int metadata_delete(const char *name) {
    char path[MAX_PATH];
    metadata_path(name, path, sizeof(path));
    return unlink(path) == 0 ? 0 : -1;
}

int metadata_list_all(ProgramMetadata **list, int *count) {
    char dir[MAX_PATH];
    get_metadata_dir(dir, sizeof(dir));

    DIR *d = opendir(dir);
    if (!d) { *list = NULL; *count = 0; return 0; }

    /* Single pass: count and collect names. */
    int n = 0, cap = 16;
    char (*names)[256] = malloc(cap * sizeof(*names));
    struct dirent *e;
    while ((e = readdir(d))) {
        char *dot = strrchr(e->d_name, '.');
        if (!dot || strcmp(dot, ".json") != 0) continue;
        if (n == cap) { cap *= 2; names = realloc(names, cap * sizeof(*names)); }
        size_t nlen = (size_t)(dot - e->d_name);
        memcpy(names[n], e->d_name, nlen);
        names[n][nlen] = '\0';
        n++;
    }
    closedir(d);

    ProgramMetadata *programs = malloc(n * sizeof(*programs));
    int loaded = 0;
    for (int i = 0; i < n; i++)
        if (metadata_load(names[i], &programs[loaded]) == 0) loaded++;

    free(names);
    *list  = programs;
    *count = loaded;
    return 0;
}

void metadata_free_list(ProgramMetadata *list) { free(list); }
