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
 * It doesn't use dynamic allocation or depend on anything
 * beyond ANSI C89.
 *
 * An up-to-date version can be found at:
 *     https://github.com/zackw/infinitestmal/
 *
 * Originally a fork of greatest:
 *     https://github.com/silentbicycle/greatest/
 */

/*********************************************************************/

#include <ctype.h>
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

/* Set to 0 to disable all use of setjmp/longjmp. */
#ifndef ITEST_USE_LONGJMP
#    define ITEST_USE_LONGJMP 0
#endif

/* Make it possible to replace fprintf with another
 * function with the same interface. */
#ifndef ITEST_FPRINTF
#    define ITEST_FPRINTF fprintf
#endif

#if ITEST_USE_LONGJMP
#    include <setjmp.h>
#endif

/* Set to 0 to disable all use of time.h / clock(). */
#ifndef ITEST_USE_TIME
#    define ITEST_USE_TIME 1
#endif

#if ITEST_USE_TIME
#    include <time.h>
#endif

/* Floating point type, for ASSERT_IN_RANGE. */
#ifndef ITEST_FLOAT
#    define ITEST_FLOAT     double
#    define ITEST_FLOAT_FMT "%g"
#endif

/* Size of buffer for test name + optional '_' separator and suffix */
#ifndef ITEST_TESTNAME_BUF_SIZE
#    define ITEST_TESTNAME_BUF_SIZE 128
#endif

/*********
 * Types *
 *********/

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

typedef struct itest_memory_cmp_env
{
    const unsigned char *exp;
    const unsigned char *got;
    size_t size;
} itest_memory_cmp_env;

/* Callbacks for string and raw memory types. */
extern itest_type_info itest_type_info_string;
extern itest_type_info itest_type_info_memory;

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

#if ITEST_USE_LONGJMP
    int pad_jmp_buf;
    unsigned char pad_2[4];
    jmp_buf jump_dest;
#endif
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
int itest_do_assert_equal_t(const void *expd, const void *got,
                            itest_type_info *type_info, void *udata);
void itest_prng_init_first_pass(int id);
int itest_prng_init_second_pass(int id, unsigned long seed);
void itest_prng_step(int id);

/* These are part of the public itest API. */
void ITEST_SET_SETUP_CB(itest_setup_cb *cb, void *udata);
void ITEST_SET_TEARDOWN_CB(itest_teardown_cb *cb, void *udata);
void ITEST_INIT(void);
void itest_parse_options(int argc, char **argv);
void ITEST_PRINT_REPORT(void);
int itest_all_passed(void);
void itest_run_suite(itest_suite_cb *suite_cb, const char *suite_name);
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

/********************
 * Language Support *
 ********************/

/* If __VA_ARGS__ (C99) is supported, allow parametric testing
 * without needing to manually manage the argument struct. */
#if (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 19901L)                \
    || (defined(_MSC_VER) && _MSC_VER >= 1800)
#    define ITEST_VA_ARGS
#endif

/**********
 * Macros *
 **********/

/* Define a suite. (The duplication is intentional -- it eliminates
 * a warning from -Wmissing-declarations.) */
#define ITEST_SUITE(NAME)                                                    \
    void NAME(void);                                                         \
    void NAME(void)

/* Declare a suite, provided by another compilation unit. */
#define ITEST_SUITE_EXTERN(NAME) void NAME(void)

/* Start defining a test function.
 * The arguments are not included, to allow parametric testing. */
#define ITEST_TEST static enum itest_test_res

/* PASS/FAIL/SKIP result from a test. Used internally. */
typedef enum itest_test_res
{
    ITEST_TEST_RES_PASS = 0,
    ITEST_TEST_RES_FAIL = -1,
    ITEST_TEST_RES_SKIP = 1
} itest_test_res;

/* Run a suite. */
#define ITEST_RUN_SUITE(S_NAME) itest_run_suite(S_NAME, #S_NAME)

