#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <time.h>

int   copy_file(const char *src, const char *dst);
int   is_executable(const char *path);
int   check_path(void);
void  warn_path(void);
void  format_time(time_t t, char *buf, size_t size);
void  format_size(long long bytes, char *buf, size_t size);
char *get_home_dir(void);
void  get_shell_config_file(char *buf, size_t size);
int   file_exists(const char *path);

#define COLOR_GREEN "\x1b[32m"
#define COLOR_RED   "\x1b[31m"
#define COLOR_BLUE  "\x1b[34m"
#define COLOR_DIM   "\x1b[2m"
#define COLOR_RESET "\x1b[0m"

#endif // UTILS_H
