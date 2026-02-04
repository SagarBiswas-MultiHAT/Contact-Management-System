// Purpose: CLI entry point, argument parsing, and interactive menu. Author: GitHub Copilot
#include "auth.h"
#include "contacts.h"
#include "csv.h"
#include "db.h"
#include "util.h"

#include <limits.h>
#include <sqlite3.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define DEFAULT_DB_PATH "contacts.db"

typedef struct {
    const char* db_path;
    int json;
    int dry_run;
    int backup;
    int strict;
    int force;
    int menu;

    int do_list;
    int do_stats;
    int do_add;
    int do_edit;
    int do_delete;
    int do_delete_all;
    int do_search;
    int do_export;
    int do_import;
    int do_sort;
    int do_set_password;

    const char* name;
    const char* phone;
    const char* address;
    const char* email;
    const char* due;
    const char* due_date;
    const char* id;
    const char* search;
    const char* export_path;
    const char* import_path;
    const char* sort_mode;
    const char* password;
    const char* current_password;
} Options;

static void print_usage(FILE* out) {
    fprintf(out,
        "Contact Manager CLI\n"
        "Usage:\n"
        "  contacts [--db path] [--menu]\n"
        "  contacts --list [--json]\n"
        "  contacts --search \"name\" [--json]\n"
        "  contacts --add --name N [--phone P] [--address A] [--email E] [--due X] [--due-date D]\n"
        "  contacts --edit --id ID [--name N] [--phone P] [--address A] [--email E] [--due X] [--due-date D]\n"
        "  contacts --delete --id ID\n"
        "  contacts --delete-all --force\n"
        "  contacts --export file.csv\n"
        "  contacts --import file.csv [--dry-run] [--strict]\n"
        "  contacts --sort name|phone|due_date\n"
        "  contacts --stats [--json]\n"
        "  contacts --set-password [--password P] [--current-password P]\n"
        "Options:\n"
        "  --db PATH           Database path (default contacts.db)\n"
        "  --json              JSON output for list/search/stats\n"
        "  --dry-run           Preview import/migration without writing\n"
        "  --backup            Create DB backup before destructive ops\n"
        "  --strict            Abort on first CSV error\n"
        "  --force             Required for delete-all\n"
        "  --menu              Interactive menu mode\n");
}

static void print_json_string_or_null(FILE* out, const char* value) {
    if (!out) {
        return;
    }
    if (value && value[0]) {
        util_print_json_string(out, value);
    }
    else {
        fprintf(out, "null");
    }
}

static void print_stats_plain(FILE* out, const ContactStats* stats) {
    if (!out || !stats) {
        return;
    }
    int due_date_valid = stats->due_date_present - stats->due_date_invalid;
    if (due_date_valid < 0) {
        due_date_valid = 0;
    }
    fprintf(out, "\nTotal contacts: %d\n", stats->total_contacts);
    fprintf(out, "Contacts with due amounts: %d\n", stats->due_contacts);
    fprintf(out, "Contacts without due amounts: %d\n", stats->no_due_contacts);
    fprintf(out, "Total due amount: %.2f\n", stats->total_due_amount);
    if (stats->due_contacts > 0) {
        fprintf(out, "\nAverage due amount (non-zero): %.2f\n", stats->avg_due_amount);
        fprintf(out, "Largest due amount: %.2f", stats->max_due_amount);
        if (stats->max_due_name[0]) {
            fprintf(out, " (%s)", stats->max_due_name);
        }
        fprintf(out, "\n");
        fprintf(out, "Smallest due amount: %.2f", stats->min_due_amount);
        if (stats->min_due_name[0]) {
            fprintf(out, " (%s)", stats->min_due_name);
        }
        fprintf(out, "\n");
    }
    else {
        fprintf(out, "\nAverage due amount (non-zero): N/A\n");
        fprintf(out, "Largest due amount: N/A\n");
        fprintf(out, "Smallest due amount: N/A\n");
    }

    fprintf(out, "\nDue date coverage:\n");
    fprintf(out, "  With due date: %d\n", stats->due_date_present);
    fprintf(out, "  Valid due date: %d\n", due_date_valid);
    fprintf(out, "  Missing due date: %d\n", stats->due_date_missing);
    fprintf(out, "  Invalid due date: %d\n", stats->due_date_invalid);

    fprintf(out, "\nDue date status (valid dates):\n");
    fprintf(out, "  Overdue: %d\n", stats->overdue_contacts);
    fprintf(out, "  Due today: %d\n", stats->due_today_contacts);
    fprintf(out, "  Due soon (<=7 days, incl. today): %d\n", stats->due_soon_contacts);
    fprintf(out, "  Due later (>7 days): %d\n", stats->due_later_contacts);

    fprintf(out, "\nDue date range (valid dates):\n");
    fprintf(out, "  Earliest: %s\n", stats->earliest_due_date[0] ? stats->earliest_due_date : "N/A");
    fprintf(out, "  Latest: %s\n", stats->latest_due_date[0] ? stats->latest_due_date : "N/A");

    fprintf(out, "\nData completeness:\n");
    fprintf(out, "  Missing phone: %d\n", stats->missing_phone);
    fprintf(out, "  Missing email: %d\n", stats->missing_email);
    fprintf(out, "  Missing address: %d\n", stats->missing_address);

    fprintf(out, "\nContacts by letter distribution:\n");
    for (int i = 0; i < 26; ++i) {
        if (stats->by_letter[i] > 0) {
            fprintf(out, "  %c: %d\n", 'A' + i, stats->by_letter[i]);
        }
    }
    if (stats->by_letter[26] > 0) {
        fprintf(out, "  #: %d\n", stats->by_letter[26]);
    }
}

