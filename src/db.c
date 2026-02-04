// Purpose: SQLite database wrapper and schema management. Author: GitHub Copilot
#include "db.h"

#include <stdio.h>
#include <string.h>

static int db_exec(sqlite3* db, const char* sql) {
    char* errmsg = NULL;
    int rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
    if (rc != SQLITE_OK) {
        if (errmsg) {
            fprintf(stderr, "SQLite error: %s\n", errmsg);
            sqlite3_free(errmsg);
        }
        return 0;
    }
    return 1;
}

int db_open(Db* db, const char* path) {
    if (!db || !path) {
        return 0;
    }
    db->path = path;
    db->handle = NULL;
    if (sqlite3_open(path, &db->handle) != SQLITE_OK) {
        fprintf(stderr, "Failed to open database: %s\n", sqlite3_errmsg(db->handle));
        sqlite3_close(db->handle);
        db->handle = NULL;
        return 0;
    }
    if (!db_exec(db->handle, "PRAGMA foreign_keys = ON;")) {
        return 0;
    }
    return 1;
}

void db_close(Db* db) {
    if (db && db->handle) {
        sqlite3_close(db->handle);
        db->handle = NULL;
    }
}

int db_init(Db* db) {
    if (!db || !db->handle) {
        return 0;
    }
    const char* schema =
        "BEGIN;"
        "CREATE TABLE IF NOT EXISTS contacts ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name TEXT NOT NULL,"
        "phone TEXT,"
        "address TEXT,"
        "email TEXT,"
        "due_amount REAL DEFAULT 0,"
        "due_date TEXT"
        ");"
        "CREATE TABLE IF NOT EXISTS settings ("
        "key TEXT PRIMARY KEY,"
        "value TEXT"
        ");"
        "CREATE TABLE IF NOT EXISTS auth ("
        "id INTEGER PRIMARY KEY CHECK (id = 1),"
        "hash TEXT NOT NULL"
        ");"
        "COMMIT;";

    return db_exec(db->handle, schema);
}

int db_begin(Db* db) {
    if (!db || !db->handle) {
        return 0;
    }
    return db_exec(db->handle, "BEGIN;");
}

int db_commit(Db* db) {
    if (!db || !db->handle) {
        return 0;
    }
    return db_exec(db->handle, "COMMIT;");
}

int db_rollback(Db* db) {
    if (!db || !db->handle) {
        return 0;
    }
    return db_exec(db->handle, "ROLLBACK;");
}

int db_set_setting(Db* db, const char* key, const char* value) {
    if (!db || !db->handle || !key || !value) {
        return 0;
    }
    const char* sql = "INSERT INTO settings(key, value) VALUES(?, ?) "
        "ON CONFLICT(key) DO UPDATE SET value=excluded.value;";
    sqlite3_stmt* stmt = NULL;
    if (sqlite3_prepare_v2(db->handle, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return 0;
    }
    sqlite3_bind_text(stmt, 1, key, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, value, -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

int db_get_setting(Db* db, const char* key, char* value, size_t value_len) {
    if (!db || !db->handle || !key || !value || value_len == 0) {
        return 0;
    }
    const char* sql = "SELECT value FROM settings WHERE key = ?;";
    sqlite3_stmt* stmt = NULL;
    if (sqlite3_prepare_v2(db->handle, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return 0;
    }
    sqlite3_bind_text(stmt, 1, key, -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        const unsigned char* v = sqlite3_column_text(stmt, 0);
        if (v) {
            snprintf(value, value_len, "%s", (const char*)v);
            sqlite3_finalize(stmt);
            return 1;
        }
    }
    sqlite3_finalize(stmt);
    return 0;
}

int db_set_password_hash(Db* db, const char* hash) {
    if (!db || !db->handle || !hash) {
        return 0;
    }
    const char* sql = "INSERT INTO auth(id, hash) VALUES(1, ?) "
        "ON CONFLICT(id) DO UPDATE SET hash=excluded.hash;";
    sqlite3_stmt* stmt = NULL;
    if (sqlite3_prepare_v2(db->handle, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return 0;
    }
    sqlite3_bind_text(stmt, 1, hash, -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

int db_get_password_hash(Db* db, char* hash, size_t hash_len) {
    if (!db || !db->handle || !hash || hash_len == 0) {
        return 0;
    }
    const char* sql = "SELECT hash FROM auth WHERE id = 1;";
    sqlite3_stmt* stmt = NULL;
    if (sqlite3_prepare_v2(db->handle, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return 0;
    }
    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        const unsigned char* v = sqlite3_column_text(stmt, 0);
        if (v) {
            snprintf(hash, hash_len, "%s", (const char*)v);
            sqlite3_finalize(stmt);
            return 1;
        }
    }
    sqlite3_finalize(stmt);
    return 0;
}
