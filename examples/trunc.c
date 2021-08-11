/* Make the buffer size VERY small, to check truncation */
#define ITEST_TESTNAME_BUF_SIZE 8

#include "itest-abbrev.h"

TEST
t(void)
{
}

TEST
abcdefghijklmnopqrstuvwxyz(void)
{
}

SUITE(suite)
{
    size_t i;
    char buf[10];
    memset(buf, 0x00, sizeof(buf));
    for (i = 0; i < sizeof(buf); i++) {
        if (i > 0) {
            buf[i - 1] = 'x';
        }
        itest_set_test_suffix(buf);
        RUN_TEST(t);
    }

    RUN_TEST(abcdefghijklmnopqrstuvwxyz);
}

int
main(int argc, char **argv)
{
    itest_init();
    itest_parse_options(argc, argv);

    RUN_SUITE(suite);

    printf("sizeof(itest_info): %lu\n", (unsigned long)sizeof(itest_info));
    return itest_print_report();
}
