#include <stddef.h>
#include <stdarg.h>
#include <setjmp.h>
// Purpose: Unit tests for CSV parsing/writing. Author: GitHub Copilot
#include <cmocka.h>
#include <stdio.h>

#include "csv.h"
#include "contacts.h"
#include "db.h"

static void test_csv_roundtrip(void** state) {
    (void)state;
    Db db;
    assert_true(db_open(&db, ":memory:"));
    assert_true(db_init(&db));

    Contact c = { 0 };
    snprintf(c.name, sizeof(c.name), "Alice, \"Quote\"");
    snprintf(c.phone, sizeof(c.phone), "123");
    snprintf(c.address, sizeof(c.address), "Line1\nLine2");
    snprintf(c.email, sizeof(c.email), "a@example.com");
    c.due_amount = 12.5;
    snprintf(c.due_date, sizeof(c.due_date), "2026-01-01");
    int64_t id = 0;
    assert_true(contacts_add(&db, &c, &id));

    FILE* tmp = tmpfile();
    assert_non_null(tmp);
    assert_true(csv_write_contacts(&db, tmp));
    rewind(tmp);

    Db db2;
    assert_true(db_open(&db2, ":memory:"));
    assert_true(db_init(&db2));
    int imported = 0, failed = 0;
    assert_true(csv_import_contacts(&db2, tmp, 1, 0, &imported, &failed));
    assert_int_equal(imported, 1);
    assert_int_equal(failed, 0);

    fclose(tmp);
    db_close(&db2);
    db_close(&db);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_csv_roundtrip),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