static void print_stats_json(FILE* out, const ContactStats* stats) {
    if (!out || !stats) {
        return;
    }
    int due_date_valid = stats->due_date_present - stats->due_date_invalid;
    if (due_date_valid < 0) {
        due_date_valid = 0;
    }
    fprintf(out, "{");
    fprintf(out, "\"total\":%d,", stats->total_contacts);
    fprintf(out, "\"due\":%d,", stats->due_contacts);
    fprintf(out, "\"no_due\":%d,", stats->no_due_contacts);
    fprintf(out, "\"overdue\":%d,", stats->overdue_contacts);
    fprintf(out, "\"due_today\":%d,", stats->due_today_contacts);
    fprintf(out, "\"due_soon\":%d,", stats->due_soon_contacts);
    fprintf(out, "\"due_later\":%d,", stats->due_later_contacts);
    fprintf(out, "\"due_date_present\":%d,", stats->due_date_present);
    fprintf(out, "\"due_date_valid\":%d,", due_date_valid);
    fprintf(out, "\"due_date_missing\":%d,", stats->due_date_missing);
    fprintf(out, "\"due_date_invalid\":%d,", stats->due_date_invalid);
    fprintf(out, "\"missing_phone\":%d,", stats->missing_phone);
    fprintf(out, "\"missing_email\":%d,", stats->missing_email);
    fprintf(out, "\"missing_address\":%d,", stats->missing_address);
    fprintf(out, "\"total_due_amount\":%.2f,", stats->total_due_amount);
    if (stats->due_contacts > 0) {
        fprintf(out, "\"avg_due_amount\":%.2f,", stats->avg_due_amount);
        fprintf(out, "\"min_due_amount\":%.2f,", stats->min_due_amount);
        fprintf(out, "\"max_due_amount\":%.2f,", stats->max_due_amount);
    }
    else {
        fprintf(out, "\"avg_due_amount\":null,");
        fprintf(out, "\"min_due_amount\":null,");
        fprintf(out, "\"max_due_amount\":null,");
    }
    fprintf(out, "\"min_due_name\":");
    print_json_string_or_null(out, stats->min_due_name);
    fprintf(out, ",\"max_due_name\":");
    print_json_string_or_null(out, stats->max_due_name);
    fprintf(out, ",\"earliest_due_date\":");
    print_json_string_or_null(out, stats->earliest_due_date);
    fprintf(out, ",\"latest_due_date\":");
    print_json_string_or_null(out, stats->latest_due_date);
    fprintf(out, ",\"by_letter\":[");
    for (int i = 0; i < 27; ++i) {
        if (i > 0) {
            fprintf(out, ",");
        }
        fprintf(out, "%d", stats->by_letter[i]);
    }
    fprintf(out, "]}\n");
}

