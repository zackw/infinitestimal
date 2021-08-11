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

#ifndef ITEST_ABBREV_H
#define ITEST_ABBREV_H

#include "itest.h"

/* Make abbreviations without the ITEST_ prefix for the
 * most commonly used symbols. */

#define TEST             ITEST_TEST
#define SUITE            ITEST_SUITE
#define SUITE_EXTERN     ITEST_SUITE_EXTERN
#define RUN_TEST         ITEST_RUN_TEST
#define RUN_TEST1        ITEST_RUN_TEST1
#define RUN_SUITE        ITEST_RUN_SUITE
#define IGNORE_TEST      ITEST_IGNORE_TEST
#define ASSERT           ITEST_ASSERT
#define ASSERTm          ITEST_ASSERTm
#define ASSERT_FALSE     ITEST_ASSERT_FALSE
#define ASSERT_EQ        ITEST_ASSERT_EQ
#define ASSERT_NEQ       ITEST_ASSERT_NEQ
#define ASSERT_GT        ITEST_ASSERT_GT
#define ASSERT_GTE       ITEST_ASSERT_GTE
#define ASSERT_LT        ITEST_ASSERT_LT
#define ASSERT_LTE       ITEST_ASSERT_LTE
#define ASSERT_EQ_FMT    ITEST_ASSERT_EQ_FMT
#define ASSERT_IN_RANGE  ITEST_ASSERT_IN_RANGE
#define ASSERT_EQUAL_T   ITEST_ASSERT_EQUAL_T
#define ASSERT_STR_EQ    ITEST_ASSERT_STR_EQ
#define ASSERT_STRN_EQ   ITEST_ASSERT_STRN_EQ
#define ASSERT_MEM_EQ    ITEST_ASSERT_MEM_EQ
#define ASSERT_ENUM_EQ   ITEST_ASSERT_ENUM_EQ
#define ASSERT_FALSEm    ITEST_ASSERT_FALSEm
#define ASSERT_EQm       ITEST_ASSERT_EQm
#define ASSERT_NEQm      ITEST_ASSERT_NEQm
#define ASSERT_GTm       ITEST_ASSERT_GTm
#define ASSERT_GTEm      ITEST_ASSERT_GTEm
#define ASSERT_LTm       ITEST_ASSERT_LTm
#define ASSERT_LTEm      ITEST_ASSERT_LTEm
#define ASSERT_EQ_FMTm   ITEST_ASSERT_EQ_FMTm
#define ASSERT_IN_RANGEm ITEST_ASSERT_IN_RANGEm
#define ASSERT_EQUAL_Tm  ITEST_ASSERT_EQUAL_Tm
#define ASSERT_STR_EQm   ITEST_ASSERT_STR_EQm
#define ASSERT_STRN_EQm  ITEST_ASSERT_STRN_EQm
#define ASSERT_MEM_EQm   ITEST_ASSERT_MEM_EQm
#define ASSERT_ENUM_EQm  ITEST_ASSERT_ENUM_EQm
#define FAIL             ITEST_FAIL
#define SKIP             ITEST_SKIP
#define FAILm            ITEST_FAILm
#define SKIPm            ITEST_SKIPm
#define SET_SETUP        itest_set_setup_cb
#define SET_TEARDOWN     itest_set_teardown_cb
#define SHUFFLE_TESTS    ITEST_SHUFFLE_TESTS
#define SHUFFLE_SUITES   ITEST_SHUFFLE_SUITES

#ifdef ITEST_VA_ARGS
#    define RUN_TESTp ITEST_RUN_TESTp
#endif

#endif /* itest-abbrev.h */
