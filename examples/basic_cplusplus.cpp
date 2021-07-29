#include "itest-abbrev.h"

TEST
standalone_test(void)
{
    FAILm("(expected failure)");
}

int
main(int argc, char **argv)
{
    itest_init();
    itest_parse_options(argc, argv);

    RUN_TEST(standalone_test);
    return itest_print_report();
}
