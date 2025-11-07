#ifndef COMMANDS_H
#define COMMANDS_H

#include "core.h"

// Comandos principales
void cmd_install(const char *src_path, const char *version);
void cmd_remove(const char *name);
void cmd_list(const ListOptions *opts);
void cmd_info(const char *name);
void cmd_update(const char *name, const char *src_path);
void cmd_verify(const char *name);
void cmd_verify_all(void);
void cmd_doctor(void);
void cmd_setup(void);

// Comandos de versiones
void cmd_version_list(const char *name);
void cmd_version_switch(const char *name, const char *version);
void cmd_version_install(const char *src_path, const char *version);

// Comandos de búsqueda y filtros
void cmd_search(const char *term);

// Comandos de backup
void cmd_backup(const char *output_path);
void cmd_restore(const char *backup_path);

// Comandos de export/import
void cmd_export(const char *output_file);
void cmd_import(const char *input_file);

// Comandos de dependencias
void cmd_deps_add(const char *program, const char *dependency);
void cmd_deps_remove(const char *program, const char *dependency);
void cmd_deps_list(const char *program);

// Comandos avanzados
void cmd_rollback(void);
void cmd_self_update(void);
void cmd_completion(const char *shell);

// Ayuda
void print_help(const char *prog_name);
void print_version(void);

#endif // COMMANDS_H
