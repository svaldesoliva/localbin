#ifndef COMMANDS_H
#define COMMANDS_H

#include "localbin/core/core.h"

void cmd_install(const char *src_path, const char *version);
void cmd_install_with_options(const char *src_path, const char *version,
                              const char *as_name, const char *alias,
                              const char *pre_hook, const char *post_hook);
void cmd_remove(const char *name);
void cmd_update(const char *name, const char *src_path);
void cmd_list(const ListOptions *opts);
void cmd_search(const char *term);
void cmd_info(const char *name);
void cmd_verify(const char *name);
void cmd_verify_all(void);
void cmd_doctor(void);
void cmd_setup(void);
void cmd_self_update(int manual_mode);

void print_help(const char *prog_name);
void print_version(void);

#endif // COMMANDS_H
