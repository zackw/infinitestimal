/*
 * Copyright (c) 2011-2021 Scott Vokes <vokes.s@gmail.com>
 * Copyright (c) 2021 Zack Weinberg <zackw@panix.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef ITEST_H
#define ITEST_H

#if defined(__cplusplus)
extern "C" {
#endif

/* 2.0.0 */
#define ITEST_VERSION_MAJOR 2
#define ITEST_VERSION_MINOR 0
#define ITEST_VERSION_PATCH 0

/* A unit testing system for C.
 *
 * An up-to-date version can be found at:
 *     https://github.com/zackw/infinitestmal/
 *
 * Originally a fork of greatest:
 *     https://github.com/silentbicycle/greatest/
 */

/*********************************************************************/

#include <ctype.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/***********
 * Options *
 ***********/

/* Default column width for non-verbose output. */
#ifndef ITEST_DEFAULT_WIDTH
#    define ITEST_DEFAULT_WIDTH 72
#endif

/* FILE *, for test logging. */
#ifndef ITEST_STDOUT
#    define ITEST_STDOUT stdout
#endif

/* Set to 0 to disable all use of time.h / clock(). */
#ifndef ITEST_USE_TIME
#    define ITEST_USE_TIME 1
#endif

#if ITEST_USE_TIME
#    include <time.h>
#endif

/* Size of buffer for test name + optional '_' separator and suffix */
#ifndef ITEST_TESTNAME_BUF_SIZE
#    define ITEST_TESTNAME_BUF_SIZE 128
#endif

/* Declaring a function that does not return (if we can) */
#if defined __cplusplus && __cplusplus >= 201103L
#    define ITEST_NORETURN [[noreturn]] void
#elif defined __STDC_VERSION__ && __STDC_VERSION__ >= 201112L
#    define ITEST_NORETURN _Noreturn void
#elif defined __GNUC__ && __GNUC__ >= 3
#    define ITEST_NORETURN void __attribute__((__noreturn__))
#else
#    define ITEST_NORETURN void
#endif

/* Declaring a function that takes a printf format string (if we can) */
#if defined __GNUC__ && __GNUC__ >= 3
#    define ITEST_PRINTFLIKE(x, y)                                           \
        __attribute__((__format__(__printf__, x, y)))
#else
#    define ITEST_PRINTFLIKE(x, y) /* nothing */
#endif

/*********
 * Types *
 *********/

/* PASS/FAIL/SKIP result from a test. Used internally.
   ITEST_TEST_RES_PASS must be zero, other statuses should be positive.  */
typedef enum itest_test_res
{
    ITEST_TEST_RES_PASS = 0,
    ITEST_TEST_RES_FAIL = 1,
    ITEST_TEST_RES_SKIP = 2
} itest_test_res;

/* Info for the current running suite. */
typedef struct itest_suite_info
{
    unsigned int tests_run;
    unsigned int passed;
    unsigned int failed;
    unsigned int skipped;

#if ITEST_USE_TIME
    /* timers, pre/post running suite and individual tests */
    clock_t pre_suite;
    clock_t post_suite;
    clock_t pre_test;
    clock_t post_test;
#endif
} itest_suite_info;

/* Type for a suite function. */
typedef void itest_suite_cb(void);

/* Type for a test function with no arguments.  */
typedef void itest_test_cb(void);

/* Type for a test function with one argument (an environment pointer).  */
typedef void itest_test_env_cb(void *udata);

/* Types for setup/teardown callbacks. If non-NULL, these will be run
 * and passed the pointer to their additional data. */
typedef void itest_setup_cb(void *udata);
typedef void itest_teardown_cb(void *udata);

/* Type for an equality comparison between two pointers of the same type.
 * Should return non-0 if equal, otherwise 0.
 * UDATA is a closure value, passed through from ASSERT_EQUAL_T[m]. */
typedef int itest_equal_cb(const void *expd, const void *got, void *udata);

/* Type for a callback that prints a value pointed to by T.
 * Return value has the same meaning as printf's.
 * UDATA is a closure value, passed through from ASSERT_EQUAL_T[m]. */
