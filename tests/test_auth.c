// Purpose: Unit tests for password hashing. Author: GitHub Copilot
#include <stddef.h>
#include <stdarg.h>
#include <setjmp.h>
#include <cmocka.h>

#include "auth.h"
#include "db.h"

static void test_password_hash(void** state) {
    (void)state;
    Db db;
    assert_true(db_open(&db, ":memory:"));
    assert_true(db_init(&db));

    assert_true(auth_set_password(&db, "secret"));
    assert_true(auth_verify_password(&db, "secret"));
    assert_false(auth_verify_password(&db, "wrong"));

    db_close(&db);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_password_hash),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
