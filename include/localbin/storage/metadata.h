#ifndef METADATA_H
#define METADATA_H

#include "localbin/core/core.h"

int   metadata_save(const ProgramMetadata *meta);
int   metadata_load(const char *program_name, ProgramMetadata *meta);
int   metadata_delete(const char *program_name);
int   metadata_list_all(ProgramMetadata **list, int *count);
void  metadata_free_list(ProgramMetadata *list);

#endif // METADATA_H
