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

    Most of those opinions are still TBD, but changes we already plan
    to make are:

    - Runtime rather than compile time configuration.  In particular,
      whether tests are timed is a runtime switch, not an ifdef.
    - No global state; it should be possible to run tests from more
      than one thread.
    - As much code as possible should be out of line (in `itest.c`
      rather than `itest.h`).

- **Reasonably Portable**

    Infinitestimal requires an ISO C99 hosted environment; notably,
    the complete functionality of `string.h` and `stdio.h` is required.

    [not yet implemented] There is optional support for isolating
    tests from each other, and recovering from crashes, using POSIX
    functionality (`fork`, `waitpid`, etc).  Patches to do the same
    thing on Windows and/or to do a more thorough job using
    OS-specific features (e.g. cgroups) will be considered.

    Heap allocation is *avoided* but may not be 100% absent.  We’ll see.

[theft][], a related project, adds [property-based testing][pbt].

## Usage

For now, see [`README-greatest.md`][rg].  All `GREATEST_` and
`greatest_` prefixes have been changed to `ITEST_` and `itest_`
respectively, but otherwise the API is nearly the same.

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
