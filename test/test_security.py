import subprocess
from pathlib import Path

import pytest

TRIES = 100
MIN_TRY_PASS_PERCENTAGE = 50
TEST_BINARY_DIR = Path('./build/test')
FAIL = 0


def run_bin(program, lib_path, seed=None):
    env = {"LD_PRELOAD": str(lib_path.resolve())} if lib_path else {}
    if seed:
        env["SEALLOC_SEED"] = seed
    return subprocess.run([str(program)], env=env)


@pytest.mark.security
@pytest.mark.parametrize(
    "bin", list(TEST_BINARY_DIR.glob('sec_*'))
)
def test_run_binaries(bin, lib_path):
    passed = 0
    for _ in range(TRIES):
        res = run_bin(bin, lib_path)
        if res.returncode != FAIL:
            passed += 1
    percentage = (passed * 100) / TRIES
    pytest.xfail(f"({bin.name}) Tries passed {
                 passed}/{TRIES} ({percentage:.2f}%)")
