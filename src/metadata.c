#include "metadata.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>

// Inicializar sistema de metadata
int metadata_init(void) {
    char metadata_dir[MAX_PATH];
    get_metadata_dir(metadata_dir, sizeof(metadata_dir));
    return ensure_dir(metadata_dir);
}

// Obtener ruta del archivo de metadata para un programa
static void get_metadata_path(const char *program_name, char *buf, size_t size) {
    char metadata_dir[MAX_PATH];
    get_metadata_dir(metadata_dir, sizeof(metadata_dir));
    snprintf(buf, size, "%s/%s.json", metadata_dir, program_name);
}

// Convertir metadata a JSON (serialización manual simple)
char* metadata_to_json(const ProgramMetadata *meta) {
    // Calcular tamaño necesario
    size_t size = 2048;  // Tamaño base
    char *json = malloc(size);
    if (!json) return NULL;
    
    int pos = 0;
    pos += snprintf(json + pos, size - pos, "{\n");
    pos += snprintf(json + pos, size - pos, "  \"name\": \"%s\",\n", meta->name);
    pos += snprintf(json + pos, size - pos, "  \"version\": \"%s\",\n", meta->version);
    pos += snprintf(json + pos, size - pos, "  \"source_path\": \"%s\",\n", meta->source_path);
    pos += snprintf(json + pos, size - pos, "  \"checksum_sha256\": \"%s\",\n", meta->checksum_sha256);
    pos += snprintf(json + pos, size - pos, "  \"install_date\": %ld,\n", meta->install_date);
    pos += snprintf(json + pos, size - pos, "  \"update_date\": %ld,\n", meta->update_date);
    pos += snprintf(json + pos, size - pos, "  \"size_bytes\": %lld,\n", meta->size_bytes);
    pos += snprintf(json + pos, size - pos, "  \"permissions\": %o,\n", meta->permissions);
    pos += snprintf(json + pos, size - pos, "  \"dependencies\": [");
    
    for (int i = 0; i < meta->dep_count; i++) {
        if (i > 0) pos += snprintf(json + pos, size - pos, ",");
        pos += snprintf(json + pos, size - pos, "\"%s\"", meta->dependencies[i]);
    }
    
    pos += snprintf(json + pos, size - pos, "],\n");
    pos += snprintf(json + pos, size - pos, "  \"dep_count\": %d\n", meta->dep_count);
    pos += snprintf(json + pos, size - pos, "}\n");
    
    return json;
}

// Parsear JSON simple (parser básico, no completo)
int metadata_from_json(const char *json, ProgramMetadata *meta) {
    memset(meta, 0, sizeof(ProgramMetadata));
    
    // Parser simple para los campos principales
    const char *p = json;
    
    // Buscar name
    if ((p = strstr(json, "\"name\":")) != NULL) {
        sscanf(p, "\"name\": \"%255[^\"]\"", meta->name);
    }
    
    // Buscar version
    if ((p = strstr(json, "\"version\":")) != NULL) {
        sscanf(p, "\"version\": \"%31[^\"]\"", meta->version);
    }
    
    // Buscar source_path
    if ((p = strstr(json, "\"source_path\":")) != NULL) {
        sscanf(p, "\"source_path\": \"%1023[^\"]\"", meta->source_path);
    }
    
    // Buscar checksum
    if ((p = strstr(json, "\"checksum_sha256\":")) != NULL) {
        sscanf(p, "\"checksum_sha256\": \"%64[^\"]\"", meta->checksum_sha256);
    }
    
    // Buscar fechas
    if ((p = strstr(json, "\"install_date\":")) != NULL) {
        long temp;
        sscanf(p, "\"install_date\": %ld", &temp);
        meta->install_date = temp;
    }
    
    if ((p = strstr(json, "\"update_date\":")) != NULL) {
        long temp;
        sscanf(p, "\"update_date\": %ld", &temp);
        meta->update_date = temp;
    }
    
    // Buscar size
    if ((p = strstr(json, "\"size_bytes\":")) != NULL) {
        sscanf(p, "\"size_bytes\": %lld", &meta->size_bytes);
    }
    
    // Buscar permisos
    if ((p = strstr(json, "\"permissions\":")) != NULL) {
        unsigned int temp;
        sscanf(p, "\"permissions\": %o", &temp);
        meta->permissions = temp;
    }
    
    // Buscar dep_count
    if ((p = strstr(json, "\"dep_count\":")) != NULL) {
        sscanf(p, "\"dep_count\": %d", &meta->dep_count);
    }
    
    // TODO: Parsear array de dependencias (más complejo)
    
    return 0;
}

