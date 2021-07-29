/*********************************************************************
 * Minimal test runner template
 *********************************************************************/
#include "itest-abbrev.h"

TEST
foo_should_foo(void)
{
    PASS();
}

static void
setup_cb(void *data)
{
    printf("setup callback for each test case\n");
}

static void
teardown_cb(void *data)
{
    printf("teardown callback for each test case\n");
}

SUITE(suite)
{
    /* Optional setup/teardown callbacks which will be run before/after
     * every test case. If using a test suite, they will be cleared when
     * the suite finishes. */
    SET_SETUP(setup_cb, 0 /* passed as 'data' argument to callback */);
    SET_TEARDOWN(teardown_cb, 0 /* passed as 'data' argument to callback */);

    RUN_TEST(foo_should_foo);
}

/* You would typically choose one of the following: */
#if 0
/* Set up, run suite(s) of tests, report pass/fail/skip stats. */
int
run_tests(void)
{
    itest_init(); /* init. itest internals */
    /* List of suites to run (if any). */
    RUN_SUITE(suite);

    /* Tests can also be run directly, without using test suites. */
    RUN_TEST(foo_should_foo);

    itest_print_report(); /* display results */
    return itest_all_passed();
}
#else
/* main(), for a standalone command-line test runner.
 * This replaces run_tests above, and adds command line option
 * handling and exiting with a pass/fail status. */
int
main(int argc, char **argv)
{
    ITEST_MAIN_BEGIN(); /* init & parse command-line args */
    RUN_SUITE(suite);
    ITEST_MAIN_END(); /* display results */
}
#endif