static int prompt_line(const char* label, char* buf, size_t len) {
    printf("%s", label);
    fflush(stdout);
    if (!util_read_line(stdin, buf, len)) {
        return 0;
    }
    util_trim(buf);
    return 1;
}

static int ensure_due_date_format(const char* value) {
    if (!value || !value[0]) {
        return 1;
    }
    struct tm tmp = { 0 };
    return util_parse_iso_date(value, &tmp);
}

static int ensure_auth(Db* db, int interactive, const Options* opt) {
    char hash[256] = { 0 };
    int has_hash = db_get_password_hash(db, hash, sizeof(hash));
    if (!has_hash) {
        if (opt->do_set_password || interactive) {
            char pw1[128] = { 0 };
            char pw2[128] = { 0 };
            if (opt->password) {
                snprintf(pw1, sizeof(pw1), "%s", opt->password);
                snprintf(pw2, sizeof(pw2), "%s", opt->password);
            }
            else {
                if (!prompt_line("\nSet new password: ", pw1, sizeof(pw1))) {
                    return 0;
                }
                if (!prompt_line("Confirm password: ", pw2, sizeof(pw2))) {
                    return 0;
                }
            }
            if (strcmp(pw1, pw2) != 0) {
                fprintf(stderr, "Passwords do not match.\n");
                return 0;
            }
            if (!auth_set_password(db, pw1)) {
                fprintf(stderr, "Failed to set password.\n");
                return 0;
            }
            return 1;
        }
        fprintf(stderr, "Password not set. Use --set-password or run in menu mode.\n");
        return 0;
    }

    if (opt->do_set_password) {
        char current[128] = { 0 };
        if (opt->current_password) {
            snprintf(current, sizeof(current), "%s", opt->current_password);
        }
        else if (interactive) {
            if (!prompt_line("Current password: ", current, sizeof(current))) {
                return 0;
            }
        }
        else {
            fprintf(stderr, "Current password required. Use --current-password.\n");
            return 0;
        }
        if (!auth_verify_password(db, current)) {
            fprintf(stderr, "Invalid current password.\n");
            return 0;
        }

        char pw1[128] = { 0 };
        char pw2[128] = { 0 };
        if (opt->password) {
            snprintf(pw1, sizeof(pw1), "%s", opt->password);
            snprintf(pw2, sizeof(pw2), "%s", opt->password);
        }
        else {
            if (!prompt_line("Set new password: ", pw1, sizeof(pw1))) {
                return 0;
            }
            if (!prompt_line("Confirm password: ", pw2, sizeof(pw2))) {
                return 0;
            }
        }
        if (strcmp(pw1, pw2) != 0) {
            fprintf(stderr, "Passwords do not match.\n");
            return 0;
        }
        if (!auth_set_password(db, pw1)) {
            fprintf(stderr, "Failed to set password.\n");
            return 0;
        }
        return 1;
    }

    char pw[128] = { 0 };
    if (opt->password) {
        snprintf(pw, sizeof(pw), "%s", opt->password);
    }
    else if (interactive) {
        if (!prompt_line("\n..:: Enter password: ", pw, sizeof(pw))) {
            return 0;
        }
    }
    else {
        fprintf(stderr, "Password required. Use --password or run in menu mode.\n");
        return 0;
    }
    if (!auth_verify_password(db, pw)) {
        fprintf(stderr, "Invalid password.\n");
        return 0;
    }
    return 1;
}

