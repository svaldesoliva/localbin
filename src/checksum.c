#include "checksum.h"
#include "core.h"
#include "metadata.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <CommonCrypto/CommonDigest.h>

// Calcular SHA256 de un archivo
int checksum_calculate_sha256(const char *filepath, char *output, size_t output_size) {
    if (output_size < 65) {  // SHA256 = 64 chars hex + \0
        return -1;
    }
    
    FILE *f = fopen(filepath, "rb");
    if (!f) {
        return -1;
    }
    
    CC_SHA256_CTX ctx;
    CC_SHA256_Init(&ctx);
    
    unsigned char buf[8192];
    size_t bytes_read;
    
    while ((bytes_read = fread(buf, 1, sizeof(buf), f)) > 0) {
        CC_SHA256_Update(&ctx, buf, (CC_LONG)bytes_read);
    }
    
    unsigned char hash[CC_SHA256_DIGEST_LENGTH];
    CC_SHA256_Final(hash, &ctx);
    
    fclose(f);
    
    // Convertir a hexadecimal
    for (int i = 0; i < CC_SHA256_DIGEST_LENGTH; i++) {
        sprintf(output + (i * 2), "%02x", hash[i]);
    }
    output[64] = '\0';
    
    return 0;
}

// Verificar si el checksum de un archivo coincide
int checksum_verify_file(const char *filepath, const char *expected_sha256) {
    char actual_sha256[65];
    
    if (checksum_calculate_sha256(filepath, actual_sha256, sizeof(actual_sha256)) != 0) {
        return -1;  // Error al calcular checksum
    }
    
    return strcmp(actual_sha256, expected_sha256) == 0 ? 0 : 1;  // 0 = match, 1 = mismatch
}

// Verificar todos los programas instalados
int checksum_verify_all(void) {
    ProgramMetadata *programs = NULL;
    int count = 0;
    
    if (metadata_list_all(&programs, &count) != 0) {
        return -1;
    }
    
    int errors = 0;
    char install_dir[MAX_PATH];
    get_install_dir(install_dir, sizeof(install_dir));
    
    printf("🔍 Verificando integridad de %d programa(s)...\n\n", count);
    
    for (int i = 0; i < count; i++) {
        char program_path[MAX_PATH];
        snprintf(program_path, sizeof(program_path), "%s/%s", install_dir, programs[i].name);
        
        int result = checksum_verify_file(program_path, programs[i].checksum_sha256);
        
        if (result == 0) {
            printf("✅ %s - OK\n", programs[i].name);
        } else if (result == 1) {
            printf("❌ %s - MODIFICADO (checksum no coincide)\n", programs[i].name);
            errors++;
        } else {
            printf("⚠️  %s - ERROR (no se pudo verificar)\n", programs[i].name);
            errors++;
        }
    }
    
    metadata_free_list(programs);
    
    printf("\n");
    if (errors == 0) {
        printf("✅ Todos los programas verificados correctamente\n");
        return 0;
    } else {
        printf("❌ Se encontraron %d error(es)\n", errors);
        return 1;
    }
}

// Comparar checksums de dos archivos
int checksum_compare_files(const char *file1, const char *file2) {
    char hash1[65], hash2[65];
    
    if (checksum_calculate_sha256(file1, hash1, sizeof(hash1)) != 0) {
        return -1;
    }
    
    if (checksum_calculate_sha256(file2, hash2, sizeof(hash2)) != 0) {
        return -1;
    }
    
    return strcmp(hash1, hash2) == 0 ? 0 : 1;
}
