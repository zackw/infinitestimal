# Planned changes to Infinitestimal

## Infrastructure

* The test suite is feeble.

  - If we had machine-parseable logs this would be much easier to fix.

## Big and/or API-breaking changes

* Process isolation for individual tests.  Use `fork` when available.
  Requires an actual configuration process, to detect availability of
  `fork` and of `unistd.h`.

  - Go POSIX-only and *require* fork?
  - Once this is implemented, tests will fail with exit() instead of
    longjmp().
  - Can trap fatal signals and map them to failure.
    - Have a way to declare that a test is expected to crash.
  - Create empty working directory for each test, remove afterward
    (preserve on failure)
  - Optionally impose resource limits, other types of isolation
    (cgroups, filesystem namespaces)

* Table-driven test suites.

  ```
  TEST(foo) { ... }
  TEST(bar) { ... }
  SUITE(suite) { foo, bar };
  int main(int argc, int argv)
  {
    itest_init();
    itest_parse_options(argc, argv);
    itest_run_suite(suite);
    return itest_print_report();
  }
  ```

  Shuffling, parallelism, TAP output become much simpler.

* The library has all its state in a global struct right now.  It
  would be cleaner to have this be an argument threaded into all the
  APIs instead.  Sketch:

  - Test functions are implicitly declared with an `ictx` argument.
  - Assertion macros expect this argument with its conventional name.
  - The awkward part is allocating the context object in the first
    place.  We don’t want to call malloc, nor do we want to expose the
    complete declaration of the struct.  Possible hack: itest_init
    takes a *callback* + a closure pointer. It calls the callback with
    a context argument allocated from its own stack frame.
  - I don’t like this hack because every command line test `main`
    would need to declare a closure struct just to pass down `argc`
    and `argv` in order to call `itest_parse_options`.

* Separate progress reports from detailed test logs.  Verbosity only
  affects what gets dumped to the terminal, not what gets logged.

* Reliably machine-parseable logs.

  - TAP format would make sense.
  - The reason the existing output _isn’t_ reliably machine parseable,
    is that random output from the tests themselves can get tangled up
    with the test harness’s logging.  Fixing this probably needs
    per-test process isolation (see above).

* Rename `ITEST_ASSERT_{ENUM,STR,STRN,MEM}_EQ` and
  `ITEST_ASSERT_EQUAL_T` to `ITEST_ASSERT_EQ_*` for consistency.

* Eliminate `ITEST_TESTNAME_BUF_SIZE`.

  - No replacement; the library should Just Work with arbitrarily long
    test names.
  - Copying the test name is only needed for suffixed tests.
  - The hard part here is not breaking the no-malloc guarantee.

* Eliminate the `m` macros.  Instead do preprocessor cleverness to
  make the message argument optional.

## Smaller, backward compatible changes

* Command line runner should be exit-code compatible with Automake’s
  test driver (special exit codes 77 and 99).

* Handle NaN correctly in `ITEST_ASSERT_IN_RANGE` and in `got`
  argument to relational assertions.

* Expand `ITEST_ASSERT_EQ_FMT` to all relational operators.

* Make `ITEST_ASSERT_EQ_FMT` evaluate its arguments only once each.

  This is straightforward but lengthy: the implementation function
  `itest_assert_eq_fmt` is already receiving both `exp` and `got` as
  variadic arguments.  It needs to do the comparison itself, instead
  of letting it happen in the caller (which is where the second
  evaluation comes from).  To do that it needs to parse the format
  string for the types of EXP and GOT, as printf does.  Simple matter
  of programming.

## Polish and bling

* Silent mode in the command line runner.

* Long options in the command line runner.

* Regex support in test filters.

* Parallelize tests.

  - Requires process isolation.
  - Interoperate with gmake jobserver.

* Eliminate `ITEST_DEFAULT_WIDTH`.

   - Add an API function to change the width.
   - Add a command line option to change the width.
   - Honor `$COLUMNS` if set in the environment.
   - If the out file is a tty, and `$TERM` says it’s possible, probe
     the terminal for its width.

* Colorize output directly, instead of with a contrib script.

  - Add command line options to control colorization.
  - Autodetect color terminals (as e.g. git, ls do)
