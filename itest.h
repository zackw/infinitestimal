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

#include <stddef.h>

#ifdef __cplusplus
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

typedef enum itest_flag_t
{
    ITEST_FLAG_FIRST_FAIL    = 0x01,
    ITEST_FLAG_LIST_ONLY     = 0x02,
    ITEST_FLAG_ABORT_ON_FAIL = 0x04
} itest_flag_t;

/* overall pass/fail/skip counts */
typedef struct itest_report_t
{
    unsigned int passed;
    unsigned int failed;
    unsigned int skipped;
    unsigned int assertions;
} itest_report_t;

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
void itest_assert_equal_str(const char *msg, const char *file,
                            unsigned int line, const char *exp,
                            const char *got);
void itest_assert_equal_strn(const char *msg, const char *file,
                             unsigned int line, const char *exp,
                             const char *got, size_t size);
void itest_assert_equal_mem(const char *msg, const char *file,
                            unsigned int line, const void *exp,
                            const void *got, size_t size);
void itest_assert_equal_t(const char *msg, const char *file,
                          unsigned int line, const void *exp, const void *got,
                          const itest_type_info *type_info, void *udata);

void itest_shuffle_init(unsigned int id, unsigned long seed);
void itest_shuffle_next(unsigned int id);
int itest_shuffle_running(unsigned int id);

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
int itest_is_filtered(void);
void itest_stop_at_first_fail(void);
void itest_abort_on_fail(void);
void itest_list_only(void);
void itest_get_report(itest_report_t *report);
unsigned int itest_get_verbosity(void);
void itest_set_verbosity(unsigned int verbosity);
void itest_set_flag(itest_flag_t flag);
int itest_get_flag(itest_flag_t flag);
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
    itest_assert_equal_str(MSG, __FILE__, __LINE__, EXP, GOT)

/* Fail if EXP is not equal to GOT, according to strncmp. */
#define ITEST_ASSERT_STRN_EQm(MSG, EXP, GOT, SIZE)                           \
    itest_assert_equal_strn(MSG, __FILE__, __LINE__, EXP, GOT, SIZE)

/* Fail if EXP is not equal to GOT, according to memcmp. */
#define ITEST_ASSERT_MEM_EQm(MSG, EXP, GOT, SIZE)                            \
    itest_assert_equal_mem(MSG, __FILE__, __LINE__, EXP, GOT, SIZE)

/* Fail if EXP is not equal to GOT, according to a comparison
 * callback in TYPE_INFO. If they are not equal, optionally use a
 * print callback in TYPE_INFO to print them. */
#define ITEST_ASSERT_EQUAL_Tm(MSG, EXP, GOT, TYPE_INFO, UDATA)               \
    itest_assert_equal_t(MSG, __FILE__, __LINE__, EXP, GOT, TYPE_INFO, UDATA)

/* Fail. */
#define ITEST_FAILm(MSG) itest_fail(MSG, __FILE__, __LINE__)

/* Skip the current test. */
#define ITEST_SKIPm(MSG) itest_skip(MSG, __FILE__, __LINE__)

/* Run every suite / test function run within a block in pseudo-random
 * order, seeded by SEED. (The top 3 bits of the seed are ignored.)
 *
 * These macros expand to for-loop heads. They should be called like:
 *     ITEST_SHUFFLE_TESTS (seed) {
 *         ITEST_RUN_TEST(some_test);
 *         ITEST_RUN_TEST(some_other_test);
 *         ITEST_RUN_TEST(yet_another_test);
 *     }
 *
 * The loop body will be executed many times.  Avoid putting code other
 * than calls to ITEST_RUN_TEST/SUITE inside.
 */
#define ITEST_SHUFFLE_SUITES(SD) ITEST_SHUFFLE(0, SD)
#define ITEST_SHUFFLE_TESTS(SD)  ITEST_SHUFFLE(1, SD)
#define ITEST_SHUFFLE(ID, SD)                                                \
    for (itest_shuffle_init(ID, SD); itest_shuffle_running(ID);              \
         itest_shuffle_next(ID))

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* itest.h */
