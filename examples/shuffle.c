#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <err.h>
#include <sys/time.h>

#include "itest-abbrev.h"

#define TEST_COUNT 1000

static char test_has_run[(TEST_COUNT / 8) + 1];

/* Don't bother complaining about tests not being run if listing tests
 * or name-based filtering means not all tests are being run. */
static int
running_all(void)
{
    if (ITEST_LIST_ONLY()) {
        return 0;
    }
    if (itest_info.test_filter != NULL || itest_info.suite_filter != NULL) {
        return 0;
    }
    return 1;
}

static int
check_run(unsigned int id)
{
    size_t offset     = id / 8;
    unsigned char bit = (unsigned char)(1U << (id & 0x07));
    return 0 != (test_has_run[offset] & bit);
}

static void
set_run(unsigned int id)
{
    size_t offset     = id / 8;
    unsigned char bit = (unsigned char)(1U << (id & 0x07));
    test_has_run[offset] |= bit;
}

static void
reset_run(void)
{
    memset(test_has_run, 0x00, sizeof(test_has_run));
}

static int print_flag = 0;

TEST
print_check_runs_and_pass(void *pid)
{
    unsigned int id = (unsigned int)(uintptr_t)pid;
    if (print_flag) {
        printf("running test %u\n", id);
    }
    if (check_run(id)) {
        fprintf(stderr, "Error: Shuffling made test run multiple times!\n");
        assert(0);
    } else {
        set_run(id);
    }
    PASS();
}

TEST
just_fail(void)
{
    FAIL();
}

static unsigned int
seed_of_time(void)
{
#if 0
    static unsigned int counter = 1;
    struct timeval tv;
    if (0 != gettimeofday(&tv, NULL)) {
        err(1, "gettimeofday");
    }
    return (unsigned int)(~(tv.tv_sec ^ tv.tv_usec) * counter++);
#else
    /* Deterministic seed used because our current excuse for a test
       suite cannot handle actual nondeterminism.  FIXME.  */
    return 0x31415926;
#endif
}

static char suffix_buf[11];

static void
set_suffix(unsigned int i)
{
    /* Don't suffix with 0, just to mix in one without a suffix,
     * to test conditionally including a "_" separator. */
    if (i > 0) {
        snprintf(suffix_buf, sizeof suffix_buf, "%u", i);
        itest_set_test_suffix(suffix_buf);
    }
}

SUITE(suite1)
{
    const unsigned int limit = TEST_COUNT;
    volatile unsigned int count;
    const unsigned int small_test_count = 11;
    volatile unsigned int i             = 0;

    /* Check that all are run exactly once, for a small number of tests */
    print_flag = 1;
    for (count = 0; count < small_test_count; count++) {
        unsigned int seed = seed_of_time();
        fprintf(stderr, "count %u, seed %u\n", count, seed);
#define COUNT_RUN(X)                                                         \
    do {                                                                     \
        if (count > (X)) {                                                   \
            set_suffix(X);                                                   \
            RUN_TEST1(print_check_runs_and_pass, (void *)(X));               \
        }                                                                    \
    } while (0)

        SHUFFLE_TESTS(seed, {
            COUNT_RUN(0);
            COUNT_RUN(1);
            COUNT_RUN(2);
            COUNT_RUN(3);
            COUNT_RUN(4);
            COUNT_RUN(5);
            COUNT_RUN(6);
            COUNT_RUN(7);
            COUNT_RUN(8);
            COUNT_RUN(9);
            COUNT_RUN(10);
        });
#undef COUNT_RUN

        for (i = 0; i < count; i++) {
            if (running_all() && !check_run(i)) {
                fprintf(stderr, "Error: Test %u got lost in the shuffle!\n",
                        i);
                assert(0);
            }
        }
        reset_run();
    }
    print_flag = 0;

    /* Check that all are run exactly once, for a larger amount of tests */
    SHUFFLE_TESTS(seed_of_time(), {
        for (i = 0; i < limit; i++) {
            set_suffix(i);
            RUN_TEST1(print_check_runs_and_pass, (void *)(uintptr_t)i);
        }
    });

    for (i = 0; i < limit; i++) {
        if (running_all() && !check_run(i)) {
            fprintf(stderr, "Error: Test %u got lost in the shuffle!\n", i);
            assert(0);
        }
    }
}

TEST
just_print_and_pass(void *pid)
{
    unsigned int id = (unsigned int)(uintptr_t)pid;
    printf("running test from suite %u\n", id);
    PASS();
}

/* A few other single-function suites */
SUITE(suite2)
{
    set_suffix(2);
    RUN_TEST1(just_print_and_pass, (void *)(uintptr_t)2);
}
SUITE(suite3)
{
    set_suffix(3);
    RUN_TEST1(just_print_and_pass, (void *)(uintptr_t)3);
}
SUITE(suite4)
{
    set_suffix(4);
    RUN_TEST1(just_print_and_pass, (void *)(uintptr_t)4);
}
SUITE(suite5)
{
    set_suffix(5);
    RUN_TEST1(just_print_and_pass, (void *)(uintptr_t)5);
}

SUITE(suite_failure)
{
    RUN_TEST(just_fail);
}

SUITE(suite_shuffle_pass_and_failure)
{
    SHUFFLE_TESTS(seed_of_time(), {
        set_suffix(1);
        RUN_TEST1(just_print_and_pass, (void *)(uintptr_t)1);
        set_suffix(2);
        RUN_TEST1(just_print_and_pass, (void *)(uintptr_t)2);
        set_suffix(3);
        RUN_TEST1(just_print_and_pass, (void *)(uintptr_t)3);
        set_suffix(4);
        RUN_TEST1(just_print_and_pass, (void *)(uintptr_t)4);
        set_suffix(5);
        RUN_TEST1(just_print_and_pass, (void *)(uintptr_t)5);
        RUN_TEST(just_fail);
    });
}

/* PRNG internal state assumes uint32_t values */
static_assert(sizeof(itest_info.prng[0].state) >= 4, "PRNG state too small");
static_assert(sizeof(itest_info.prng[0].a) >= 4, "PRNG state too small");
static_assert(sizeof(itest_info.prng[0].c) >= 4, "PRNG state too small");
static_assert(sizeof(itest_info.prng[0].m) >= 4, "PRNG state too small");

int
main(int argc, char **argv)
{
    itest_init();
    itest_parse_options(argc, argv);

    SHUFFLE_SUITES(seed_of_time(), {
        RUN_SUITE(suite1);
        RUN_SUITE(suite2);
        RUN_SUITE(suite3);
        RUN_SUITE(suite4);
        RUN_SUITE(suite5);
        RUN_SUITE(suite_shuffle_pass_and_failure);
        RUN_SUITE(suite_failure);
    });

    return itest_print_report();
}
