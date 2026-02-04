// Purpose: Contact data model and business logic. Author: GitHub Copilot
#include "contacts.h"
#include "util.h"

#include <ctype.h>
#include <sqlite3.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static const char* default_sort_mode = "name";

static const char* sort_clause_for_mode(const char* mode) {
    if (!mode) {
        return "ORDER BY name COLLATE NOCASE";
    }
    if (strcmp(mode, "name") == 0) {
        return "ORDER BY name COLLATE NOCASE";
    }
    if (strcmp(mode, "phone") == 0) {
        return "ORDER BY phone COLLATE NOCASE";
    }
    if (strcmp(mode, "due_date") == 0) {
        return "ORDER BY due_date COLLATE NOCASE";
    }
    return "ORDER BY name COLLATE NOCASE";
}

int contacts_add(Db* db, const Contact* c, int64_t* out_id) {
    if (!db || !db->handle || !c || !c->name[0]) {
        return 0;
    }
    const char* sql =
        "INSERT INTO contacts(name, phone, address, email, due_amount, due_date)"
        " VALUES(?,?,?,?,?,?);";
    sqlite3_stmt* stmt = NULL;
    if (sqlite3_prepare_v2(db->handle, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return 0;
    }
    sqlite3_bind_text(stmt, 1, c->name, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, c->phone, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, c->address, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, c->email, -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 5, c->due_amount);
    sqlite3_bind_text(stmt, 6, c->due_date, -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) {
        return 0;
    }
    if (out_id) {
        *out_id = (int64_t)sqlite3_last_insert_rowid(db->handle);
    }
    return 1;
}

int contacts_update(Db* db, const Contact* c) {
    if (!db || !db->handle || !c || c->id <= 0) {
        return 0;
    }
    const char* sql =
        "UPDATE contacts SET name=?, phone=?, address=?, email=?, due_amount=?, due_date=? WHERE id=?;";
    sqlite3_stmt* stmt = NULL;
    if (sqlite3_prepare_v2(db->handle, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return 0;
    }
    sqlite3_bind_text(stmt, 1, c->name, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, c->phone, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, c->address, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, c->email, -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 5, c->due_amount);
    sqlite3_bind_text(stmt, 6, c->due_date, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 7, c->id);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

int contacts_delete(Db* db, int64_t id) {
    if (!db || !db->handle || id <= 0) {
        return 0;
    }
    const char* sql = "DELETE FROM contacts WHERE id=?;";
    sqlite3_stmt* stmt = NULL;
    if (sqlite3_prepare_v2(db->handle, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return 0;
    }
    sqlite3_bind_int64(stmt, 1, id);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

int contacts_get_by_id(Db* db, int64_t id, Contact* out) {
    if (!db || !db->handle || !out || id <= 0) {
        return 0;
    }
    const char* sql =
        "SELECT id, name, phone, address, email, due_amount, due_date FROM contacts WHERE id=?;";
    sqlite3_stmt* stmt = NULL;
    if (sqlite3_prepare_v2(db->handle, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return 0;
    }
    sqlite3_bind_int64(stmt, 1, id);
    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        out->id = sqlite3_column_int64(stmt, 0);
        snprintf(out->name, sizeof(out->name), "%s", (const char*)sqlite3_column_text(stmt, 1));
        snprintf(out->phone, sizeof(out->phone), "%s", (const char*)sqlite3_column_text(stmt, 2));
        snprintf(out->address, sizeof(out->address), "%s", (const char*)sqlite3_column_text(stmt, 3));
        snprintf(out->email, sizeof(out->email), "%s", (const char*)sqlite3_column_text(stmt, 4));
        out->due_amount = sqlite3_column_double(stmt, 5);
        snprintf(out->due_date, sizeof(out->due_date), "%s", (const char*)sqlite3_column_text(stmt, 6));
        sqlite3_finalize(stmt);
        return 1;
    }
    sqlite3_finalize(stmt);
    return 0;
}

static void print_due_notice(FILE* out, const char* due_date) {
    if (!out || !due_date || !due_date[0]) {
        return;
    }
    int days = 0;
    if (!util_due_days(due_date, &days)) {
        fprintf(out, "\n\tDue status : invalid date (expected YYYY-MM-DD)\n");
        return;
    }
    if (days < 0) {
        fprintf(out, "\n\tDue status : Expired %d day%s ago\n", -days, (-days) == 1 ? "" : "s");
    }
    else if (days == 0) {
        fprintf(out, "\n\tDue status : Due today\n");
    }
    else {
        fprintf(out, "\n\tDue status : Due in %d day%s\n", days, days == 1 ? "" : "s");
    }
}

static void print_contact_plain(FILE* out, const Contact* c) {
    fprintf(out, "\tID\t: %lld\n", (long long)c->id);
    fprintf(out, "\t\t\tName      : %s\n", c->name);
    fprintf(out, "\t\t\tPhone     : %s\n", c->phone);
    fprintf(out, "\t\t\tAddress   : %s\n", c->address);
    fprintf(out, "\t\t\tEmail     : %s\n", c->email);
    fprintf(out, "\t\t\tDue Amt   : %.2f\n", c->due_amount);
    fprintf(out, "\t\t\tDue Date  : %s\n", c->due_date);
    print_due_notice(out, c->due_date);
}

static void print_contact_json(FILE* out, const Contact* c, int trailing_comma) {
    fprintf(out, "{");
    fprintf(out, "\"id\":%lld,", (long long)c->id);
    fprintf(out, "\"name\":");
    util_print_json_string(out, c->name);
    fprintf(out, ",\"phone\":");
    util_print_json_string(out, c->phone);
    fprintf(out, ",\"address\":");
    util_print_json_string(out, c->address);
    fprintf(out, ",\"email\":");
    util_print_json_string(out, c->email);
    fprintf(out, ",\"due_amount\":%.2f,", c->due_amount);
    fprintf(out, "\"due_date\":");
    util_print_json_string(out, c->due_date);
    fprintf(out, "}%s", trailing_comma ? "," : "");
}

static void print_today(FILE* out) {
    if (!out) {
        return;
    }
    char today[16] = { 0 };
    util_format_iso_date(time(NULL), today, sizeof(today));
    fprintf(out, "\nToday is %s\n\n", today);
}

static int list_query(Db* db, const char* where_clause, const char* param, int json, FILE* out, int show_today) {
    char sort_mode[32] = { 0 };
    if (!contacts_get_sort_mode(db, sort_mode, sizeof(sort_mode))) {
        snprintf(sort_mode, sizeof(sort_mode), "%s", default_sort_mode);
    }
    char sql[512];
    snprintf(sql, sizeof(sql),
        "SELECT id, name, phone, address, email, due_amount, due_date FROM contacts %s %s;",
        where_clause ? where_clause : "",
        sort_clause_for_mode(sort_mode));
    sqlite3_stmt* stmt = NULL;
    if (sqlite3_prepare_v2(db->handle, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return 0;
    }
    if (param) {
        sqlite3_bind_text(stmt, 1, param, -1, SQLITE_TRANSIENT);
    }

    int first = 1;
    if (!json && show_today) {
        print_today(out);
    }
    if (json) {
        fprintf(out, "[");
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Contact c = { 0 };
        c.id = sqlite3_column_int64(stmt, 0);
        snprintf(c.name, sizeof(c.name), "%s", (const char*)sqlite3_column_text(stmt, 1));
        snprintf(c.phone, sizeof(c.phone), "%s", (const char*)sqlite3_column_text(stmt, 2));
        snprintf(c.address, sizeof(c.address), "%s", (const char*)sqlite3_column_text(stmt, 3));
        snprintf(c.email, sizeof(c.email), "%s", (const char*)sqlite3_column_text(stmt, 4));
        c.due_amount = sqlite3_column_double(stmt, 5);
        snprintf(c.due_date, sizeof(c.due_date), "%s", (const char*)sqlite3_column_text(stmt, 6));

        if (json) {
            if (!first) {
                fprintf(out, ",");
            }
            print_contact_json(out, &c, 0);
        }
        else {
            print_contact_plain(out, &c);
            fprintf(out, "\n");
        }
        first = 0;
    }

    if (json) {
        fprintf(out, "]\n");
    }

    sqlite3_finalize(stmt);
    return 1;
}

int contacts_list(Db* db, int json, FILE* out) {
    if (!db || !db->handle || !out) {
        return 0;
    }
    return list_query(db, NULL, NULL, json, out, 1);
}

int contacts_search_by_name(Db* db, const char* name, int json, FILE* out) {
    if (!db || !db->handle || !name || !out) {
        return 0;
    }
    return list_query(db, "WHERE name LIKE ? COLLATE NOCASE", name, json, out, 0);
}

int contacts_stats(Db* db, ContactStats* out) {
    if (!db || !db->handle || !out) {
        return 0;
    }
    memset(out, 0, sizeof(*out));
    const char* sql = "SELECT name, phone, address, email, due_amount, due_date FROM contacts;";
    sqlite3_stmt* stmt = NULL;
    if (sqlite3_prepare_v2(db->handle, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return 0;
    }
    int has_due_amount = 0;
    int has_valid_due_date = 0;
    time_t earliest_time = 0;
    time_t latest_time = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char* name = sqlite3_column_text(stmt, 0);
        const unsigned char* phone = sqlite3_column_text(stmt, 1);
        const unsigned char* address = sqlite3_column_text(stmt, 2);
        const unsigned char* email = sqlite3_column_text(stmt, 3);
        double due_amount = sqlite3_column_double(stmt, 4);
        const unsigned char* due_date = sqlite3_column_text(stmt, 5);
        out->total_contacts++;
        if (!phone || !phone[0]) {
            out->missing_phone++;
        }
        if (!address || !address[0]) {
            out->missing_address++;
        }
        if (!email || !email[0]) {
            out->missing_email++;
        }
        if (due_amount > 0.0) {
            out->due_contacts++;
            out->total_due_amount += due_amount;
            if (!has_due_amount || due_amount < out->min_due_amount) {
                out->min_due_amount = due_amount;
                util_copy_str(out->min_due_name, sizeof(out->min_due_name),
                    name ? (const char*)name : "");
            }
            if (!has_due_amount || due_amount > out->max_due_amount) {
                out->max_due_amount = due_amount;
                util_copy_str(out->max_due_name, sizeof(out->max_due_name),
                    name ? (const char*)name : "");
            }
            has_due_amount = 1;
        }
        else {
            out->no_due_contacts++;
        }
        if (due_date && due_date[0]) {
            out->due_date_present++;
            int days = 0;
            if (util_due_days((const char*)due_date, &days)) {
                if (days < 0) {
                    out->overdue_contacts++;
                }
                else if (days == 0) {
                    out->due_today_contacts++;
                    out->due_soon_contacts++;
                }
                else if (days <= 7) {
                    out->due_soon_contacts++;
                }
                else {
                    out->due_later_contacts++;
                }

                struct tm due_tm = { 0 };
                if (util_parse_iso_date((const char*)due_date, &due_tm)) {
                    time_t due_time = mktime(&due_tm);
                    if (due_time != (time_t)-1) {
                        if (!has_valid_due_date || due_time < earliest_time) {
                            earliest_time = due_time;
                            util_copy_str(out->earliest_due_date, sizeof(out->earliest_due_date),
                                (const char*)due_date);
                        }
                        if (!has_valid_due_date || due_time > latest_time) {
                            latest_time = due_time;
                            util_copy_str(out->latest_due_date, sizeof(out->latest_due_date),
                                (const char*)due_date);
                        }
                        has_valid_due_date = 1;
                    }
                }
            }
            else {
                out->due_date_invalid++;
            }
        }
        else {
            out->due_date_missing++;
        }
        if (name && name[0]) {
            unsigned char c = (unsigned char)name[0];
            if (isalpha(c)) {
                int idx = toupper(c) - 'A';
                if (idx >= 0 && idx < 26) {
                    out->by_letter[idx]++;
                }
            }
            else {
                out->by_letter[26]++;
            }
        }
    }
    sqlite3_finalize(stmt);
    if (out->due_contacts > 0) {
        out->avg_due_amount = out->total_due_amount / out->due_contacts;
    }
    return 1;
}

int contacts_set_sort_mode(Db* db, const char* mode) {
    if (!db || !db->handle || !mode) {
        return 0;
    }
    if (strcmp(mode, "name") != 0 && strcmp(mode, "phone") != 0 && strcmp(mode, "due_date") != 0) {
        return 0;
    }
    return db_set_setting(db, "sort_mode", mode);
}

int contacts_get_sort_mode(Db* db, char* mode, size_t mode_len) {
    if (!db || !db->handle || !mode || mode_len == 0) {
        return 0;
    }
    if (db_get_setting(db, "sort_mode", mode, mode_len)) {
        return 1;
    }
    snprintf(mode, mode_len, "%s", default_sort_mode);
    return 1;
}
