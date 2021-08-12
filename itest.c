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

#include "itest.h"

/***********
 * Options *
 ***********/

/* Default column width for non-verbose output. */
#ifndef ITEST_DEFAULT_WIDTH
#    define ITEST_DEFAULT_WIDTH 72
#endif

/* Set to 0 to disable all use of time.h / clock(). */
#ifndef ITEST_USE_TIME
#    define ITEST_USE_TIME 1
#endif

/* Size of buffer for test name + optional '_' separator and suffix */
#ifndef ITEST_TESTNAME_BUF_SIZE
#    define ITEST_TESTNAME_BUF_SIZE 128
#endif

/* System headers */

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef ITEST_USE_TIME
#    include <time.h>
#endif

/* Infinitestimal: out-of-line test harness code.  */

/*********
 * Types *
 *********/

typedef struct itest_memory_cmp_env
{
    const unsigned char *exp;
    const unsigned char *got;
    size_t size;
} itest_memory_cmp_env;

/* Internal state for a PRNG, used to shuffle test order. */
typedef struct itest_prng
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
} itest_prng;

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
    const char *fail_file;
    const char *msg;

    /* output to this file */
    FILE *out;

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

/* Global var for the current testing context.  */
static itest_run_info itest_info;

/* PRNG internal state assumes uint32_t values */
static_assert(sizeof(itest_info.prng[0].state) >= 4, "PRNG state too small");
static_assert(sizeof(itest_info.prng[0].a) >= 4, "PRNG state too small");
static_assert(sizeof(itest_info.prng[0].c) >= 4, "PRNG state too small");
static_assert(sizeof(itest_info.prng[0].m) >= 4, "PRNG state too small");

/* Functions */