// Guardar metadata de un programa
int metadata_save(const ProgramMetadata *meta) {
    if (metadata_init() != 0) {
        return -1;
    }
    
    char metadata_path[MAX_PATH];
    get_metadata_path(meta->name, metadata_path, sizeof(metadata_path));
    
    char *json = metadata_to_json(meta);
    if (!json) {
        return -1;
    }
    
    FILE *f = fopen(metadata_path, "w");
    if (!f) {
        free(json);
        return -1;
    }
    
    fprintf(f, "%s", json);
    fclose(f);
    free(json);
    
    return 0;
}

// Cargar metadata de un programa
int metadata_load(const char *program_name, ProgramMetadata *meta) {
    char metadata_path[MAX_PATH];
    get_metadata_path(program_name, metadata_path, sizeof(metadata_path));
    
    FILE *f = fopen(metadata_path, "r");
    if (!f) {
        return -1;
    }
    
    // Leer todo el archivo
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char *json = malloc(size + 1);
    if (!json) {
        fclose(f);
        return -1;
    }
    
    fread(json, 1, size, f);
    json[size] = '\0';
    fclose(f);
    
    int result = metadata_from_json(json, meta);
    free(json);
    
    return result;
}

// Eliminar metadata de un programa
int metadata_delete(const char *program_name) {
    char metadata_path[MAX_PATH];
    get_metadata_path(program_name, metadata_path, sizeof(metadata_path));
    
    if (unlink(metadata_path) != 0) {
        return -1;
    }
    
    return 0;
}

// Verificar si existe metadata para un programa
int metadata_exists(const char *program_name) {
    char metadata_path[MAX_PATH];
    get_metadata_path(program_name, metadata_path, sizeof(metadata_path));
    return file_exists(metadata_path);
}

// Listar todos los programas con metadata
int metadata_list_all(ProgramMetadata **list, int *count) {
    char metadata_dir[MAX_PATH];
    get_metadata_dir(metadata_dir, sizeof(metadata_dir));
    
    DIR *dir = opendir(metadata_dir);
    if (!dir) {
        *list = NULL;
        *count = 0;
        return 0;  // No es error, simplemente no hay metadata
    }
    
    // Contar archivos .json
    int n = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strstr(entry->d_name, ".json")) {
            n++;
        }
    }
    rewinddir(dir);
    
    // Asignar memoria
    ProgramMetadata *programs = malloc(n * sizeof(ProgramMetadata));
    if (!programs) {
        closedir(dir);
        return -1;
    }
    
    // Cargar metadata
    int i = 0;
    while ((entry = readdir(dir)) != NULL && i < n) {
        if (strstr(entry->d_name, ".json")) {
            // Extraer nombre del programa (sin .json)
            char name[256];
            strncpy(name, entry->d_name, sizeof(name) - 1);
            char *dot = strrchr(name, '.');
            if (dot) *dot = '\0';
            
            if (metadata_load(name, &programs[i]) == 0) {
                i++;
            }
        }
    }
    
    closedir(dir);
    
    *list = programs;
    *count = i;
    return 0;
}

// Liberar lista de metadata
void metadata_free_list(ProgramMetadata *list) {
    if (list) {
        free(list);
    }
}

// Actualizar un campo específico de metadata
int metadata_update_field(const char *program_name, const char *field, const char *value) {
    ProgramMetadata meta;
    
    if (metadata_load(program_name, &meta) != 0) {
        return -1;
    }
    
    // Actualizar el campo correspondiente
    if (strcmp(field, "version") == 0) {
        strncpy(meta.version, value, sizeof(meta.version) - 1);
    } else if (strcmp(field, "source_path") == 0) {
        strncpy(meta.source_path, value, sizeof(meta.source_path) - 1);
    } else if (strcmp(field, "checksum") == 0) {
        strncpy(meta.checksum_sha256, value, sizeof(meta.checksum_sha256) - 1);
    }
    // ... más campos según sea necesario
    
    return metadata_save(&meta);
}
