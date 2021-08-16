# Programs used (can override from the command line)
CC = cc
CXX = c++
PERL = perl

# Appropriate default compiler options for current generation GCC and Clang.
WARN_CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Wconversion -Werror
WARN_CFLAGS += -Wstrict-prototypes -Wmissing-prototypes -Wwrite-strings
WARN_CFLAGS += -Wno-unused-parameter

WARN_CXXFLAGS = -std=c++11 -Wall -Wextra -Wpedantic -Wconversion -Werror

# Optimization and debugging, independently overridable from the command line.
CFLAGS = -g -Og
CXXFLAGS = -g -Og

ALL_CFLAGS = $(CFLAGS) $(WARN_CFLAGS)
ALL_CXXFLAGS = $(CXXFLAGS) $(WARN_CXXFLAGS)

CPPFLAGS = -I.

PROGRAMS = \
	examples/basic \
	examples/basic_cplusplus \
	examples/minimal_template \
	examples/no_runner \
	examples/no_suite \
	examples/shuffle \
	examples/trunc

SOURCES = \
	itest.c \
	itest.h \
	itest-abbrev.h \
	examples/basic.c \
	examples/basic_cplusplus.cpp \
	examples/minimal_template.c \
	examples/no_runner.c \
	examples/no_suite.c \
	examples/shuffle.c \
	examples/suite.c \
	examples/trunc.c

all: $(PROGRAMS)

%.o: %.c
	$(CC) -c -o $@ $(ALL_CFLAGS) $(CPPFLAGS) $<

%.o: %.cpp
	$(CXX) -c -o $@ $(ALL_CXXFLAGS) $(CPPFLAGS) $<

%: %.o
	$(LINK) -o $@ $(ALL_CFLAGS) $(LDFLAGS) $^

# By default use the C compiler to link, but use the C++ compiler
# for the C++ examples.
LINK = $(CC)
examples/basic_cplusplus: LINK = $(CXX)

check-examples: all
	if command -V pytest-3; then		\
	  pytest-3;				\
	elif command -V pytest; then		\
	  pytest;				\
	fi

check-lint:
	if command -V clang-tidy; then		\
	  clang-tidy $(SOURCES) -- -I.;		\
	fi
	if command -V cppcheck; then		\
	  cppcheck --enable=all --inconclusive	\
	  --project=.config.cppcheck		\
	  $(SOURCES);				\
	fi

check: check-examples check-lint

clean:
	rm -f $(PROGRAMS) $(PROGRAMS:=.o) examples/suite.o itest.o \
		example-output.log example-output-filtered.log

.PHONY: all check check-examples check-lint clean

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
