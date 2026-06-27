/*
 * Portable SHA-256 — public domain, no external dependencies.
 * Works on Linux, macOS, and any C11 compiler.
 */
#ifndef SHA256_H
#define SHA256_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint32_t state[8];
    uint64_t count;
    uint8_t  buf[64];
} SHA256_CTX;

void sha256_init(SHA256_CTX *ctx);
void sha256_update(SHA256_CTX *ctx, const void *data, size_t len);
void sha256_final(SHA256_CTX *ctx, uint8_t digest[32]);

#endif // SHA256_H
