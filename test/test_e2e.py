import subprocess
import pytest
from pathlib import Path

# Seeds for allocator
SEEDS = ["123", "31247823", "1"]


def run_bin(program, lib_path, seed):
    return subprocess.run(
        [str(program)],
        env={"SEALLOC_SEED": seed, "LD_PRELOAD": str(lib_path.resolve())},
    )


@pytest.mark.parametrize("seed", SEEDS)
@pytest.mark.parametrize("bin_name", [
    "e2e_overflow_large",
    "e2e_overflow_medium",
    "e2e_overflow_small",
    "e2e_arbitrary_free_near",
    "e2e_double_free_close",
    "e2e_double_free_far",
    "e2e_uaf_close",
    "e2e_uaf_many_no_duplicates"
])
def test_df_close(seed, bin_name, request):
    bin_dir = Path(request.config.getoption("--bin-dir"))
    lib_path = Path(request.config.getoption("--lib-path"))
    bin = bin_dir / bin_name
    res = run_bin(bin, lib_path, seed)
    assert res.returncode != 0
