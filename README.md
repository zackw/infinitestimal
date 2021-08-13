# Infinitestimal

A scaffold for unit testing C programs, in one `.c` and two `.h` files.
Based on [greatest][] but with somewhat divergent design goals.

## Key Features and Project Goals

- **Modular**

    Tests can be run individually, or grouped into suites. Suites can
    share common setup, and can be in distinct compilation units.

- **Low Friction**

    Specific tests or suites can be run by name, for focused and rapid
    iteration during development. greatest adds very little startup
    latency.

- **Minimal Boilerplate**

    Tests and suites are specially marked functions.  Utility routines
    are available for making assertions and reporting success or failure.

    Each source file containing tests and/or suites needs to include
    either `itest.h` or `itest-abbrev.h`, depending on whether
    namespace cleanliness or concise notation is more important.

    Each *program* containing tests needs to be linked with (the
    object file compiled from) `itest.c`, and needs to include code to
    start the tests running.  A straightforward command line interface
    is provided as a utility routine (not as `main`).

    That’s it.

- **Somewhat Opinionated**

    This is the biggest point of divergence from greatest: we wanted a
    test scaffold with a few more opinions about the right way to run
    tests and present their output.

    At present, we have made only minor API changes (see below) and
    moved as much code out-of-line as possible (which only affects how
    you incorporate itest into your project.  More changes are
    planned, see [`TODO.md`](./TODO.md) for details.

- **Reasonably Portable**

    Infinitestimal requires an ISO C99 hosted environment.  A handful
    of features of ISO C2011, and common compiler extensions, are
    used, but only with fallbacks.  [Note: there is presently no
    automated testing that the fallbacks work.  If you encounter
    problems with older / rarer compilers, please file a bug report.]

    [not yet implemented] There is optional support for isolating
    tests from each other, and recovering from crashes, using POSIX
    functionality (`fork`, `waitpid`, etc).  Patches to do the same
    thing on Windows and/or to do a more thorough job using
    OS-specific features (e.g. cgroups) will be considered.

    Heap allocation is *avoided* but may not be 100% absent.  We’ll see.

[theft][], a related project, adds [property-based testing][pbt].

## Usage

For now, see [`README-greatest.md`][rg].   The API is *almost* the
same, but a few changes have been made:

 - All `GREATEST_` and `greatest_` prefixes have been changed to
   `ITEST_` and `itest_` respectively.

 - The names of the functions `itest_init`, `itest_set_setup_cb`,
   `itest_set_teardown_cb`, and `itest_print_report` are now lower
   case.

 - `ITEST_MAIN_BEGIN` and `ITEST_MAIN_END` have been removed.  The
   canonical test runner main() now begins with

        itest_init();
        itest_parse_options(argc, argv);

    and ends with

        return itest_print_report();

    This avoids hiding control flow, and makes it clearer what the
    difference is between a standalone test runner program and a test
    suite embedded in a larger program — namely, the call to
    `itest_parse_options`.

 - `PASS()` and `PASSm()` have been removed.  A test function
   indicates success by returning without having called `FAIL[m]()` or
   `SKIP[m]()` or failing an assertion.  Note that (as in greatest)
   `FAIL[m]()`, `SKIP[m]()`, and failing an assertion all cause the
   test function to return as well.

 - `CHECK_CALL` has been removed.  Subroutines of test functions can
   use `ASSERT*`, `FAIL`, etc. and can still be called normally.

 - The `_WITH_LONGJMP` macros have been removed.  _All_ ways of
   failing or skipping a test now involve an invocation of `longjmp`.
   It is no longer possible to configure the library to not use setjmp
   and longjmp.

 - The `ITEST_FPRINTF`, `ITEST_FLOAT`, and `ITEST_FLOAT_FMT` macros
   have been removed.  fprintf, double, and "%g" are used
   unconditionally.  A more flexible (per-callsite instead of
   per-build) way to vary the type of the EXP and GOT arguments to
   `ITEST_ASSERT_IN_RANGE` may be added in the future.

 - The `ITEST_SHUFFLE_TESTS` and `ITEST_SHUFFLE_SUITES` macros no
   longer take a block of code as an argument.  The block is instead
   written immediately after the macro call, as if it were the body of
   a loop, like this:

        SHUFFLE_TESTS (seed) {
            RUN_TEST(foo);
            RUN_TEST(bar);
            RUN_TEST(baz);
        }

   This avoids a variety of situations where the preprocessor
   would have misinterpreted the block as *multiple* arguments.

- itest.h now only includes stddef.h and stdio.h, which may break code
  that was relying on it to include other C library headers.

- The `itest_info` global, the types `itest_suite_info`, `itest_prng`,
  `itest_run_info` and `itest_test_res`, and the macros
  `ITEST_ABORT_ON_FAIL`, `ITEST_FIRST_FAIL`, `ITEST_IS_VERBOSE` and
  `ITEST_LIST_ONLY` are no longer part of the public API.  New
  functions `itest_get_flag` and `itest_is_filtered` have been added
  to compensate.

- The configuration macro `ITEST_STDOUT` has been removed.  You can now
  set the FILE to which test output will be printed at runtime, with
  the new function `itest_set_output`.

  Relatedly, `itest_printf_cb` has been renamed `itest_fprintf_cb` and
  now takes the FILE to print to as an additional argument.

  The “usage” message printed by `-h` is unconditionally printed to
  stderr.  (It used to be sent to ITEST_STDOUT.)

- The configuration macro `ITEST_USE_TIME` has been removed.  Whether
  to record CPU time for each test is now controllable at runtime,
  using the `ITEST_FLAG_RECORD_TIMING` flag and/or the `-T` switch to
  the command line runner.  Timing is on by default.

  Relatedly, there is a new API function `itest_clear_flag`.

- The remaining compile-time configuration macros have been moved to
  itest.c; you only need to override them when compiling itest.c (and
  they have no effect on any other file that includes itest.h)

A proper manual will be written Real Soon Now.

## Licensing

Infinitestimal is released under the [ISC License][ISC] to honor the
preferences of the author of greatest, who is still responsible for
the bulk of the code; however, permissive licensing is *not*
considered a design goal for infinitestimal.  Copylefted code *will*
be incorporated, and thus the overall license terms changed, if it
makes technical sense to do so.

[greatest]: https://github.com/silentbicycle/greatest
[theft]: https://github.com/silentbicycle/theft
[pbt]: https://spin.atomicobject.com/2014/09/17/property-based-testing-c/
[rg]: ./README-greatest.md
[ISC]: https://opensource.org/licenses/ISC
