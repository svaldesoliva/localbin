#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>  // Para size_t
#include <sys/types.h>

// Funciones de utilidad general
int copy_file(const char *src, const char *dst);
int is_executable(const char *path);
int check_path(void);
void warn_path(void);
void format_time(time_t t, char *buf, size_t size);
void format_size(long long bytes, char *buf, size_t size);
char* get_home_dir(void);
void get_shell_config_file(char *buf, size_t size);
int file_exists(const char *path);
long long get_file_size(const char *path);

#endif // UTILS_H