static int parse_args(int argc, char** argv, Options* opt) {
    memset(opt, 0, sizeof(*opt));
    opt->db_path = DEFAULT_DB_PATH;

    for (int i = 1; i < argc; ++i) {
        const char* arg = argv[i];
        if (strcmp(arg, "--db") == 0 && i + 1 < argc) {
            opt->db_path = argv[++i];
        }
        else if (strcmp(arg, "--json") == 0) {
            opt->json = 1;
        }
        else if (strcmp(arg, "--dry-run") == 0) {
            opt->dry_run = 1;
        }
        else if (strcmp(arg, "--backup") == 0) {
            opt->backup = 1;
        }
        else if (strcmp(arg, "--strict") == 0) {
            opt->strict = 1;
        }
        else if (strcmp(arg, "--force") == 0) {
            opt->force = 1;
        }
        else if (strcmp(arg, "--menu") == 0) {
            opt->menu = 1;
        }
        else if (strcmp(arg, "--list") == 0) {
            opt->do_list = 1;
        }
        else if (strcmp(arg, "--stats") == 0) {
            opt->do_stats = 1;
        }
        else if (strcmp(arg, "--add") == 0) {
            opt->do_add = 1;
        }
        else if (strcmp(arg, "--edit") == 0) {
            opt->do_edit = 1;
        }
        else if (strcmp(arg, "--delete") == 0) {
            opt->do_delete = 1;
        }
        else if (strcmp(arg, "--delete-all") == 0) {
            opt->do_delete_all = 1;
        }
        else if (strcmp(arg, "--search") == 0 && i + 1 < argc) {
            opt->do_search = 1;
            opt->search = argv[++i];
        }
        else if (strcmp(arg, "--export") == 0 && i + 1 < argc) {
            opt->do_export = 1;
            opt->export_path = argv[++i];
        }
        else if (strcmp(arg, "--import") == 0 && i + 1 < argc) {
            opt->do_import = 1;
            opt->import_path = argv[++i];
        }
        else if (strcmp(arg, "--sort") == 0 && i + 1 < argc) {
            opt->do_sort = 1;
            opt->sort_mode = argv[++i];
        }
        else if (strcmp(arg, "--set-password") == 0) {
            opt->do_set_password = 1;
        }
        else if (strcmp(arg, "--password") == 0 && i + 1 < argc) {
            opt->password = argv[++i];
        }
        else if (strcmp(arg, "--current-password") == 0 && i + 1 < argc) {
            opt->current_password = argv[++i];
        }
        else if (strcmp(arg, "--name") == 0 && i + 1 < argc) {
            opt->name = argv[++i];
        }
        else if (strcmp(arg, "--phone") == 0 && i + 1 < argc) {
            opt->phone = argv[++i];
        }
        else if (strcmp(arg, "--address") == 0 && i + 1 < argc) {
            opt->address = argv[++i];
        }
        else if (strcmp(arg, "--email") == 0 && i + 1 < argc) {
            opt->email = argv[++i];
        }
        else if (strcmp(arg, "--due") == 0 && i + 1 < argc) {
            opt->due = argv[++i];
        }
        else if (strcmp(arg, "--due-date") == 0 && i + 1 < argc) {
            opt->due_date = argv[++i];
        }
        else if (strcmp(arg, "--id") == 0 && i + 1 < argc) {
            opt->id = argv[++i];
        }
        else if (strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0) {
            print_usage(stdout);
            return 0;
        }
        else {
            fprintf(stderr, "Unknown argument: %s\n", arg);
            print_usage(stderr);
            return 0;
        }
    }

    return 1;
}

static int do_backup_if_requested(const Options* opt, const char* db_path) {
    if (!opt->backup) {
        return 1;
    }
    if (!util_file_exists(db_path)) {
        return 1;
    }
    char backup_path[512];
    if (!util_make_backup(db_path, backup_path, sizeof(backup_path))) {
        fprintf(stderr, "Failed to create backup.\n");
        return 0;
    }
    printf("Backup created: %s\n", backup_path);
    return 1;
}