/* Run a test in the current suite. */
#define ITEST_RUN_TEST(TEST)                                                 \
    do {                                                                     \
        if (itest_test_pre(#TEST) == 1) {                                    \
            enum itest_test_res res = ITEST_SAVE_CONTEXT();                  \
            if (res == ITEST_TEST_RES_PASS) {                                \
                res = TEST();                                                \
            }                                                                \
            itest_test_post(res);                                            \
        }                                                                    \
    } while (0)

/* Ignore a test, don't warn about it being unused. */
#define ITEST_IGNORE_TEST(TEST) (void)TEST

/* Run a test in the current suite with one void * argument,
 * which can be a pointer to a struct with multiple arguments. */
#define ITEST_RUN_TEST1(TEST, ENV)                                           \
    do {                                                                     \
        if (itest_test_pre(#TEST) == 1) {                                    \
            enum itest_test_res res = ITEST_SAVE_CONTEXT();                  \
            if (res == ITEST_TEST_RES_PASS) {                                \
                res = TEST(ENV);                                             \
            }                                                                \
            itest_test_post(res);                                            \
        }                                                                    \
    } while (0)

#ifdef ITEST_VA_ARGS
#    define ITEST_RUN_TESTp(TEST, ...)                                       \
        do {                                                                 \
            if (itest_test_pre(#TEST) == 1) {                                \
                enum itest_test_res res = ITEST_SAVE_CONTEXT();              \
                if (res == ITEST_TEST_RES_PASS) {                            \
                    res = TEST(__VA_ARGS__);                                 \
                }                                                            \
                itest_test_post(res);                                        \
            }                                                                \
        } while (0)
#endif

/* Check if the test runner is in verbose mode. */
#define ITEST_IS_VERBOSE()    ((itest_info.verbosity) > 0)
#define ITEST_LIST_ONLY()     (itest_info.flags & ITEST_FLAG_LIST_ONLY)
#define ITEST_FIRST_FAIL()    (itest_info.flags & ITEST_FLAG_FIRST_FAIL)
#define ITEST_ABORT_ON_FAIL() (itest_info.flags & ITEST_FLAG_ABORT_ON_FAIL)
#define ITEST_FAILURE_ABORT()                                                \
    (ITEST_FIRST_FAIL()                                                      \
     && (itest_info.suite.failed > 0 || itest_info.failed > 0))

/* Message-less forms of tests defined below. */
#define ITEST_PASS()                  ITEST_PASSm(NULL)
#define ITEST_FAIL()                  ITEST_FAILm(NULL)
#define ITEST_SKIP()                  ITEST_SKIPm(NULL)
#define ITEST_ASSERT(COND)            ITEST_ASSERTm(#COND, COND)
#define ITEST_ASSERT_OR_LONGJMP(COND) ITEST_ASSERT_OR_LONGJMPm(#COND, COND)
#define ITEST_ASSERT_FALSE(COND)      ITEST_ASSERT_FALSEm(#COND, COND)
#define ITEST_ASSERT_EQ(EXP, GOT)     ITEST_ASSERT_EQm(#EXP " != " #GOT, EXP, GOT)
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
#define ITEST_ASSERTm(MSG, COND)                                             \
    do {                                                                     \
        itest_info.assertions++;                                             \
        if (!(COND)) {                                                       \
            ITEST_FAILm(MSG);                                                \
        }                                                                    \
    } while (0)

/* Fail if a condition is not true, longjmping out of test. */
#define ITEST_ASSERT_OR_LONGJMPm(MSG, COND)                                  \
    do {                                                                     \
        itest_info.assertions++;                                             \
        if (!(COND)) {                                                       \
            ITEST_FAIL_WITH_LONGJMPm(MSG);                                   \
        }                                                                    \
    } while (0)

/* Fail if a condition is not false, with message. */
#define ITEST_ASSERT_FALSEm(MSG, COND)                                       \
    do {                                                                     \
        itest_info.assertions++;                                             \
        if ((COND)) {                                                        \
            ITEST_FAILm(MSG);                                                \
        }                                                                    \
    } while (0)

/* Internal macro for relational assertions */
#define ITEST__REL(REL, MSG, EXP, GOT)                                       \
    do {                                                                     \
        itest_info.assertions++;                                             \
        if (!((EXP)REL(GOT))) {                                              \
            ITEST_FAILm(MSG);                                                \
        }                                                                    \
    } while (0)

/* Fail if EXP is not ==, !=, >, <, >=, or <= to GOT. */
#define ITEST_ASSERT_EQm(MSG, E, G)  ITEST__REL(==, MSG, E, G)
#define ITEST_ASSERT_NEQm(MSG, E, G) ITEST__REL(!=, MSG, E, G)
#define ITEST_ASSERT_GTm(MSG, E, G)  ITEST__REL(>, MSG, E, G)
#define ITEST_ASSERT_GTEm(MSG, E, G) ITEST__REL(>=, MSG, E, G)
#define ITEST_ASSERT_LTm(MSG, E, G)  ITEST__REL(<, MSG, E, G)
#define ITEST_ASSERT_LTEm(MSG, E, G) ITEST__REL(<=, MSG, E, G)

/* Fail if EXP != GOT (equality comparison by ==).
 * Warning: FMT, EXP, and GOT will be evaluated more
 * than once on failure. */
#define ITEST_ASSERT_EQ_FMTm(MSG, EXP, GOT, FMT)                             \
    do {                                                                     \
        itest_info.assertions++;                                             \
        if ((EXP) != (GOT)) {                                                \
            ITEST_FPRINTF(ITEST_STDOUT, "\nExpected: ");                     \
            ITEST_FPRINTF(ITEST_STDOUT, FMT, EXP);                           \
            ITEST_FPRINTF(ITEST_STDOUT, "\n     Got: ");                     \
            ITEST_FPRINTF(ITEST_STDOUT, FMT, GOT);                           \
            ITEST_FPRINTF(ITEST_STDOUT, "\n");                               \
            ITEST_FAILm(MSG);                                                \
        }                                                                    \
    } while (0)

/* Fail if EXP is not equal to GOT, printing enum IDs. */
#define ITEST_ASSERT_ENUM_EQm(MSG, EXP, GOT, ENUM_STR)                       \
    do {                                                                     \
        int itest_EXP                      = (int)(EXP);                     \
        int itest_GOT                      = (int)(GOT);                     \
        itest_enum_str_fun *itest_ENUM_STR = ENUM_STR;                       \
        if (itest_EXP != itest_GOT) {                                        \
            ITEST_FPRINTF(ITEST_STDOUT, "\nExpected: %s",                    \
                          itest_ENUM_STR(itest_EXP));                        \
            ITEST_FPRINTF(ITEST_STDOUT, "\n     Got: %s\n",                  \
                          itest_ENUM_STR(itest_GOT));                        \
            ITEST_FAILm(MSG);                                                \
        }                                                                    \
    } while (0)

/* Fail if GOT not in range of EXP +|- TOL. */
#define ITEST_ASSERT_IN_RANGEm(MSG, EXP, GOT, TOL)                           \
    do {                                                                     \
        ITEST_FLOAT itest_EXP = (EXP);                                       \
        ITEST_FLOAT itest_GOT = (GOT);                                       \
        ITEST_FLOAT itest_TOL = (TOL);                                       \
        itest_info.assertions++;                                             \
        if ((itest_EXP > itest_GOT && itest_EXP - itest_GOT > itest_TOL)     \
            || (itest_EXP < itest_GOT                                        \
                && itest_GOT - itest_EXP > itest_TOL)) {                     \
            ITEST_FPRINTF(ITEST_STDOUT,                                      \
                          "\nExpected: " ITEST_FLOAT_FMT                     \
                          " +/- " ITEST_FLOAT_FMT                            \
                          "\n     Got: " ITEST_FLOAT_FMT "\n",               \
                          itest_EXP, itest_TOL, itest_GOT);                  \
            ITEST_FAILm(MSG);                                                \
        }                                                                    \
    } while (0)

/* Fail if EXP is not equal to GOT, according to strcmp. */
#define ITEST_ASSERT_STR_EQm(MSG, EXP, GOT)                                  \
    do {                                                                     \
        ITEST_ASSERT_EQUAL_Tm(MSG, EXP, GOT, &itest_type_info_string, NULL); \
    } while (0)

/* Fail if EXP is not equal to GOT, according to strncmp. */
#define ITEST_ASSERT_STRN_EQm(MSG, EXP, GOT, SIZE)                           \
    do {                                                                     \
        size_t size = SIZE;                                                  \
        ITEST_ASSERT_EQUAL_Tm(MSG, EXP, GOT, &itest_type_info_string,        \
                              &size);                                        \
    } while (0)

/* Fail if EXP is not equal to GOT, according to memcmp. */
#define ITEST_ASSERT_MEM_EQm(MSG, EXP, GOT, SIZE)                            \
    do {                                                                     \
        itest_memory_cmp_env env;                                            \
        env.exp  = (const unsigned char *)EXP;                               \
        env.got  = (const unsigned char *)GOT;                               \
        env.size = SIZE;                                                     \
        ITEST_ASSERT_EQUAL_Tm(MSG, env.exp, env.got,                         \
                              &itest_type_info_memory, &env);                \
    } while (0)

/* Fail if EXP is not equal to GOT, according to a comparison
 * callback in TYPE_INFO. If they are not equal, optionally use a
 * print callback in TYPE_INFO to print them. */
#define ITEST_ASSERT_EQUAL_Tm(MSG, EXP, GOT, TYPE_INFO, UDATA)               \
    do {                                                                     \
        itest_type_info *type_info = (TYPE_INFO);                            \
        itest_info.assertions++;                                             \
        if (!itest_do_assert_equal_t(EXP, GOT, type_info, UDATA)) {          \
            if (type_info == NULL || type_info->equal == NULL) {             \
                ITEST_FAILm("type_info->equal callback missing!");           \
            } else {                                                         \
                ITEST_FAILm(MSG);                                            \
            }                                                                \
        }                                                                    \
    } while (0)

/* Pass. */
#define ITEST_PASSm(MSG)                                                     \
    do {                                                                     \
        itest_info.msg = MSG;                                                \
        return ITEST_TEST_RES_PASS;                                          \
    } while (0)

/* Fail. */
#define ITEST_FAILm(MSG)                                                     \
    do {                                                                     \
        itest_info.fail_file = __FILE__;                                     \
        itest_info.fail_line = __LINE__;                                     \
        itest_info.msg       = MSG;                                          \
        if (ITEST_ABORT_ON_FAIL()) {                                         \
            abort();                                                         \
        }                                                                    \
        return ITEST_TEST_RES_FAIL;                                          \
    } while (0)

/* Optional ITEST_FAILm variant that longjmps. */
#if ITEST_USE_LONGJMP
#    define ITEST_FAIL_WITH_LONGJMP() ITEST_FAIL_WITH_LONGJMPm(NULL)
#    define ITEST_FAIL_WITH_LONGJMPm(MSG)                                    \
        do {                                                                 \
            itest_info.fail_file = __FILE__;                                 \
            itest_info.fail_line = __LINE__;                                 \
            itest_info.msg       = MSG;                                      \
            longjmp(itest_info.jump_dest, ITEST_TEST_RES_FAIL);              \
        } while (0)
#endif

/* Skip the current test. */
#define ITEST_SKIPm(MSG)                                                     \
    do {                                                                     \
        itest_info.msg = MSG;                                                \
        return ITEST_TEST_RES_SKIP;                                          \
    } while (0)

/* Check the result of a subfunction using ASSERT, etc. */
#define ITEST_CHECK_CALL(RES)                                                \
    do {                                                                     \
        enum itest_test_res itest_RES = RES;                                 \
        if (itest_RES != ITEST_TEST_RES_PASS) {                              \
            return itest_RES;                                                \
        }                                                                    \
    } while (0)

#if ITEST_USE_TIME
#    define ITEST_SET_TIME(NAME)                                             \
        NAME = clock();                                                      \
        if (NAME == (clock_t)-1) {                                           \
            ITEST_FPRINTF(ITEST_STDOUT, "clock error: %s\n", #NAME);         \
            exit(EXIT_FAILURE);                                              \
        }

#    define ITEST_CLOCK_DIFF(C1, C2)                                         \
        ITEST_FPRINTF(ITEST_STDOUT, " (%lu ticks, %.3f sec)",                \
                      (long unsigned int)(C2) - (long unsigned int)(C1),     \
                      (double)((C2) - (C1))                                  \
                          / (1.0 * (double)CLOCKS_PER_SEC))
#else
#    define ITEST_SET_TIME(UNUSED)
#    define ITEST_CLOCK_DIFF(UNUSED1, UNUSED2)
#endif

#if ITEST_USE_LONGJMP
#    define ITEST_SAVE_CONTEXT()                                             \
        /* setjmp returns 0 (ITEST_TEST_RES_PASS) on first call *            \
         * so the test runs, then RES_FAIL from FAIL_WITH_LONGJMP. */        \
        ((enum itest_test_res)(setjmp(itest_info.jump_dest)))
#else
#    define ITEST_SAVE_CONTEXT()                                             \
        /*a no-op, since setjmp/longjmp aren't being used */                 \
        ITEST_TEST_RES_PASS
#endif

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
            BODY;                                                            \
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

/* Handle command-line arguments, etc. */
#define ITEST_MAIN_BEGIN()                                                   \
    do {                                                                     \
        ITEST_INIT();                                                        \
        itest_parse_options(argc, argv);                                     \
    } while (0)

/* Report results, exit with exit status based on results. */
#define ITEST_MAIN_END()                                                     \
    do {                                                                     \
        ITEST_PRINT_REPORT();                                                \
        return (itest_all_passed() ? EXIT_SUCCESS : EXIT_FAILURE);           \
    } while (0)

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif /* itest.h */
