#include <stdio.h>
#include <stdlib.h>

#include "itest-abbrev.h"

/* Declare a local suite. */
SUITE(other_suite);

TEST
blah(void)
{
}

TEST
todo(void)
{
    SKIPm("TODO");
}

ITEST_SUITE(other_suite)
{
    RUN_TEST(blah);
    RUN_TEST(todo);
}
