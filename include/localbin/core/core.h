#ifndef CORE_H
#define CORE_H

#include <stddef.h>
#include <time.h>
#include <sys/types.h>

#define INSTALL_DIR  "/.localbin"
#define CONFIG_FLAG  "/.localbin/.configured"
#define METADATA_DIR "/.localbin/.metadata"
#define MAX_PATH     1024

typedef struct {
    char     name[256];
    char     version[32];
    char     source_path[MAX_PATH];
    char     alias[256];
    char     pre_update_hook[MAX_PATH];
    char     post_update_hook[MAX_PATH];
    char     checksum_sha256[65];
    time_t   install_date;
    time_t   update_date;
    long long size_bytes;
    mode_t   permissions;
} ProgramMetadata;

typedef struct {
    char      name[256];
    char      date[64];
    char      size[32];
    long long size_bytes;
    time_t    timestamp;
} ProgramInfo;

typedef enum { SORT_NAME, SORT_DATE, SORT_SIZE } SortMode;

typedef struct {
    SortMode sort;
    char     search_term[256];
    int      json_output;
} ListOptions;

/* core/paths.c */
void get_install_dir(char *buf, size_t size);
void get_metadata_dir(char *buf, size_t size);
void get_config_flag(char *buf, size_t size);
int  is_configured(void);
void mark_configured(void);
int  ensure_dir(const char *path);

#endif // CORE_H
