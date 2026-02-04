// Purpose: Password hashing and verification. Author: GitHub Copilot
#include "auth.h"
#include "util.h"

#include <stdio.h>
#include <string.h>

#if defined(HAVE_LIBSODIUM)
#include <sodium.h>
#elif defined(HAVE_ARGON2)
#include <argon2.h>
#endif

#define HASH_MAX 256

int auth_set_password(Db* db, const char* password) {
    if (!db || !db->handle || !password || !password[0]) {
        return 0;
    }
    char hash[HASH_MAX];
    memset(hash, 0, sizeof(hash));

#if defined(HAVE_LIBSODIUM)
    if (sodium_init() < 0) {
        return 0;
    }
    if (crypto_pwhash_str(hash, password, strlen(password),
        crypto_pwhash_OPSLIMIT_MODERATE,
        crypto_pwhash_MEMLIMIT_MODERATE) != 0) {
        return 0;
    }
#elif defined(HAVE_ARGON2)
    uint8_t salt[16];
    if (!util_random_bytes(salt, sizeof(salt))) {
        return 0;
    }
    uint32_t t_cost = 3;
    uint32_t m_cost = 1 << 16;
    uint32_t parallelism = 1;
    size_t hash_len = 32;
    size_t encoded_len = argon2_encodedlen(t_cost, m_cost, parallelism, (uint32_t)sizeof(salt), (uint32_t)hash_len, Argon2_id);
    if (encoded_len > sizeof(hash)) {
        return 0;
    }
    if (argon2id_hash_encoded(t_cost, m_cost, parallelism, password, strlen(password),
        salt, sizeof(salt), hash_len, hash, encoded_len) != ARGON2_OK) {
        return 0;
    }
#else
    (void)password;
    return 0;
#endif

    return db_set_password_hash(db, hash);
}

int auth_verify_password(Db* db, const char* password) {
    if (!db || !db->handle || !password || !password[0]) {
        return 0;
    }
    char hash[HASH_MAX] = { 0 };
    if (!db_get_password_hash(db, hash, sizeof(hash))) {
        return 0;
    }

#if defined(HAVE_LIBSODIUM)
    if (sodium_init() < 0) {
        return 0;
    }
    return crypto_pwhash_str_verify(hash, password, strlen(password)) == 0;
#elif defined(HAVE_ARGON2)
    return argon2id_verify(hash, password, strlen(password)) == ARGON2_OK;
#else
    (void)password;
    return 0;
#endif
}
