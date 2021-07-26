#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "itest-abbrev.h"

TEST
standalone_fail(void)
{
    FAILm("(expected failure)");
}

TEST
standalone_pass(void)
{
    PASS();
}

TEST
standalone_skip(void)
{
    SKIPm("skipped");
}

int
main(int argc, char **argv)
{
    ITEST_MAIN_BEGIN(); /* command-line arguments, initialization. */
    /* If tests are run outside of a suite, a default suite is used. */
    RUN_TEST(standalone_fail);
    RUN_TEST(standalone_pass);
    RUN_TEST(standalone_skip);

    ITEST_MAIN_END(); /* display results */
}
