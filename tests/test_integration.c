// Purpose: Integration tests for DB, CSV, and auth workflows. Author: GitHub Copilot
#include <stddef.h>
#include <stdarg.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdio.h>
#include <time.h>

#include "auth.h"
#include "contacts.h"
#include "csv.h"
#include "db.h"

static void format_relative_date(char* buf, size_t len, int offset_days) {
    time_t when = time(NULL) + (time_t)offset_days * 86400;
    struct tm tmv;
#if defined(_WIN32)
    localtime_s(&tmv, &when);
#else
    localtime_r(&when, &tmv);
#endif
    snprintf(buf, len, "%04d-%02d-%02d", tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday);
}

static void test_end_to_end(void** state) {
    (void)state;
    Db db;
    assert_true(db_open(&db, ":memory:"));
    assert_true(db_init(&db));

    Contact c1 = { 0 };
    snprintf(c1.name, sizeof(c1.name), "Bob");
    snprintf(c1.phone, sizeof(c1.phone), "555");
    c1.due_amount = 5.0;
    format_relative_date(c1.due_date, sizeof(c1.due_date), 1);

    Contact c2 = { 0 };
    snprintf(c2.name, sizeof(c2.name), "Cara");
    snprintf(c2.phone, sizeof(c2.phone), "777");
    c2.due_amount = 0.0;

    int64_t id1 = 0, id2 = 0;
    assert_true(contacts_add(&db, &c1, &id1));
    assert_true(contacts_add(&db, &c2, &id2));

    ContactStats stats;
    assert_true(contacts_stats(&db, &stats));
    assert_int_equal(stats.total_contacts, 2);
    assert_int_equal(stats.due_contacts, 1);
    assert_int_equal(stats.no_due_contacts, 1);
    assert_int_equal(stats.overdue_contacts, 0);
    assert_int_equal(stats.due_soon_contacts, 1);
    assert_int_equal(stats.due_date_present, 1);
    assert_int_equal(stats.due_date_missing, 1);
    assert_int_equal(stats.due_date_invalid, 0);
    assert_int_equal(stats.missing_phone, 0);
    assert_int_equal(stats.missing_email, 2);
    assert_int_equal(stats.missing_address, 2);
    assert_true(stats.total_due_amount > 4.999 && stats.total_due_amount < 5.001);
    assert_true(stats.avg_due_amount > 4.999 && stats.avg_due_amount < 5.001);
    assert_true(stats.min_due_amount > 4.999 && stats.min_due_amount < 5.001);
    assert_true(stats.max_due_amount > 4.999 && stats.max_due_amount < 5.001);
    assert_string_equal(stats.min_due_name, "Bob");
    assert_string_equal(stats.max_due_name, "Bob");
    assert_string_equal(stats.earliest_due_date, c1.due_date);
    assert_string_equal(stats.latest_due_date, c1.due_date);

    FILE* tmp = tmpfile();
    assert_non_null(tmp);
    assert_true(csv_write_contacts(&db, tmp));
    rewind(tmp);

    Db db2;
    assert_true(db_open(&db2, ":memory:"));
    assert_true(db_init(&db2));

    int imported = 0, failed = 0;
    assert_true(csv_import_contacts(&db2, tmp, 1, 0, &imported, &failed));
    assert_int_equal(imported, 2);
    assert_int_equal(failed, 0);

    assert_true(auth_set_password(&db2, "pass"));
    assert_true(auth_verify_password(&db2, "pass"));

    fclose(tmp);
    db_close(&db2);
    db_close(&db);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_end_to_end),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
