import subprocess
import pytest
from pathlib import Path

# Seeds for allocator
SEEDS = ["1237", "31247823", "1"]
TRIES = 20
MIN_TRY_PASS_PERCENTAGE = 50


def run_bin(program, lib_path, seed=None):
    env = {"LD_PRELOAD": str(lib_path.resolve())}
    if seed:
        env["SEALLOC_SEED"] = seed
    return subprocess.run(
        [str(program)],
        env=env
    )


def save_output(output_dir: Path, name: str, stdout: bytes, stderr: bytes):
    Path(output_dir / f"{name}_stdout").write_bytes(stdout)
    Path(output_dir / f"{name}_stderr").write_bytes(stderr)


@pytest.mark.security
@pytest.mark.parametrize(
    "bin_name",
    [
        "e2e_randomized_allocations",
        "e2e_overflow_large_detection",
        "e2e_overflow_medium_detection",
        "e2e_overflow_small_detection",
        "e2e_overflow_simple_prevention",
        "e2e_arbitrary_free_near",
        "e2e_double_free_close",
        "e2e_double_free_far",
        "e2e_uaf_close",
        "e2e_heap_spray_prevention",
        "e2e_uaf_many_no_duplicates",
    ],
)
def test_run_binaries(bin_name, bin_dir, lib_path):
    bin = bin_dir / bin_name
    passed = 0
    for _ in range(TRIES):
        res = run_bin(bin, lib_path)
        if res.returncode != 0:
            passed += 1
    assert (passed * 100) / TRIES >= MIN_TRY_PASS_PERCENTAGE


@pytest.mark.real_programs
@pytest.mark.parametrize(
    "prog,args",
    [
        ("cfrac", ["714660644893037395618884729892003411"]),
    ],
)
def test_real_programs(prog, args, output_dir_e2e, progs_dir, lib_path):
    res = subprocess.run(
        [str(progs_dir / prog), *args],
        env={"SEALLOC_SEED": "1234", "LD_PRELOAD": str(lib_path.resolve())},
        capture_output=True,
        timeout=360,
        shell=True,
    )
    save_output(output_dir_e2e, prog, res.stdout, res.stderr)
    assert res.returncode == 0


@pytest.mark.real_programs
def test_kissat(output_dir_e2e, progs_dir, lib_path):
    inp = Path("./test/assets/cnf.out").read_bytes()
    res = subprocess.run(
        [str(progs_dir / "kissat")],
        env={"SEALLOC_SEED": "1234", "LD_PRELOAD": str(lib_path.resolve())},
        capture_output=True,
        timeout=120,
        shell=True,
        input=inp,
    )
    save_output(output_dir_e2e, "kissat", res.stdout, res.stderr)
    assert res.returncode == 10
