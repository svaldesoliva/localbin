#ifndef COMMANDS_INTERNAL_H
#define COMMANDS_INTERNAL_H

#include <stddef.h>

int cli_is_url_path(const char *value);
int cli_download_to_temp(const char *url, char *out_path, size_t out_size);
int cli_run_hook_script(const char *hook_path, const char *program_name, const char *target_path, const char *source_path);

#endif
