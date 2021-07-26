# Appropriate default compiler options for current generation GCC and Clang.
WARN_CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Wconversion -Werror
WARN_CFLAGS += -Wstrict-prototypes -Wmissing-prototypes -Wwrite-strings

WARN_CXXFLAGS = -std=c++11 -Wall -Wextra -Wpedantic -Wconversion -Werror

# Optimization and debugging, independently overridable from the command line.
CFLAGS = -g -Og
CXXFLAGS = -g -Og

ALL_CFLAGS = $(CFLAGS) $(WARN_CFLAGS)
ALL_CXXFLAGS = $(CXXFLAGS) $(WARN_CXXFLAGS)

CPPFLAGS = -I. -DITEST_USE_LONGJMP=1

PROGRAMS = \
	examples/basic \
	examples/basic_cplusplus \
	examples/minimal_template \
	examples/no_runner \
	examples/no_suite \
	examples/shuffle \
	examples/trunc

all: $(PROGRAMS)

%.o: %.c
	$(CC) -c -o $@ $(CFLAGS) $(CPPFLAGS) $<

%.o: %.cpp
	$(CXX) -c -o $@ $(CXXFLAGS) $(CPPFLAGS) $<

%: %.o
	$(LINK) -o $@ $(CFLAGS) $(LDFLAGS) $^

# By default use the C compiler to link, but use the C++ compiler
# for the C++ examples.
LINK = $(CC)
examples/basic_cplusplus: LINK = $(CXX)

check: all
	@set -x; for p in $(PROGRAMS); do $$p; done; exit 0

clean:
	rm -f $(PROGRAMS) $(PROGRAMS:=.o) examples/suite.o itest.o

.PHONY: all check clean

# Program dependencies
examples/basic: examples/basic.o examples/suite.o itest.o
examples/basic_cplusplus: examples/basic_cplusplus.o itest.o
examples/minimal_template: examples/minimal_template.o itest.o
examples/no_runner: examples/no_runner.o itest.o
examples/no_suite: examples/no_suite.o itest.o
examples/shuffle: examples/shuffle.o itest.o
examples/trunc: examples/trunc.o itest.o

# Header dependencies
examples/basic.o: examples/basic.c itest.h itest-abbrev.h
examples/basic_cplusplus.o: examples/basic_cplusplus.cpp itest.h itest-abbrev.h
examples/minimal_template.o: examples/minimal_template.c itest.h itest-abbrev.h
examples/no_runner.o: examples/no_runner.c itest.h itest-abbrev.h
examples/no_suite.o: examples/no_suite.c itest.h itest-abbrev.h
examples/shuffle.o: examples/shuffle.c itest.h itest-abbrev.h
examples/suite.o: examples/suite.c itest.h itest-abbrev.h
examples/trunc.o: examples/trunc.c itest.h itest-abbrev.h
itest.o: itest.c itest.h
