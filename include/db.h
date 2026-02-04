// Purpose: SQLite database wrapper and schema management. Author: GitHub Copilot
#ifndef CONTACTS_DB_H
#define CONTACTS_DB_H

#include <sqlite3.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct {
        const char* path;
        sqlite3* handle;
    } Db;

    int db_open(Db* db, const char* path);
    void db_close(Db* db);
    int db_init(Db* db);
    int db_begin(Db* db);
    int db_commit(Db* db);
    int db_rollback(Db* db);
    int db_set_setting(Db* db, const char* key, const char* value);
    int db_get_setting(Db* db, const char* key, char* value, size_t value_len);

    int db_set_password_hash(Db* db, const char* hash);
    int db_get_password_hash(Db* db, char* hash, size_t hash_len);

#ifdef __cplusplus
}
#endif

#endif
