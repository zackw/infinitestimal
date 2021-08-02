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

#include <errno.h>

/* Infinitestimal: out-of-line test harness code.  */

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

/* Query a CPU time clock.  */
static clock_t
itest_get_cpu_time(void)
{
#if ITEST_USE_TIME
    clock_t res = clock();
    if (res == (clock_t)-1) {
        ITEST_FPRINTF(ITEST_STDOUT, "clock: %s\n", strerror(errno));
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
    ITEST_FPRINTF(ITEST_STDOUT, " (%lu ticks, %.3f sec)", delta,
                  ((double)delta) / CLOCKS_PER_SEC);
#endif
}

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
        enum itest_test_res res = ITEST_SAVE_CONTEXT();
        if (res == ITEST_TEST_RES_PASS) {
            res = test_cb();
        }
        itest_test_post(res);
    }
}

/* Run one test function, passing one `void *` argument.  */
void
itest_run_test_with_env(itest_test_env_cb *test_cb, const char *test_name,
                        void *env)
{
    if (itest_test_pre(test_name) == 1) {
        enum itest_test_res res = ITEST_SAVE_CONTEXT();
        if (res == ITEST_TEST_RES_PASS) {
            res = test_cb(env);
        }
        itest_test_post(res);
    }
}

enum itest_test_res
itest_set_test_status(const char *msg, const char *file, unsigned int line,
                      enum itest_test_res res)
{
    itest_info.fail_file = file;
    itest_info.fail_line = line;
    itest_info.msg       = msg;
    if (res == ITEST_TEST_RES_FAIL && ITEST_ABORT_ON_FAIL()) {
        abort();
    }
    return res;
}

#if ITEST_USE_LONGJMP
void /* noreturn */
itest_fail_with_longjmp(const char *msg, const char *file, unsigned int line)
{
    itest_set_test_status(msg, file, line, ITEST_TEST_RES_FAIL);
    longjmp(itest_info.jump_dest, ITEST_TEST_RES_FAIL);
}
#endif

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
    if (ITEST_LIST_ONLY()) { /* just listing test names */
        if (match) {
            ITEST_FPRINTF(ITEST_STDOUT, "  %s\n", g->name_buf);
        }
        goto clear;
    }
    if (match && (!ITEST_FIRST_FAIL() || g->suite.failed == 0)) {
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
    if (ITEST_IS_VERBOSE()) {
        ITEST_FPRINTF(ITEST_STDOUT, "PASS %s: %s", g->name_buf,
                      g->msg ? g->msg : "");
    } else {
        ITEST_FPRINTF(ITEST_STDOUT, ".");
    }
    g->suite.passed++;
}

static void
itest_do_fail(void)
{
    struct itest_run_info *g = &itest_info;
    if (ITEST_IS_VERBOSE()) {
        ITEST_FPRINTF(ITEST_STDOUT, "FAIL %s: %s (%s:%u)", g->name_buf,
                      g->msg ? g->msg : "", g->fail_file, g->fail_line);
    } else {
        ITEST_FPRINTF(ITEST_STDOUT, "F");
        g->col++; /* add linebreak if in line of '.'s */
        if (g->col != 0) {
            ITEST_FPRINTF(ITEST_STDOUT, "\n");
            g->col = 0;
        }
        ITEST_FPRINTF(ITEST_STDOUT, "FAIL %s: %s (%s:%u)\n", g->name_buf,
                      g->msg ? g->msg : "", g->fail_file, g->fail_line);
    }
    g->suite.failed++;
}

