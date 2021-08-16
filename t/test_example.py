#! /usr/bin/python3

import glob
import os
import re
import subprocess

import pytest

EXAMPLE_DIR = os.path.relpath(
    os.path.join(os.path.dirname(__file__), "..", "examples")
)

# Shim for removesuffix, which was added in Python 3.9.
if hasattr("", "removesuffix"):

    def removesuffix(s: str, suffix: str) -> str:
        return s.removesuffix(suffix)
else:

    def removesuffix(s: str, suffix: str) -> str:
        if suffix and s.endswith(suffix):
            return s[:-len(suffix)]
        else:
            return s


def filter_log(log: str) -> str:
    """Return an edited version of LOG, redacting elements that are expected
       to vary from run to run of a test.
    """
    rsub = re.sub
    return "\n".join(
        rsub(
            r"init_second_pass: a \d+, c \d+, state \d+",
            "init_second_pass: <<variable>>",
            rsub(
                r"\(\d+ ticks, \d+\.\d+ sec\)", "(nn ticks, n.nnn sec)",
                rsub(
                    r", seed \d+$", ", seed nnnnn",
                    rsub(r"(\.c(?:c|pp)?:)\d+", r"\1nn", line.rstrip())
                )
            )
        ) for line in log.splitlines()
    ) + "\n"


def run_suite(prog: str) -> str:
    result = subprocess.run(
        [prog],
        stdin=subprocess.DEVNULL,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        encoding="utf-8",
    )

    # raise CalledProcessError for fatal signals but not unsuccessful exit
    if result.returncode < 0:
        result.check_returncode()
    return filter_log(result.stdout) + "+ exit {}\n".format(result.returncode)


@pytest.mark.parametrize(
    "exp_file", glob.glob(os.path.join(EXAMPLE_DIR, "*.exp"))
)
def test_example(exp_file: str) -> None:
    with open(exp_file, "rt", encoding="utf-8") as fp:
        expected = fp.read()
    actual = run_suite(removesuffix(exp_file, ".exp"))

    assert actual == expected
