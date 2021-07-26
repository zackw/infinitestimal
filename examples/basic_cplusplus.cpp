#include "itest-abbrev.h"

TEST
standalone_test(void)
{
    FAILm("(expected failure)");
}

int
main(int argc, char **argv)
{
    ITEST_MAIN_BEGIN();
    RUN_TEST(standalone_test);
    ITEST_MAIN_END(); /* display results */
    return 0;
}