/* Query a CPU time clock.  */
static clock_t
itest_get_cpu_time(void)
{
#if ITEST_USE_TIME
    clock_t res = clock();
    if (res == (clock_t)-1) {
        fprintf(itest_info.out, "clock: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    return res;
#else
    return (clock_t)-1;
#endif
}

/* Report an elapsed CPU time interval.  */
static void
itest_report_interval(clock_t begin, clock_t end)
{
#if ITEST_USE_TIME
    // This subtraction must be done in unsigned arithmetic lest it
    // produce nonsense when the timer wraps around.
    unsigned long delta = (unsigned long)end - (unsigned long)begin;
    fprintf(itest_info.out, " (%lu ticks, %.3f sec)", delta,
            ((double)delta) / CLOCKS_PER_SEC);
#endif
}

static int
itest_string_equal_cb(const void *exp, const void *got, void *udata)
{
    size_t *size = (size_t *)udata;
    return (size != NULL
                ? (0 == strncmp((const char *)exp, (const char *)got, *size))
                : (0 == strcmp((const char *)exp, (const char *)got)));
}

static int
itest_string_fprintf_cb(FILE *fp, const void *t, void *udata)
{
    (void)udata; /* note: does not check \0 termination. */
    return fprintf(fp, "%s", (const char *)t);
}

static const itest_type_info itest_type_info_string = {
    itest_string_equal_cb,
    itest_string_fprintf_cb,
};

static int
itest_memory_equal_cb(const void *exp, const void *got, void *udata)
{
    itest_memory_cmp_env *env = (itest_memory_cmp_env *)udata;
    return (0 == memcmp(exp, got, env->size));
}

/* Hexdump raw memory, with differences highlighted */
static int
itest_memory_fprintf_cb(FILE *fp, const void *t, void *udata)
{
    itest_memory_cmp_env *env = (itest_memory_cmp_env *)udata;
    const unsigned char *buf  = (const unsigned char *)t;
    unsigned char diff_mark;
    size_t i, line_i, line_len = 0;
    int len = 0; /* format hexdump with differences highlighted */
    for (i = 0; i < env->size; i += line_len) {
        diff_mark = ' ';
        line_len  = env->size - i;
        if (line_len > 16) {
            line_len = 16;
        }
        for (line_i = i; line_i < i + line_len; line_i++) {
            if (env->exp[line_i] != env->got[line_i]) {
                diff_mark = 'X';
            }
        }
        len += fprintf(fp, "\n%04x %c ", (unsigned int)i, diff_mark);
        for (line_i = i; line_i < i + line_len; line_i++) {
            int m = env->exp[line_i] == env->got[line_i]; /* match? */
            len += fprintf(fp, "%02x%c", buf[line_i], m ? ' ' : '<');
        }
        for (line_i = 0; line_i < 16 - line_len; line_i++) {
            len += fprintf(fp, "   ");
        }
        fprintf(fp, " ");
        for (line_i = i; line_i < i + line_len; line_i++) {
            unsigned char c = buf[line_i];
            len += fprintf(fp, "%c", isprint(c) ? c : '.');
        }
    }
    len += fprintf(fp, "\n");
    return len;
}

static const itest_type_info itest_type_info_memory = {
    itest_memory_equal_cb,
    itest_memory_fprintf_cb,
};

/* Is FILTER a subset of NAME? */
static int
itest_name_match(const char *name, const char *filter, int res_if_none)
{
    size_t offset     = 0;
    size_t filter_len = filter ? strlen(filter) : 0;
    if (filter_len == 0) {
        return res_if_none;
    } /* no filter */
    if (itest_info.exact_name_match && strlen(name) != filter_len) {
        return 0; /* ignore substring matches */
    }
    while (name[offset] != '\0') {
        if (name[offset] == filter[0]) {
            if (0 == strncmp(&name[offset], filter, filter_len)) {
                return 1;
            }
        }
        offset++;
    }

    return 0;
}

static void
itest_buffer_test_name(const char *name)
{
    struct itest_run_info *g = &itest_info;
    size_t len = strlen(name), size = sizeof(g->name_buf);
    memset(g->name_buf, 0x00, size);
    (void)strncat(g->name_buf, name, size - 1);
    if (g->name_suffix && (len + 1 < size)) {
        g->name_buf[len] = '_';
        strncat(&g->name_buf[len + 1], g->name_suffix, size - (len + 2));
    }
}

/* Run one test function, passing no arguments.  */
void
itest_run_test(itest_test_cb *test_cb, const char *test_name)
{
    if (itest_test_pre(test_name) == 1) {
        /* ITEST_TEST_RES_PASS is 0, so test_cb is called only on
           setjmp's first return */
        int res = setjmp(itest_info.jump_dest);
        if (res == ITEST_TEST_RES_PASS) {
            test_cb();
        }
        itest_test_post((enum itest_test_res)res);
    }
}

/* Run one test function, passing one `void *` argument.  */
void
itest_run_test_with_env(itest_test_env_cb *test_cb, const char *test_name,
                        void *env)
{
    if (itest_test_pre(test_name) == 1) {
        /* ITEST_TEST_RES_PASS is 0, so test_cb is called only on
           setjmp's first return */
        int res = setjmp(itest_info.jump_dest);
        if (res == ITEST_TEST_RES_PASS) {
            test_cb(env);
        }
        itest_test_post((enum itest_test_res)res);
    }
}

ITEST_NORETURN
itest_fail(const char *msg, const char *file, unsigned int line)
{
    itest_info.fail_file = file;
    itest_info.fail_line = line;
    itest_info.msg       = msg;
    if (itest_get_flag(ITEST_FLAG_ABORT_ON_FAIL)) {
        abort();
    }
    longjmp(itest_info.jump_dest, ITEST_TEST_RES_FAIL);
}

ITEST_NORETURN
itest_skip(const char *msg, const char *file, unsigned int line)
{
    itest_info.fail_file = file;
    itest_info.fail_line = line;
    itest_info.msg       = msg;
    longjmp(itest_info.jump_dest, ITEST_TEST_RES_SKIP);
}

/* Before running a test, check the name filtering and
 * test shuffling state, if applicable, and then call setup hooks. */
int
itest_test_pre(const char *name)
{
    struct itest_run_info *g = &itest_info;
    int match;
    itest_buffer_test_name(name);
    match = itest_name_match(g->name_buf, g->test_filter, 1)
            && !itest_name_match(g->name_buf, g->test_exclude, 0);
    if (itest_get_flag(ITEST_FLAG_LIST_ONLY)) { /* just listing test names */
        if (match) {
            fprintf(itest_info.out, "  %s\n", g->name_buf);
        }
        goto clear;
    }
    if (match
        && (!itest_get_flag(ITEST_FLAG_FIRST_FAIL) || g->suite.failed == 0)) {
        struct itest_prng *p = &g->prng[1];
        if (p->random_order) {
            p->count++;
            if (!p->initialized || ((p->count - 1) != p->state)) {
                goto clear; /* don't run this test yet */
            }
        }
        if (g->running_test) {
            fprintf(stderr, "Error: Test run inside another test.\n");
            return 0;
        }
        g->suite.pre_test = itest_get_cpu_time();
        if (g->setup) {
            g->setup(g->setup_udata);
        }
        p->count_run++;
        g->running_test = 1;
        return 1; /* test should be run */
    } else {
        goto clear; /* skipped */
    }
clear:
    g->name_suffix = NULL;
    return 0;
}

static void
itest_do_pass(void)
{
    struct itest_run_info *g = &itest_info;
    if (itest_get_verbosity()) {
        fprintf(itest_info.out, "PASS %s: %s", g->name_buf,
                g->msg ? g->msg : "");
    } else {
        fprintf(itest_info.out, ".");
    }
    g->suite.passed++;
}

static void
itest_do_fail(void)
{
    struct itest_run_info *g = &itest_info;
    if (itest_get_verbosity()) {
        fprintf(itest_info.out, "FAIL %s: %s (%s:%u)", g->name_buf,
                g->msg ? g->msg : "", g->fail_file, g->fail_line);
    } else {
        fprintf(itest_info.out, "F");
        g->col++; /* add linebreak if in line of '.'s */
        if (g->col != 0) {
            fprintf(itest_info.out, "\n");
            g->col = 0;
        }
        fprintf(itest_info.out, "FAIL %s: %s (%s:%u)\n", g->name_buf,
                g->msg ? g->msg : "", g->fail_file, g->fail_line);
    }
    g->suite.failed++;
}

static void
itest_do_skip(void)
{
    struct itest_run_info *g = &itest_info;
    if (itest_get_verbosity()) {
        fprintf(itest_info.out, "SKIP %s: %s", g->name_buf,
                g->msg ? g->msg : "");
    } else {
        fprintf(itest_info.out, "s");
    }
    g->suite.skipped++;
}

void
itest_test_post(int res)
{
    itest_info.suite.post_test = itest_get_cpu_time();
    if (itest_info.teardown) {
        void *udata = itest_info.teardown_udata;
        itest_info.teardown(udata);
    }

    itest_info.running_test = 0;
    switch (res) {
    case ITEST_TEST_RES_PASS:
        itest_do_pass();
        break;

    case ITEST_TEST_RES_SKIP:
        itest_do_skip();
        break;

    /* FIXME introduce a fail/error distinction.  */
    case ITEST_TEST_RES_FAIL:
    default:
        itest_do_fail();
        break;
    }

    itest_info.name_suffix = NULL;
    itest_info.suite.tests_run++;
    itest_info.col++;
    if (itest_get_verbosity()) {
        itest_report_interval(itest_info.suite.pre_test,
                              itest_info.suite.post_test);
        fprintf(itest_info.out, "\n");
    } else if (itest_info.col % itest_info.width == 0) {
        fprintf(itest_info.out, "\n");
        itest_info.col = 0;
    }
    fflush(itest_info.out);
}

static void
report_suite(void)
{
    if (itest_info.suite.tests_run > 0) {
        fprintf(itest_info.out,
                "\n%u test%s - %u passed, %u failed, %u skipped",
                itest_info.suite.tests_run,
                itest_info.suite.tests_run == 1 ? "" : "s",
                itest_info.suite.passed, itest_info.suite.failed,
                itest_info.suite.skipped);
        itest_report_interval(itest_info.suite.pre_suite,
                              itest_info.suite.post_suite);
        fprintf(itest_info.out, "\n");
    }
}

static void
update_counts_and_reset_suite(void)
{
    itest_info.setup          = NULL;
    itest_info.setup_udata    = NULL;
    itest_info.teardown       = NULL;
    itest_info.teardown_udata = NULL;
    itest_info.passed += itest_info.suite.passed;
    itest_info.failed += itest_info.suite.failed;
    itest_info.skipped += itest_info.suite.skipped;
    itest_info.tests_run += itest_info.suite.tests_run;
    memset(&itest_info.suite, 0, sizeof(itest_info.suite));
    itest_info.col = 0;
}

static int
itest_suite_pre(const char *suite_name)
{
    struct itest_prng *p = &itest_info.prng[0];
    if (!itest_name_match(suite_name, itest_info.suite_filter, 1)
        || (itest_get_flag(ITEST_FLAG_ABORT_ON_FAIL))) {
        return 0;
    }
    if (p->random_order) {
        p->count++;
        if (!p->initialized || ((p->count - 1) != p->state)) {
            return 0; /* don't run this suite yet */
        }
    }
    p->count_run++;
    update_counts_and_reset_suite();
    fprintf(itest_info.out, "\n* Suite %s:\n", suite_name);
    itest_info.suite.pre_suite = itest_get_cpu_time();
    return 1;
}

static void
itest_suite_post(void)
{
    itest_info.suite.post_suite = itest_get_cpu_time();
    report_suite();
}

void
itest_run_suite(itest_suite_cb *suite_cb, const char *suite_name)
{
    if (itest_suite_pre(suite_name)) {
        suite_cb();
        itest_suite_post();
    }
}

void
itest_assert(const char *msg, const char *file, unsigned int line, int cond)
{
    itest_info.assertions++;
    if (!cond) {
        itest_fail(msg, file, line);
    }
}

void
itest_assert_eq_fmt(const char *msg, const char *file, unsigned int line,
                    const char *fmt, int cond, ...)
{
    itest_info.assertions++;
    if (!cond) {
        va_list ap;
        va_start(ap, cond);
        vfprintf(itest_info.out, fmt, ap);
        va_end(ap);
        itest_fail(msg, file, line);
    }
}

void
itest_assert_eq_enum(const char *msg, const char *file, unsigned int line,
                     itest_enum_str_fun enum_str, int exp, int got)
{
    itest_info.assertions++;
    if (exp != got) {
        fprintf(itest_info.out, "\nExpected: %s", enum_str(exp));
        fprintf(itest_info.out, "\n     Got: %s\n", enum_str(got));
        itest_fail(msg, file, line);
    }
}

void
itest_assert_in_range(const char *msg, const char *file, unsigned int line,
                      double exp, double got, double tol)
{
    itest_info.assertions++;
    if ((exp > got && exp - got > tol) || (exp < got && got - exp > tol)) {
        fprintf(itest_info.out,
                "\nExpected: %g +/- %g"
                "\n     Got: %g\n",
                exp, tol, got);
        itest_fail(msg, file, line);
    }
}

void
itest_assert_equal_t(const char *msg, const char *file, unsigned int line,
                     const void *exp, const void *got,
                     const itest_type_info *type_info, void *udata)
{
    itest_info.assertions++;
    if (type_info == NULL || type_info->equal == NULL) {
        itest_fail("type_info->equal callback missing!", file, line);
    }
    if (!type_info->equal(exp, got, udata)) {
        if (type_info->print != NULL) {
            FILE *out = itest_info.out;
            fputs("\nExpected: ", out);
            (void)type_info->print(out, exp, udata);
            fputs("\n     Got: ", out);
            (void)type_info->print(out, got, udata);
            fputc('\n', out);
        }
        itest_fail(msg, file, line);
    }
}

void
itest_assert_equal_str(const char *msg, const char *file, unsigned int line,
                       const char *exp, const char *got)
{
    itest_assert_equal_t(msg, file, line, exp, got, &itest_type_info_string,
                         NULL);
}

void
itest_assert_equal_strn(const char *msg, const char *file, unsigned int line,
                        const char *exp, const char *got, size_t size)
{
    itest_assert_equal_t(msg, file, line, exp, got, &itest_type_info_string,
                         &size);
}

void
itest_assert_equal_mem(const char *msg, const char *file, unsigned int line,
                       const void *exp, const void *got, size_t size)
{
    itest_memory_cmp_env env;
    env.exp  = exp;
    env.got  = got;
    env.size = size;
    itest_assert_equal_t(msg, file, line, exp, got, &itest_type_info_memory,
                         &env);
}

static void
itest_usage(const char *name)
{
    fprintf(itest_info.out,
            "Usage: %s [-hlfavex] [-s SUITE] [-t TEST] [-x EXCLUDE]\n"
            "  -h, --help  print this Help\n"
            "  -l          List suites and tests, then exit (dry run)\n"
            "  -f          Stop runner after first failure\n"
            "  -a          Abort on first failure (implies -f)\n"
            "  -v          Verbose output\n"
            "  -s SUITE    only run suites containing substring SUITE\n"
            "  -t TEST     only run tests containing substring TEST\n"
            "  -e          only run exact name match for -s or -t\n"
            "  -x EXCLUDE  exclude tests containing substring EXCLUDE\n",
            name);
}

void
itest_parse_options(int argc, char **argv)
{
    int i = 0;
    for (i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            char f = argv[i][1];
            if ((f == 's' || f == 't' || f == 'x') && argc <= i + 1) {
                itest_usage(argv[0]);
                exit(EXIT_FAILURE);
            }
            switch (f) {
            case 's': /* suite name filter */
                itest_set_suite_filter(argv[i + 1]);
                i++;
                break;
            case 't': /* test name filter */
                itest_set_test_filter(argv[i + 1]);
                i++;
                break;
            case 'x': /* test name exclusion */
                itest_set_test_exclude(argv[i + 1]);
                i++;
                break;
            case 'e': /* exact name match */
                itest_set_exact_name_match();
                break;
            case 'f': /* first fail flag */
                itest_stop_at_first_fail();
                break;
            case 'a': /* abort() on fail flag */
                itest_abort_on_fail();
                break;
            case 'l': /* list only (dry run) */
                itest_list_only();
                break;
            case 'v': /* first fail flag */
                itest_info.verbosity++;
                break;
            case 'h': /* help */
                itest_usage(argv[0]);
                exit(EXIT_SUCCESS);
            default:
            case '-':
                if (0 == strncmp("--help", argv[i], 6)) {
                    itest_usage(argv[0]);
                    exit(EXIT_SUCCESS);
                } else if (0 == strcmp("--", argv[i])) {
                    return; /* ignore following arguments */
                }
                fprintf(itest_info.out, "Unknown argument '%s'\n", argv[i]);
                itest_usage(argv[0]);
                exit(EXIT_FAILURE);
            }
        }
    }
}

int
itest_all_passed(void)
{
    return (itest_info.failed == 0);
}

void
itest_set_test_filter(const char *filter)
{
    itest_info.test_filter = filter;
}

void
itest_set_test_exclude(const char *filter)
{
    itest_info.test_exclude = filter;
}

void
itest_set_suite_filter(const char *filter)
{
    itest_info.suite_filter = filter;
}

void
itest_set_exact_name_match(void)
{
    itest_info.exact_name_match = 1;
}

int
itest_is_filtered(void)
{
    return itest_info.test_filter != NULL || itest_info.test_exclude != NULL
           || itest_info.suite_filter != NULL;
}

void
itest_stop_at_first_fail(void)
{
    itest_set_flag(ITEST_FLAG_FIRST_FAIL);
}

void
itest_abort_on_fail(void)
{
    itest_set_flag(ITEST_FLAG_ABORT_ON_FAIL);
}

void
itest_list_only(void)
{
    itest_set_flag(ITEST_FLAG_LIST_ONLY);
}

void
itest_get_report(struct itest_report_t *report)
{
    if (report) {
        report->passed     = itest_info.passed;
        report->failed     = itest_info.failed;
        report->skipped    = itest_info.skipped;
        report->assertions = itest_info.assertions;
    }
}

unsigned int
itest_get_verbosity(void)
{
    return itest_info.verbosity;
}

void
itest_set_verbosity(unsigned int verbosity)
{
    itest_info.verbosity = (unsigned char)verbosity;
}

int
itest_get_flag(itest_flag_t flag)
{
    return !!(itest_info.flags & flag);
}

void
itest_set_flag(itest_flag_t flag)
{
    itest_info.flags = (unsigned char)(itest_info.flags | flag);
}

void
itest_set_test_suffix(const char *suffix)
{
    itest_info.name_suffix = suffix;
}

void
itest_set_setup_cb(itest_setup_cb *cb, void *udata)
{
    itest_info.setup       = cb;
    itest_info.setup_udata = udata;
}

void
itest_set_teardown_cb(itest_teardown_cb *cb, void *udata)
{
    itest_info.teardown       = cb;
    itest_info.teardown_udata = udata;
}

void
itest_set_output(FILE *fp)
{
    itest_info.out = fp;
}

/* Test shuffling uses a linear congruential pseudorandom number
 * generator, with the power-of-two ceiling of the test count as the
 * modulus, the masked seed as the multiplier, and a prime as the
 * increment. For each generated value < the test count, we run the
 * corresponding test.  This is guaranteed to visit all IDs 0 <= X < mod
 * once before repeating, with a starting position chosen based on
 * the initial seed.  For details, see: Knuth, The Art of Computer
 * Programming Volume. 2, section 3.2.1.
 */
void
itest_shuffle_init(unsigned int id, unsigned long seed)
{
    struct itest_prng *p = &itest_info.prng[id];
    p->random_order      = 1;
    p->count             = 0;
    p->count_run         = 0;
    p->initialized       = 0; /* first pass: count the tests/suites */
    p->state             = seed & 0x1fffffff;     /* only use lower 29 bits */
    p->a                 = 4LU * p->state;        /* to avoid overflow when */
    p->a                 = (p->a ? p->a : 4) | 1; /* multiplied by 4 */
    p->c                 = 2147483647; /* and so p->c ((2 ** 31) - 1) is
                                          always relatively prime to p->a. */
}

void
itest_shuffle_next(unsigned int id)
{
    struct itest_prng *p = &itest_info.prng[id];
    if (p->initialized) {
        /* Step the pseudorandom number generator until its state reaches
           another test ID between 0 and the test count. */
        do {
            p->state = ((p->a * p->state) + p->c) & (p->m - 1);
        } while (p->state >= p->count_ceil);

    } else {
        /* done counting tests, finish initialization */
        p->initialized = 1;
        p->count_ceil  = p->count;
        if (p->count == 0) {
            return;
        }
        p->m = 1;
        while (p->m < p->count) {
            p->m <<= 1;
        }
        fprintf(stderr, "init_second_pass: a %lu, c %lu, state %lu\n", p->a,
                p->c, p->state);
    }
    p->count = 0;
}

/* Return true if shuffle ID is still running.  */
int
itest_shuffle_running(unsigned int id)
{
    struct itest_prng *p = &itest_info.prng[id];
    if ((!p->initialized || p->count_run < p->count_ceil)
        && !(itest_get_flag(ITEST_FLAG_FIRST_FAIL)
             && (itest_info.suite.failed > 0 || itest_info.failed > 0))) {
        return 1;
    }
    memset(p, 0, sizeof *p);
    return 0;
}

void
itest_init(void)
{
    memset(&itest_info, 0, sizeof(itest_info));
    itest_info.width = ITEST_DEFAULT_WIDTH;
    itest_info.begin = itest_get_cpu_time();
    itest_info.out   = stdout;
}

/* Report passes, failures, skipped tests, the number of
 * assertions, and the overall run time.  As a convenience,
 * returns EXIT_SUCCESS if all tests passed, EXIT_FAILURE
 * otherwise, so main can end with 'return itest_print_report();'
 */
int
itest_print_report(void)
{
    if (itest_get_flag(ITEST_FLAG_LIST_ONLY)) {
        return EXIT_SUCCESS;
    }

    update_counts_and_reset_suite();
    itest_info.end = itest_get_cpu_time();
    fprintf(itest_info.out, "\nTotal: %u test%s", itest_info.tests_run,
            itest_info.tests_run == 1 ? "" : "s");
    itest_report_interval(itest_info.begin, itest_info.end);
    fprintf(itest_info.out, ", %u assertion%s\n", itest_info.assertions,
            itest_info.assertions == 1 ? "" : "s");
    fprintf(itest_info.out, "Pass: %u, fail: %u, skip: %u.\n",
            itest_info.passed, itest_info.failed, itest_info.skipped);

    return itest_all_passed() ? EXIT_SUCCESS : EXIT_FAILURE;
}