static int handle_non_interactive(Db* db, const Options* opt) {
    if (opt->do_list) {
        return contacts_list(db, opt->json, stdout);
    }
    if (opt->do_search) {
        char pattern[256];
        snprintf(pattern, sizeof(pattern), "%%%s%%", opt->search ? opt->search : "");
        return contacts_search_by_name(db, pattern, opt->json, stdout);
    }
    if (opt->do_stats) {
        ContactStats stats;
        if (!contacts_stats(db, &stats)) {
            return 0;
        }
        if (opt->json) {
            print_stats_json(stdout, &stats);
        }
        else {
            print_stats_plain(stdout, &stats);
        }
        return 1;
    }
    if (opt->do_add) {
        if (!opt->name) {
            fprintf(stderr, "--add requires --name\n");
            return 0;
        }
        Contact c = { 0 };
        snprintf(c.name, sizeof(c.name), "%s", opt->name);
        snprintf(c.phone, sizeof(c.phone), "%s", opt->phone ? opt->phone : "");
        snprintf(c.address, sizeof(c.address), "%s", opt->address ? opt->address : "");
        snprintf(c.email, sizeof(c.email), "%s", opt->email ? opt->email : "");
        if (opt->due_date) {
            if (!ensure_due_date_format(opt->due_date)) {
                fprintf(stderr, "Invalid due date format. Use YYYY-MM-DD.\n");
                return 0;
            }
            snprintf(c.due_date, sizeof(c.due_date), "%s", opt->due_date);
        }
        if (opt->due && !util_parse_double(opt->due, &c.due_amount, -1e12, 1e12)) {
            fprintf(stderr, "Invalid due amount.\n");
            return 0;
        }
        int64_t id = 0;
        if (!contacts_add(db, &c, &id)) {
            return 0;
        }
        printf("\nAdded contact with ID %lld\n", (long long)id);
        return 1;
    }
    if (opt->do_edit) {
        if (!opt->id) {
            fprintf(stderr, "--edit requires --id\n");
            return 0;
        }
        int64_t id = 0;
        if (!util_parse_i64(opt->id, &id, 1, INT64_MAX)) {
            fprintf(stderr, "Invalid ID.\n");
            return 0;
        }
        Contact c;
        if (!contacts_get_by_id(db, id, &c)) {
            fprintf(stderr, "Contact not found.\n");
            return 0;
        }
        if (opt->name) snprintf(c.name, sizeof(c.name), "%s", opt->name);
        if (opt->phone) snprintf(c.phone, sizeof(c.phone), "%s", opt->phone);
        if (opt->address) snprintf(c.address, sizeof(c.address), "%s", opt->address);
        if (opt->email) snprintf(c.email, sizeof(c.email), "%s", opt->email);
        if (opt->due_date) {
            if (!ensure_due_date_format(opt->due_date)) {
                fprintf(stderr, "Invalid due date format. Use YYYY-MM-DD.\n");
                return 0;
            }
            snprintf(c.due_date, sizeof(c.due_date), "%s", opt->due_date);
        }
        if (opt->due) {
            double v = 0.0;
            if (!util_parse_double(opt->due, &v, -1e12, 1e12)) {
                fprintf(stderr, "Invalid due amount.\n");
                return 0;
            }
            c.due_amount = v;
        }
        if (!contacts_update(db, &c)) {
            return 0;
        }
        printf("Updated contact %lld\n", (long long)c.id);
        return 1;
    }
    if (opt->do_delete) {
        if (!opt->id) {
            fprintf(stderr, "--delete requires --id\n");
            return 0;
        }
        int64_t id = 0;
        if (!util_parse_i64(opt->id, &id, 1, INT64_MAX)) {
            fprintf(stderr, "Invalid ID.\n");
            return 0;
        }
        if (!contacts_delete(db, id)) {
            return 0;
        }
        printf("Deleted contact %lld\n", (long long)id);
        return 1;
    }
    if (opt->do_delete_all) {
        if (!opt->force) {
            fprintf(stderr, "--delete-all requires --force\n");
            return 0;
        }
        if (!do_backup_if_requested(opt, db->path)) {
            return 0;
        }
        const char* sql = "DELETE FROM contacts;";
        char* err = NULL;
        if (sqlite3_exec(db->handle, sql, NULL, NULL, &err) != SQLITE_OK) {
            fprintf(stderr, "SQLite error: %s\n", err ? err : "unknown");
            sqlite3_free(err);
            return 0;
        }
        printf("All contacts deleted.\n");
        return 1;
    }
    if (opt->do_export) {
        FILE* f = fopen(opt->export_path, "wb");
        if (!f) {
            perror("Failed to open export file");
            return 0;
        }
        int ok = csv_write_contacts(db, f);
        fclose(f);
        return ok;
    }
    if (opt->do_import) {
        if (!do_backup_if_requested(opt, db->path)) {
            return 0;
        }
        FILE* f = fopen(opt->import_path, "rb");
        if (!f) {
            perror("Failed to open import file");
            return 0;
        }
        int imported = 0, failed = 0;
        int ok = csv_import_contacts(db, f, opt->strict, opt->dry_run, &imported, &failed);
        fclose(f);
        printf("Imported: %d, Failed: %d\n", imported, failed);
        return ok;
    }
    if (opt->do_sort) {
        if (!do_backup_if_requested(opt, db->path)) {
            return 0;
        }
        if (!contacts_set_sort_mode(db, opt->sort_mode ? opt->sort_mode : "name")) {
            fprintf(stderr, "Invalid sort mode.\n");
            return 0;
        }
        printf("Sort mode set to %s\n", opt->sort_mode ? opt->sort_mode : "name");
        return 1;
    }
    if (opt->do_set_password) {
        return ensure_auth(db, 0, opt);
    }
    print_usage(stdout);
    return 1;
}

