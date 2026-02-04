// Purpose: Robust CSV parsing and writing. Author: GitHub Copilot
#include "csv.h"
#include "util.h"

#include <sqlite3.h>
#include <stdlib.h>
#include <string.h>

static void csv_write_field(FILE* out, const char* s) {
    int need_quote = 0;
    for (const char* p = s; p && *p; ++p) {
        if (*p == ',' || *p == '"' || *p == '\n' || *p == '\r') {
            need_quote = 1;
            break;
        }
    }
    if (!need_quote) {
        fprintf(out, "%s", s ? s : "");
        return;
    }
    fputc('"', out);
    for (const char* p = s; p && *p; ++p) {
        if (*p == '"') {
            fputc('"', out);
            fputc('"', out);
        }
        else {
            fputc(*p, out);
        }
    }
    fputc('"', out);
}

int csv_write_contacts(Db* db, FILE* out) {
    if (!db || !db->handle || !out) {
        return 0;
    }
    fprintf(out, "Name,Phone,Address,Email,DueAmount,DueDate\n");

    const char* sql = "SELECT name, phone, address, email, due_amount, due_date FROM contacts ORDER BY name COLLATE NOCASE;";
    sqlite3_stmt* stmt = NULL;
    if (sqlite3_prepare_v2(db->handle, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return 0;
    }
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* name = (const char*)sqlite3_column_text(stmt, 0);
        const char* phone = (const char*)sqlite3_column_text(stmt, 1);
        const char* address = (const char*)sqlite3_column_text(stmt, 2);
        const char* email = (const char*)sqlite3_column_text(stmt, 3);
        double due_amount = sqlite3_column_double(stmt, 4);
        const char* due_date = (const char*)sqlite3_column_text(stmt, 5);

        char due_buf[64];
        snprintf(due_buf, sizeof(due_buf), "%.2f", due_amount);

        csv_write_field(out, name ? name : "");
        fputc(',', out);
        csv_write_field(out, phone ? phone : "");
        fputc(',', out);
        csv_write_field(out, address ? address : "");
        fputc(',', out);
        csv_write_field(out, email ? email : "");
        fputc(',', out);
        csv_write_field(out, due_buf);
        fputc(',', out);
        csv_write_field(out, due_date ? due_date : "");
        fputc('\n', out);
    }
    sqlite3_finalize(stmt);
    return 1;
}

static int csv_read_record(FILE* in, char** fields, size_t field_count) {
    if (!in || !fields || field_count == 0) {
        return 0;
    }
    size_t field = 0;
    int c;
    int in_quotes = 0;
    size_t cap = 256;
    size_t len = 0;
    char* buf = (char*)malloc(cap);
    if (!buf) {
        return -1;
    }
    memset(fields, 0, sizeof(char*) * field_count);

    while ((c = fgetc(in)) != EOF) {
        if (field >= field_count) {
            break;
        }
        if (!in_quotes && (c == ',' || c == '\n' || c == '\r')) {
            buf[len] = '\0';
            fields[field] = (char*)malloc(len + 1);
            if (!fields[field]) {
                free(buf);
                return -1;
            }
            memcpy(fields[field], buf, len + 1);
            field++;
            len = 0;
            if (c == '\n') {
                break;
            }
            if (c == '\r') {
                int next = fgetc(in);
                if (next != '\n' && next != EOF) {
                    ungetc(next, in);
                }
                break;
            }
            continue;
        }

        if (c == '"') {
            if (in_quotes) {
                int next = fgetc(in);
                if (next == '"') {
                    c = '"';
                }
                else {
                    in_quotes = 0;
                    if (next != EOF) {
                        ungetc(next, in);
                        continue;
                    }
                    continue;
                }
            }
            else if (len == 0) {
                in_quotes = 1;
                continue;
            }
        }

        if (len + 1 >= cap) {
            cap *= 2;
            char* tmp = (char*)realloc(buf, cap);
            if (!tmp) {
                free(buf);
                return -1;
            }
            buf = tmp;
        }
        buf[len++] = (char)c;
    }

    if (len > 0 || field > 0) {
        if (field < field_count) {
            buf[len] = '\0';
            fields[field] = (char*)malloc(len + 1);
            if (!fields[field]) {
                free(buf);
                return -1;
            }
            memcpy(fields[field], buf, len + 1);
            field++;
        }
    }
    else {
        free(buf);
        return 0;
    }

    free(buf);
    return (int)field;
}

static void csv_free_fields(char** fields, size_t field_count) {
    for (size_t i = 0; i < field_count; ++i) {
        free(fields[i]);
        fields[i] = NULL;
    }
}

int csv_import_contacts(Db* db, FILE* in, int strict, int dry_run, int* out_imported, int* out_failed) {
    if (!db || !db->handle || !in) {
        return 0;
    }
    int imported = 0;
    int failed = 0;

    char* fields[6];
    int header_read = 0;

    if (!dry_run) {
        if (!db_begin(db)) {
            return 0;
        }
    }

    while (1) {
        int count = csv_read_record(in, fields, 6);
        if (count == 0) {
            break;
        }
        if (count < 0) {
            failed++;
            csv_free_fields(fields, 6);
            if (strict) {
                if (!dry_run) {
                    db_rollback(db);
                }
                return 0;
            }
            continue;
        }
        if (!header_read) {
            header_read = 1;
            csv_free_fields(fields, 6);
            continue;
        }
        if (count < 6) {
            failed++;
            csv_free_fields(fields, 6);
            if (strict) {
                if (!dry_run) {
                    db_rollback(db);
                }
                return 0;
            }
            continue;
        }
        Contact c = { 0 };
        snprintf(c.name, sizeof(c.name), "%s", fields[0] ? fields[0] : "");
        snprintf(c.phone, sizeof(c.phone), "%s", fields[1] ? fields[1] : "");
        snprintf(c.address, sizeof(c.address), "%s", fields[2] ? fields[2] : "");
        snprintf(c.email, sizeof(c.email), "%s", fields[3] ? fields[3] : "");
        snprintf(c.due_date, sizeof(c.due_date), "%s", fields[5] ? fields[5] : "");
        if (!util_parse_double(fields[4] ? fields[4] : "0", &c.due_amount, -1e12, 1e12)) {
            c.due_amount = 0.0;
        }
        int64_t id = 0;
        int ok = 1;
        if (!dry_run) {
            ok = contacts_add(db, &c, &id);
        }
        if (ok) {
            imported++;
        }
        else {
            failed++;
            if (strict) {
                csv_free_fields(fields, 6);
                if (!dry_run) {
                    db_rollback(db);
                }
                return 0;
            }
        }
        csv_free_fields(fields, 6);
    }

    if (!dry_run) {
        if (!db_commit(db)) {
            db_rollback(db);
            return 0;
        }
    }

    if (out_imported) {
        *out_imported = imported;
    }
    if (out_failed) {
        *out_failed = failed;
    }
    return 1;
}