typedef int itest_printf_cb(const void *t, void *udata);

/* Callbacks for an arbitrary type; needed for type-specific
 * comparisons via ITEST_ASSERT_EQUAL_T[m].*/
typedef struct itest_type_info
{
    itest_equal_cb *equal;
    itest_printf_cb *print;
} itest_type_info;

typedef enum
{
    ITEST_FLAG_FIRST_FAIL    = 0x01,
    ITEST_FLAG_LIST_ONLY     = 0x02,
    ITEST_FLAG_ABORT_ON_FAIL = 0x04
} itest_flag_t;

/* Internal state for a PRNG, used to shuffle test order. */
struct itest_prng
{
    unsigned char random_order; /* use random ordering? */
    unsigned char initialized;  /* is random ordering initialized? */
    unsigned char pad_0[6];
    unsigned long state;      /* PRNG state */
    unsigned long count;      /* how many tests, this pass */
    unsigned long count_ceil; /* total number of tests */
    unsigned long count_run;  /* total tests run */
    unsigned long a;          /* LCG multiplier */
    unsigned long c;          /* LCG increment */
    unsigned long m;          /* LCG modulus, based on count_ceil */
};

/* Struct containing all test runner state. */
typedef struct itest_run_info
{
    unsigned char flags;
    unsigned char verbosity;
    unsigned char running_test; /* guard for nested RUN_TEST calls */
    unsigned char exact_name_match;

    unsigned int tests_run; /* total test count */

    /* currently running test suite */
    itest_suite_info suite;

    /* overall pass/fail/skip counts */
    unsigned int passed;
    unsigned int failed;
    unsigned int skipped;
    unsigned int assertions;

    /* info to print about the most recent failure */
    unsigned int fail_line;
    unsigned int pad_1;
    const char *fail_file;
    const char *msg;

    /* current setup/teardown hooks and userdata */
    itest_setup_cb *setup;
    void *setup_udata;
    itest_teardown_cb *teardown;
    void *teardown_udata;

    /* formatting info for ".....s...F"-style output */
    unsigned int col;
    unsigned int width;

    /* only run a specific suite or test */
    const char *suite_filter;
    const char *test_filter;
    const char *test_exclude;
    const char *name_suffix; /* print suffix with test name */
    char name_buf[ITEST_TESTNAME_BUF_SIZE];

    struct itest_prng prng[2]; /* 0: suites, 1: tests */

#if ITEST_USE_TIME
    /* overall timers */
    clock_t begin;
    clock_t end;
#endif

    jmp_buf jump_dest;
} itest_run_info;

struct itest_report_t
{
    /* overall pass/fail/skip counts */
    unsigned int passed;
    unsigned int failed;
    unsigned int skipped;
    unsigned int assertions;
};

/* Global var for the current testing context.
 * Initialized by ITEST_MAIN_DEFS(). */
extern itest_run_info itest_info;

/* Type for ASSERT_ENUM_EQ's ENUM_STR argument. */
typedef const char *itest_enum_str_fun(int value);

/**********************
 * Exported functions *
 **********************/

/* These are used internally by itest macros. */
int itest_test_pre(const char *name);
void itest_test_post(int res);

void itest_assert(const char *msg, const char *file, unsigned int line,
                  int cond);
void itest_assert_eq_fmt(const char *msg, const char *file, unsigned int line,
                         const char *fmt, int cond, ...)
    ITEST_PRINTFLIKE(4, 6);
void itest_assert_eq_enum(const char *msg, const char *file,
                          unsigned int line, itest_enum_str_fun enum_str,
                          int exp, int got);
void itest_assert_in_range(const char *msg, const char *file,
                           unsigned int line, double exp, double got,
                           double tol);
void itest_assert_equal_str(const char *expd, const char *got,
                            const char *file, unsigned int line,
                            const char *msg);
void itest_assert_equal_strn(const char *expd, const char *got, size_t size,
                             const char *file, unsigned int line,
                             const char *msg);
void itest_assert_equal_mem(const void *expd, const void *got, size_t size,
                            const char *file, unsigned int line,
                            const char *msg);