static void
itest_do_skip(void)
{
    struct itest_run_info *g = &itest_info;
    if (ITEST_IS_VERBOSE()) {
        ITEST_FPRINTF(ITEST_STDOUT, "SKIP %s: %s", g->name_buf,
                      g->msg ? g->msg : "");
    } else {
        ITEST_FPRINTF(ITEST_STDOUT, "s");
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
    if (res <= ITEST_TEST_RES_FAIL) {
        itest_do_fail();
    } else if (res >= ITEST_TEST_RES_SKIP) {
        itest_do_skip();
    } else if (res == ITEST_TEST_RES_PASS) {
        itest_do_pass();
    }
    itest_info.name_suffix = NULL;
    itest_info.suite.tests_run++;
    itest_info.col++;
    if (ITEST_IS_VERBOSE()) {
        itest_report_interval(itest_info.suite.pre_test,
                              itest_info.suite.post_test);
        ITEST_FPRINTF(ITEST_STDOUT, "\n");
    } else if (itest_info.col % itest_info.width == 0) {
        ITEST_FPRINTF(ITEST_STDOUT, "\n");
        itest_info.col = 0;
    }
    fflush(ITEST_STDOUT);
}

static void
report_suite(void)
{
    if (itest_info.suite.tests_run > 0) {
        ITEST_FPRINTF(ITEST_STDOUT,
                      "\n%u test%s - %u passed, %u failed, %u skipped",
                      itest_info.suite.tests_run,
                      itest_info.suite.tests_run == 1 ? "" : "s",
                      itest_info.suite.passed, itest_info.suite.failed,
                      itest_info.suite.skipped);
        itest_report_interval(itest_info.suite.pre_suite,
                              itest_info.suite.post_suite);
        ITEST_FPRINTF(ITEST_STDOUT, "\n");
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
        || (ITEST_FAILURE_ABORT())) {
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
    ITEST_FPRINTF(ITEST_STDOUT, "\n* Suite %s:\n", suite_name);
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

int
itest_do_assert_equal_t(const void *expd, const void *got,
                        itest_type_info *type_info, void *udata)
{
    int eq = 0;
    if (type_info == NULL || type_info->equal == NULL) {
        return 0;
    }
    eq = type_info->equal(expd, got, udata);
    if (!eq) {
        if (type_info->print != NULL) {
            ITEST_FPRINTF(ITEST_STDOUT, "\nExpected: ");
            (void)type_info->print(expd, udata);
            ITEST_FPRINTF(ITEST_STDOUT, "\n     Got: ");
            (void)type_info->print(got, udata);
            ITEST_FPRINTF(ITEST_STDOUT, "\n");
        }
    }
    return eq;
}

static void
itest_usage(const char *name)
{
    ITEST_FPRINTF(
        ITEST_STDOUT,
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
                ITEST_FPRINTF(ITEST_STDOUT, "Unknown argument '%s'\n",
                              argv[i]);
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

static int
itest_string_equal_cb(const void *expd, const void *got, void *udata)
{
    size_t *size = (size_t *)udata;
    return (size != NULL
                ? (0 == strncmp((const char *)expd, (const char *)got, *size))
                : (0 == strcmp((const char *)expd, (const char *)got)));
}

static int
itest_string_printf_cb(const void *t, void *udata)
{
    (void)udata; /* note: does not check \0 termination. */
    return ITEST_FPRINTF(ITEST_STDOUT, "%s", (const char *)t);
}

itest_type_info itest_type_info_string = {
    itest_string_equal_cb,
    itest_string_printf_cb,
};

static int
itest_memory_equal_cb(const void *expd, const void *got, void *udata)
{
    itest_memory_cmp_env *env = (itest_memory_cmp_env *)udata;
    return (0 == memcmp(expd, got, env->size));
}

/* Hexdump raw memory, with differences highlighted */
static int
itest_memory_printf_cb(const void *t, void *udata)
{
    itest_memory_cmp_env *env = (itest_memory_cmp_env *)udata;
    const unsigned char *buf  = (const unsigned char *)t;
    FILE *out                 = ITEST_STDOUT;
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
        len += ITEST_FPRINTF(out, "\n%04x %c ", (unsigned int)i, diff_mark);
        for (line_i = i; line_i < i + line_len; line_i++) {
            int m = env->exp[line_i] == env->got[line_i]; /* match? */
            len += ITEST_FPRINTF(out, "%02x%c", buf[line_i], m ? ' ' : '<');
        }
        for (line_i = 0; line_i < 16 - line_len; line_i++) {
            len += ITEST_FPRINTF(out, "   ");
        }
        ITEST_FPRINTF(out, " ");
        for (line_i = i; line_i < i + line_len; line_i++) {
            unsigned char c = buf[line_i];
            len += ITEST_FPRINTF(out, "%c", isprint(c) ? c : '.');
        }
    }
    len += ITEST_FPRINTF(out, "\n");
    return len;
}

void
itest_prng_init_first_pass(int id)
{
    itest_info.prng[id].random_order = 1;
    itest_info.prng[id].count_run    = 0;
}

int
itest_prng_init_second_pass(int id, unsigned long seed)
{
    struct itest_prng *p = &itest_info.prng[id];
    if (p->count == 0) {
        return 0;
    }
    p->count_ceil = p->count;
    for (p->m = 1; p->m < p->count; p->m <<= 1) {
    }
    p->state       = seed & 0x1fffffff;     /* only use lower 29 bits */
    p->a           = 4LU * p->state;        /* to avoid overflow when */
    p->a           = (p->a ? p->a : 4) | 1; /* multiplied by 4 */
    p->c           = 2147483647; /* and so p->c ((2 ** 31) - 1) is */
    p->initialized = 1;          /* always relatively prime to p->a. */
    fprintf(stderr, "init_second_pass: a %lu, c %lu, state %lu\n", p->a, p->c,
            p->state);
    return 1;
}

/* Step the pseudorandom number generator until its state reaches
 * another test ID between 0 and the test count.
 * This use a linear congruential pseudorandom number generator,
 * with the power-of-two ceiling of the test count as the modulus, the
 * masked seed as the multiplier, and a prime as the increment. For
 * each generated value < the test count, run the corresponding test.
 * This will visit all IDs 0 <= X < mod once before repeating,
 * with a starting position chosen based on the initial seed.
 * For details, see: Knuth, The Art of Computer Programming
 * Volume. 2, section 3.2.1. */
void
itest_prng_step(int id)
{
    struct itest_prng *p = &itest_info.prng[id];
    do {
        p->state = ((p->a * p->state) + p->c) & (p->m - 1);
    } while (p->state >= p->count_ceil);
}

void
itest_init(void)
{
    memset(&itest_info, 0, sizeof(itest_info));
    itest_info.width = ITEST_DEFAULT_WIDTH;
    itest_info.begin = itest_get_cpu_time();
}

/* Report passes, failures, skipped tests, the number of
 * assertions, and the overall run time.  As a convenience,
 * returns EXIT_SUCCESS if all tests passed, EXIT_FAILURE
 * otherwise, so main can end with 'return itest_print_report();'
 */
int
itest_print_report(void)
{
    if (ITEST_LIST_ONLY()) {
        return EXIT_SUCCESS;
    }

    update_counts_and_reset_suite();
    itest_info.end = itest_get_cpu_time();
    ITEST_FPRINTF(ITEST_STDOUT, "\nTotal: %u test%s", itest_info.tests_run,
                  itest_info.tests_run == 1 ? "" : "s");
    itest_report_interval(itest_info.begin, itest_info.end);
    ITEST_FPRINTF(ITEST_STDOUT, ", %u assertion%s\n", itest_info.assertions,
                  itest_info.assertions == 1 ? "" : "s");
    ITEST_FPRINTF(ITEST_STDOUT, "Pass: %u, fail: %u, skip: %u.\n",
                  itest_info.passed, itest_info.failed, itest_info.skipped);

    return itest_all_passed() ? EXIT_SUCCESS : EXIT_FAILURE;
}

itest_type_info itest_type_info_memory = {
    itest_memory_equal_cb,
    itest_memory_printf_cb,
};

itest_run_info itest_info;
