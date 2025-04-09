import subprocess
import pytest
from pathlib import Path

# Seeds for allocator
SEEDS = ["1237", "31247823", "1"]


def run_bin(program, lib_path, seed):
    return subprocess.run(
        [str(program)],
        env={"SEALLOC_SEED": seed, "LD_PRELOAD": str(lib_path.resolve())},
    )


def save_output(output_dir: Path, name: str, stdout: bytes, stderr: bytes):
    Path(output_dir / f"{name}_stdout").write_bytes(stdout)
    Path(output_dir / f"{name}_stderr").write_bytes(stderr)


@pytest.mark.security
@pytest.mark.parametrize("seed", SEEDS)
@pytest.mark.parametrize(
    "bin_name",
    [
        "e2e_randomized_allocations",
        "e2e_overflow_large",
        "e2e_overflow_medium",
        "e2e_overflow_small",
        "e2e_arbitrary_free_near",
        "e2e_double_free_close",
        "e2e_double_free_far",
        "e2e_uaf_close",
        "e2e_uaf_many_no_duplicates",
    ],
)
def test_run_binaries(seed, bin_name, bin_dir, lib_path):
    bin = bin_dir / bin_name
    res = run_bin(bin, lib_path, seed)
    assert res.returncode != 0


@pytest.mark.real_programs
@pytest.mark.parametrize(
    "prog,args",
    [
        ("cfrac", ["3707030275882252342412325295197136712092001"]),
    ],
)
def test_real_programs(prog, args, output_dir_e2e, progs_dir, lib_path):
    res = subprocess.run(
        [str(progs_dir / prog), *args],
        env={"SEALLOC_SEED": "1234", "LD_PRELOAD": str(lib_path.resolve())},
        capture_output=True,
        timeout=120,
        shell=True,
    )
    save_output(output_dir_e2e, prog, res.stdout, res.stderr)
    assert res.stderr == b""
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
    assert res.stderr == b""
    assert res.returncode == 10