static int interactive_menu(Db* db, const Options* opt) {
    (void)opt;
    for (;;) {
        printf("\n\t\t\t\t=== Contact Manager ===\n\n");
        printf("\t\t01. Add Contact\n");
        printf("\t\t02. List Contacts\n");
        printf("\t\t03. Search Contacts\n");
        printf("\t\t04. Edit Contact\n");
        printf("\t\t05. Delete Contact\n");
        printf("\t\t06. Export CSV\n");
        printf("\t\t07. Import CSV\n");
        printf("\t\t08. Set Password\n");
        printf("\t\t09. Statistics\n");
        printf("\t\t10. Sort Contacts\n");
        printf("\t\t0. Exit\n");
        printf("\n==> Select: ");

        char line[32];
        if (!util_read_line(stdin, line, sizeof(line))) {
            return 0;
        }
        util_trim(line);
        long choice = 0;
        if (!util_parse_long(line, &choice, 0, 10)) {
            printf("Invalid choice.\n");
            continue;
        }

        if (choice == 0) {
            return 1;
        }

        if (choice == 1) {
            Contact c = { 0 };
            if (!prompt_line("\n\t\t\tName: ", c.name, sizeof(c.name)) || !c.name[0]) {
                printf("Name is required.\n");
                continue;
            }
            prompt_line("\t\t\tPhone: ", c.phone, sizeof(c.phone));
            prompt_line("\t\t\tAddress: ", c.address, sizeof(c.address));
            prompt_line("\t\t\tEmail: ", c.email, sizeof(c.email));
            char due[64] = { 0 };
            prompt_line("\t\t\tDue amount: ", due, sizeof(due));
            if (due[0] && !util_parse_double(due, &c.due_amount, -1e12, 1e12)) {
                printf("Invalid due amount.\n");
                continue;
            }
            prompt_line("\t\t\tDue date (YYYY-MM-DD): ", c.due_date, sizeof(c.due_date));
            if (c.due_date[0] && !ensure_due_date_format(c.due_date)) {
                printf("Invalid due date; use YYYY-MM-DD.\n");
                continue;
            }
            int64_t id = 0;
            if (!contacts_add(db, &c, &id)) {
                printf("Failed to add contact.\n");
            }
            else {
                printf("\nAdded contact ID %lld\n", (long long)id);
            }
        }
        else if (choice == 2) {
            contacts_list(db, 0, stdout);
        }
        else if (choice == 3) {
            char query[128];
            prompt_line("\nSearch name: ", query, sizeof(query));
            char pattern[256];
            snprintf(pattern, sizeof(pattern), "%%%s%%", query);
            contacts_search_by_name(db, pattern, 0, stdout);
        }
        else if (choice == 4) {
            char idbuf[32];
            prompt_line("\nContact ID: ", idbuf, sizeof(idbuf));
            int64_t id = 0;
            if (!util_parse_i64(idbuf, &id, 1, INT64_MAX)) {
                printf("Invalid ID.\n");
                continue;
            }
            Contact c;
            if (!contacts_get_by_id(db, id, &c)) {
                printf("Contact not found.\n");
                continue;
            }
            printf("Leave blank to keep existing.\n");
            char buf[256];
            prompt_line("\nName: ", buf, sizeof(buf));
            if (buf[0]) util_copy_str(c.name, sizeof(c.name), buf);
            prompt_line("Phone: ", buf, sizeof(buf));
            if (buf[0]) util_copy_str(c.phone, sizeof(c.phone), buf);
            prompt_line("Address: ", buf, sizeof(buf));
            if (buf[0]) util_copy_str(c.address, sizeof(c.address), buf);
            prompt_line("Email: ", buf, sizeof(buf));
            if (buf[0]) util_copy_str(c.email, sizeof(c.email), buf);
            prompt_line("Due amount: ", buf, sizeof(buf));
            if (buf[0]) {
                double v = 0.0;
                if (!util_parse_double(buf, &v, -1e12, 1e12)) {
                    printf("Invalid due amount.\n");
                    continue;
                }
                c.due_amount = v;
            }
            prompt_line("Due date (YYYY-MM-DD): ", buf, sizeof(buf));
            if (buf[0]) {
                if (!ensure_due_date_format(buf)) {
                    printf("Invalid due date; use YYYY-MM-DD.\n");
                    continue;
                }
                util_copy_str(c.due_date, sizeof(c.due_date), buf);
            }
            if (!contacts_update(db, &c)) {
                printf("Failed to update contact.\n");
            }
            else {
                printf("Updated contact %lld\n", (long long)c.id);
            }
        }
        else if (choice == 5) {
            char idbuf[32];
            prompt_line("\nContact ID: ", idbuf, sizeof(idbuf));
            int64_t id = 0;
            if (!util_parse_i64(idbuf, &id, 1, INT64_MAX)) {
                printf("Invalid ID.\n");
                continue;
            }
            char confirm[8];
            prompt_line("Confirm delete (y/N): ", confirm, sizeof(confirm));
            if (confirm[0] != 'y' && confirm[0] != 'Y') {
                continue;
            }
            if (!contacts_delete(db, id)) {
                printf("Failed to delete contact.\n");
            }
            else {
                printf("Deleted contact %lld\n", (long long)id);
            }
        }
        else if (choice == 6) {
            char path[256];
            prompt_line("\nExport CSV path (eg. tests\\fixtures\\import.csv OR import.csv): ", path, sizeof(path));
            FILE* f = fopen(path, "wb");
            if (!f) {
                perror("Open failed");
                continue;
            }
            if (csv_write_contacts(db, f)) {
                printf("Exported to %s\n", path);
            }
            fclose(f);
        }
        else if (choice == 7) {
            char path[256];
            prompt_line("\nImport CSV path (eg. tests\\fixtures\\import.csv OR import.csv): ", path, sizeof(path));
            FILE* f = fopen(path, "rb");
            if (!f) {
                perror("Open failed");
                continue;
            }
            int imported = 0, failed = 0;
            if (csv_import_contacts(db, f, 0, 0, &imported, &failed)) {
                printf("\n\tImported: %d, Failed: %d\n", imported, failed);
            }
            fclose(f);
        }
        else if (choice == 8) {
            Options tmp = { 0 };
            tmp.do_set_password = 1;
            if (!ensure_auth(db, 1, &tmp)) {
                printf("Failed to set password.\n");
            }
        }
        else if (choice == 9) {
            ContactStats stats;
            if (contacts_stats(db, &stats)) {
                print_stats_plain(stdout, &stats);
            }
        }
        else if (choice == 10) {
            char mode[32];
            prompt_line("\nSort by name|phone|due_date: ", mode, sizeof(mode));
            if (contacts_set_sort_mode(db, mode)) {
                printf("\nSort mode set to %s (check 02. List Contacts to see)\n", mode);
            }
            else {
                printf("\nInvalid sort mode...!\n");
            }
        }
    }
}

int main(int argc, char** argv) {
    Options opt;
    if (!parse_args(argc, argv, &opt)) {
        return 1;
    }

    int interactive = opt.menu;
    if (!interactive) {
        if (!(opt.do_list || opt.do_stats || opt.do_add || opt.do_edit || opt.do_delete || opt.do_delete_all ||
            opt.do_search || opt.do_export || opt.do_import || opt.do_sort || opt.do_set_password)) {
            interactive = 1;
        }
    }

    Db db;
    if (!db_open(&db, opt.db_path)) {
        return 1;
    }
    if (!db_init(&db)) {
        db_close(&db);
        return 1;
    }

    if (!ensure_auth(&db, interactive, &opt)) {
        db_close(&db);
        return 1;
    }

    int ok = 1;
    if (interactive) {
        ok = interactive_menu(&db, &opt);
    }
    else {
        ok = handle_non_interactive(&db, &opt);
    }

    db_close(&db);
    return ok ? 0 : 1;
}