void itest_assert_equal_t(const void *expd, const void *got,
                          const itest_type_info *type_info, void *udata,
                          const char *file, unsigned int line,
                          const char *msg);

void itest_prng_init_first_pass(int id);
int itest_prng_init_second_pass(int id, unsigned long seed);
void itest_prng_step(int id);

/* These are part of the public itest API. */
void itest_set_setup_cb(itest_setup_cb *cb, void *udata);
void itest_set_teardown_cb(itest_teardown_cb *cb, void *udata);
void itest_init(void);
void itest_parse_options(int argc, char **argv);
int itest_print_report(void);
int itest_all_passed(void);
void itest_run_suite(itest_suite_cb *suite_cb, const char *suite_name);
void itest_run_test(itest_test_cb *test_cb, const char *test_name);
void itest_run_test_with_env(itest_test_env_cb *test_cb,
                             const char *test_name, void *env);
void itest_set_suite_filter(const char *filter);
void itest_set_test_filter(const char *filter);
void itest_set_test_exclude(const char *filter);
void itest_set_exact_name_match(void);
void itest_stop_at_first_fail(void);
void itest_abort_on_fail(void);
void itest_list_only(void);
void itest_get_report(struct itest_report_t *report);
unsigned int itest_get_verbosity(void);
void itest_set_verbosity(unsigned int verbosity);
void itest_set_flag(itest_flag_t flag);
void itest_set_test_suffix(const char *suffix);
ITEST_NORETURN itest_fail(const char *msg, const char *file,
                          unsigned int line);
ITEST_NORETURN itest_skip(const char *msg, const char *file,
                          unsigned int line);

/**********
 * Macros *
 **********/

/* Define a suite. (The duplication is intentional -- it eliminates
 * a warning from -Wmissing-declarations.) */
#define ITEST_SUITE(NAME)                                                    \
    void NAME(void);                                                         \
    void NAME(void)

/* Declare a suite, provided by another compilation unit. */
#define ITEST_SUITE_EXTERN(NAME) extern void NAME(void)

/* Start defining a test function.
 * The arguments are not included, to allow parametric testing. */
#define ITEST_TEST static void

/* Run a suite. */
#define ITEST_RUN_SUITE(S_NAME) itest_run_suite(S_NAME, #S_NAME)

/* Run test function TEST in the current suite, supplying no arguments. */
#define ITEST_RUN_TEST(TEST) itest_run_test(TEST, #TEST)

/* Run test function TEST in the current suite, supplying one argument,
   which is a `void *`.  */
#define ITEST_RUN_TEST1(TEST, ENV) itest_run_test_with_env(TEST, #TEST, ENV)

/* Ignore test function TEST, don't warn about it being unused. */
#define ITEST_IGNORE_TEST(TEST) (void)TEST

/* Check if the test runner is in verbose mode. */
#define ITEST_IS_VERBOSE()    ((itest_info.verbosity) > 0)
#define ITEST_LIST_ONLY()     (itest_info.flags & ITEST_FLAG_LIST_ONLY)
#define ITEST_FIRST_FAIL()    (itest_info.flags & ITEST_FLAG_FIRST_FAIL)
#define ITEST_ABORT_ON_FAIL() (itest_info.flags & ITEST_FLAG_ABORT_ON_FAIL)
#define ITEST_FAILURE_ABORT()                                                \
    (ITEST_FIRST_FAIL()                                                      \
     && (itest_info.suite.failed > 0 || itest_info.failed > 0))

