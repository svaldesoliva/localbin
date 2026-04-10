#ifndef METADATA_H
#define METADATA_H

#include "localbin/core/core.h"

// Funciones de gestión de metadata
int metadata_init(void);
int metadata_save(const ProgramMetadata *meta);
int metadata_load(const char *program_name, ProgramMetadata *meta);
int metadata_delete(const char *program_name);
int metadata_exists(const char *program_name);
int metadata_list_all(ProgramMetadata **list, int *count);
void metadata_free_list(ProgramMetadata *list);
int metadata_update_field(const char *program_name, const char *field, const char *value);

// Serialización JSON
char* metadata_to_json(const ProgramMetadata *meta);
int metadata_from_json(const char *json, ProgramMetadata *meta);

#endif // METADATA_H
