import subprocess
from pathlib import Path

import pytest

# Seeds for allocator
SEEDS = ["1237", "31247823", "1"]
TRIES = 20
MIN_TRY_PASS_PERCENTAGE = 50


def run_bin(program, lib_path, seed=None):
    env = {"LD_PRELOAD": str(lib_path.resolve())} if lib_path else {}
    if seed:
        env["SEALLOC_SEED"] = seed
    return subprocess.run([str(program)], env=env)


def save_output(output_dir: Path, name: str, stdout: bytes, stderr: bytes, subproc):
    Path(output_dir / f"{name}_stdout").write_bytes(stdout)
    Path(output_dir / f"{name}_stderr").write_bytes(stderr)
    Path(output_dir / f"{name}_subproc").write_text(subproc.__str__())

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
        timeout=360,
        shell=True,
    )
    save_output(output_dir_e2e, prog, res.stdout, res.stderr, res)
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
    save_output(output_dir_e2e, "kissat", res.stdout, res.stderr, res)
    assert res.returncode == 10