/* Message-less forms of tests defined below. */
#define ITEST_FAIL()              ITEST_FAILm(NULL)
#define ITEST_SKIP()              ITEST_SKIPm(NULL)
#define ITEST_ASSERT(COND)        ITEST_ASSERTm(#COND, COND)
#define ITEST_ASSERT_FALSE(COND)  ITEST_ASSERT_FALSEm(#COND, COND)
#define ITEST_ASSERT_EQ(EXP, GOT) ITEST_ASSERT_EQm(#EXP " != " #GOT, EXP, GOT)
#define ITEST_ASSERT_NEQ(EXP, GOT)                                           \
    ITEST_ASSERT_NEQm(#EXP " == " #GOT, EXP, GOT)
#define ITEST_ASSERT_GT(EXP, GOT) ITEST_ASSERT_GTm(#EXP " <= " #GOT, EXP, GOT)
#define ITEST_ASSERT_GTE(EXP, GOT)                                           \
    ITEST_ASSERT_GTEm(#EXP " < " #GOT, EXP, GOT)
#define ITEST_ASSERT_LT(EXP, GOT) ITEST_ASSERT_LTm(#EXP " >= " #GOT, EXP, GOT)
#define ITEST_ASSERT_LTE(EXP, GOT)                                           \
    ITEST_ASSERT_LTEm(#EXP " > " #GOT, EXP, GOT)
#define ITEST_ASSERT_EQ_FMT(EXP, GOT, FMT)                                   \
    ITEST_ASSERT_EQ_FMTm(#EXP " != " #GOT, EXP, GOT, FMT)
#define ITEST_ASSERT_IN_RANGE(EXP, GOT, TOL)                                 \
    ITEST_ASSERT_IN_RANGEm(#EXP " != " #GOT " +/- " #TOL, EXP, GOT, TOL)
#define ITEST_ASSERT_EQUAL_T(EXP, GOT, TYPE_INFO, UDATA)                     \
    ITEST_ASSERT_EQUAL_Tm(#EXP " != " #GOT, EXP, GOT, TYPE_INFO, UDATA)
#define ITEST_ASSERT_STR_EQ(EXP, GOT)                                        \
    ITEST_ASSERT_STR_EQm(#EXP " != " #GOT, EXP, GOT)
#define ITEST_ASSERT_STRN_EQ(EXP, GOT, SIZE)                                 \
    ITEST_ASSERT_STRN_EQm(#EXP " != " #GOT, EXP, GOT, SIZE)
#define ITEST_ASSERT_MEM_EQ(EXP, GOT, SIZE)                                  \
    ITEST_ASSERT_MEM_EQm(#EXP " != " #GOT, EXP, GOT, SIZE)
#define ITEST_ASSERT_ENUM_EQ(EXP, GOT, ENUM_STR)                             \
    ITEST_ASSERT_ENUM_EQm(#EXP " != " #GOT, EXP, GOT, ENUM_STR)

/* The following forms take an additional message argument first,
 * to be displayed by the test runner. */

/* Fail if a condition is not true, with message. */
#define ITEST_ASSERTm(MSG, COND) itest_assert(MSG, __FILE__, __LINE__, COND)

/* Fail if a condition is not false, with message. */
#define ITEST_ASSERT_FALSEm(MSG, COND)                                       \
    itest_assert(MSG, __FILE__, __LINE__, !(COND))

/* Internal macro for relational assertions */
#define ITEST__REL(REL, MSG, EXP, GOT)                                       \
    itest_assert(MSG, __FILE__, __LINE__, ((EXP)REL(GOT)))

/* Fail if EXP is not ==, !=, >, <, >=, or <= to GOT. */
#define ITEST_ASSERT_EQm(MSG, E, G)  ITEST__REL(==, MSG, E, G)
#define ITEST_ASSERT_NEQm(MSG, E, G) ITEST__REL(!=, MSG, E, G)
#define ITEST_ASSERT_GTm(MSG, E, G)  ITEST__REL(>, MSG, E, G)
#define ITEST_ASSERT_GTEm(MSG, E, G) ITEST__REL(>=, MSG, E, G)
#define ITEST_ASSERT_LTm(MSG, E, G)  ITEST__REL(<, MSG, E, G)
#define ITEST_ASSERT_LTEm(MSG, E, G) ITEST__REL(<=, MSG, E, G)

/* Fail if EXP != GOT (equality comparison by ==).  FMT must be a
 * string literal containing a single printf format specifier which
 * agrees with  the type of both EXP and GOT.
 * Warning: EXP and GOT will be evaluated exactly twice each.  */
#define ITEST_ASSERT_EQ_FMTm(MSG, EXP, GOT, FMT)                             \
    itest_assert_eq_fmt(MSG, __FILE__, __LINE__,                             \
                        "\nExpected: " FMT "\n     Got: " FMT "\n",          \
                        (GOT) == (EXP), EXP, GOT);

/* Fail if EXP is not equal to GOT, printing enum IDs. */
#define ITEST_ASSERT_ENUM_EQm(MSG, EXP, GOT, ENUM_STR)                       \
    itest_assert_eq_enum(MSG, __FILE__, __LINE__, ENUM_STR, (int)(EXP),      \
                         (int)(GOT))

/* Fail if GOT not in range of EXP +|- TOL. */
#define ITEST_ASSERT_IN_RANGEm(MSG, EXP, GOT, TOL)                           \
    itest_assert_in_range(MSG, __FILE__, __LINE__, EXP, GOT, TOL)

/* Fail if EXP is not equal to GOT, according to strcmp. */
#define ITEST_ASSERT_STR_EQm(MSG, EXP, GOT)                                  \
    itest_assert_equal_str(EXP, GOT, __FILE__, __LINE__, MSG)

/* Fail if EXP is not equal to GOT, according to strncmp. */
#define ITEST_ASSERT_STRN_EQm(MSG, EXP, GOT, SIZE)                           \
    itest_assert_equal_strn(EXP, GOT, SIZE, __FILE__, __LINE__, MSG)

/* Fail if EXP is not equal to GOT, according to memcmp. */
#define ITEST_ASSERT_MEM_EQm(MSG, EXP, GOT, SIZE)                            \
    itest_assert_equal_mem(EXP, GOT, SIZE, __FILE__, __LINE__, MSG)

/* Fail if EXP is not equal to GOT, according to a comparison
 * callback in TYPE_INFO. If they are not equal, optionally use a
 * print callback in TYPE_INFO to print them. */
#define ITEST_ASSERT_EQUAL_Tm(MSG, EXP, GOT, TYPE_INFO, UDATA)               \
    itest_assert_equal_t(EXP, GOT, TYPE_INFO, UDATA, __FILE__, __LINE__, MSG)

/* Fail. */
#define ITEST_FAILm(MSG) itest_fail(MSG, __FILE__, __LINE__)

/* Skip the current test. */
#define ITEST_SKIPm(MSG) itest_skip(MSG, __FILE__, __LINE__)

/* Run every suite / test function run within BODY in pseudo-random
 * order, seeded by SEED. (The top 3 bits of the seed are ignored.)
 *
 * This should be called like:
 *     ITEST_SHUFFLE_TESTS(seed, {
 *         ITEST_RUN_TEST(some_test);
 *         ITEST_RUN_TEST(some_other_test);
 *         ITEST_RUN_TEST(yet_another_test);
 *     });
 *
 * Note that the body of the second argument will be evaluated
 * multiple times. */
#define ITEST_SHUFFLE_SUITES(SD, BODY) ITEST_SHUFFLE(0, SD, BODY)
#define ITEST_SHUFFLE_TESTS(SD, BODY)  ITEST_SHUFFLE(1, SD, BODY)
#define ITEST_SHUFFLE(ID, SD, BODY)                                          \
    do {                                                                     \
        struct itest_prng *prng = &itest_info.prng[ID];                      \
        itest_prng_init_first_pass(ID);                                      \
        do {                                                                 \
            prng->count = 0;                                                 \
            if (prng->initialized) {                                         \
                itest_prng_step(ID);                                         \
            }                                                                \
            BODY; /* NOLINT(bugprone-macro-parentheses) */                   \
            if (!prng->initialized) {                                        \
                if (!itest_prng_init_second_pass(ID, SD)) {                  \
                    break;                                                   \
                }                                                            \
            } else if (prng->count_run == prng->count_ceil) {                \
                break;                                                       \
            }                                                                \
        } while (!ITEST_FAILURE_ABORT());                                    \
        prng->count_run = prng->random_order = prng->initialized = 0;        \
    } while (0)

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif /* itest.h */
