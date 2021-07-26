#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "itest-abbrev.h"

TEST
standalone_pass(void)
{
    PASS();
}

int
main(int argc, char **argv)
{
    struct itest_report_t report;
    (void)argc;
    (void)argv;

    /* Initialize itest, but don't build the CLI test runner code. */
    ITEST_INIT();

    RUN_TEST(standalone_pass);

    /* Print report, but do not exit. */
    printf("\nStandard report, as printed by itest:\n");
    ITEST_PRINT_REPORT();

    itest_get_report(&report);

    printf("\nCustom report:\n");
    printf("pass %u, fail %u, skip %u, assertions %u\n", report.passed,
           report.failed, report.skipped, report.assertions);

    if (report.failed > 0) {
        return 1;
    }

    return 0;
}
