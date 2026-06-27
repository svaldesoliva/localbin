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
    snprintf(buf, size, "%s/%s.env", dir, name);
}

int metadata_save(const ProgramMetadata *meta) {
    char dir[MAX_PATH];
    get_metadata_dir(dir, sizeof(dir));
    if (ensure_dir(dir) != 0) return -1;

    char path[MAX_PATH];
    metadata_path(meta->name, path, sizeof(path));

    FILE *f = fopen(path, "w");
    if (!f) return -1;

    fprintf(f, "name=%s\n", meta->name);
    fprintf(f, "version=%s\n", meta->version);
    fprintf(f, "source_path=%s\n", meta->source_path);
    fprintf(f, "alias=%s\n", meta->alias);
    fprintf(f, "pre_update_hook=%s\n", meta->pre_update_hook);
    fprintf(f, "post_update_hook=%s\n", meta->post_update_hook);
    fprintf(f, "checksum_sha256=%s\n", meta->checksum_sha256);
    fprintf(f, "install_date=%ld\n", (long)meta->install_date);
    fprintf(f, "update_date=%ld\n", (long)meta->update_date);
    fprintf(f, "size_bytes=%lld\n", meta->size_bytes);
    fprintf(f, "permissions=%o\n", meta->permissions);

    fclose(f);
    return 0;
}

int metadata_load(const char *name, ProgramMetadata *meta) {
    char path[MAX_PATH];
    metadata_path(name, path, sizeof(path));

    FILE *f = fopen(path, "r");
    if (!f) return -1;

    memset(meta, 0, sizeof(*meta));
    char line[2048];
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = 0;
        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq++ = 0;
        if (strcmp(line, "name") == 0) strncpy(meta->name, eq, sizeof(meta->name)-1);
        else if (strcmp(line, "version") == 0) strncpy(meta->version, eq, sizeof(meta->version)-1);
        else if (strcmp(line, "source_path") == 0) strncpy(meta->source_path, eq, sizeof(meta->source_path)-1);
        else if (strcmp(line, "alias") == 0) strncpy(meta->alias, eq, sizeof(meta->alias)-1);
        else if (strcmp(line, "pre_update_hook") == 0) strncpy(meta->pre_update_hook, eq, sizeof(meta->pre_update_hook)-1);
        else if (strcmp(line, "post_update_hook") == 0) strncpy(meta->post_update_hook, eq, sizeof(meta->post_update_hook)-1);
        else if (strcmp(line, "checksum_sha256") == 0) strncpy(meta->checksum_sha256, eq, sizeof(meta->checksum_sha256)-1);
        else if (strcmp(line, "install_date") == 0) meta->install_date = (time_t)atol(eq);
        else if (strcmp(line, "update_date") == 0) meta->update_date = (time_t)atol(eq);
        else if (strcmp(line, "size_bytes") == 0) meta->size_bytes = atoll(eq);
        else if (strcmp(line, "permissions") == 0) {
            unsigned int tmp;
            sscanf(eq, "%o", &tmp);
            meta->permissions = (mode_t)tmp;
        }
    }

    fclose(f);
    return 0;
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

    int n = 0, cap = 16;
    char (*names)[256] = malloc(cap * sizeof(*names));
    struct dirent *e;
    while ((e = readdir(d))) {
        char *dot = strrchr(e->d_name, '.');
        if (!dot || strcmp(dot, ".env") != 0) continue;
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
