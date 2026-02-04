// Purpose: Unit tests for util helpers. Author: GitHub Copilot
#include <stddef.h>
#include <stdarg.h>
#include <setjmp.h>
#include <cmocka.h>
#include "util.h"

static void test_parse_long(void** state) {
    (void)state;
    long v = 0;
    assert_true(util_parse_long("123", &v, 0, 200));
    assert_int_equal(v, 123);
    assert_false(util_parse_long("abc", &v, 0, 200));
}

static void test_parse_double(void** state) {
    (void)state;
    double v = 0.0;
    assert_true(util_parse_double("12.50", &v, 0, 100));
    assert_true(v > 12.49 && v < 12.51);
    assert_false(util_parse_double("bad", &v, 0, 100));
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_parse_long),
        cmocka_unit_test(test_parse_double),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
