import subprocess
import pytest
from pathlib import Path
import shutil


def prepare_env(dir: Path, prog: Path, library: Path):
    prog_name = prog.name
    patched_prog = dir / prog_name
    shutil.copyfile(prog, patched_prog, follow_symlinks=True)
    shutil.copystat(prog, patched_prog, follow_symlinks=True)
    subprocess.run(["patchelf", "--add-needed",
                   library.name, str(patched_prog)])
    subprocess.run(["patchelf", "--add-rpath",
                   library.parent, str(patched_prog)])
    return patched_prog


@pytest.mark.performance
def test_kissat_runtime(output_dir_performance, tmp_path, lib_path, progs_dir):
    input_file = Path("./test/assets/cnf.out")
    shutil.copyfile(input_file, tmp_path / input_file.name)
    prepared_kissat = prepare_env(tmp_path, progs_dir / "kissat", lib_path)
    # Use absolute path, otherwise arguments are not working
    res = subprocess.run(
        ['/usr/bin/time', '-v', '--output=time.kissat',
            str(prepared_kissat), "-q", input_file.name],
        env={"SEALLOC_SEED": "1234"},
        capture_output=True,
        timeout=120,
        cwd=tmp_path
    )
    time_result = tmp_path / "time.kissat"
    if time_result.exists():
        shutil.copyfile(time_result, output_dir_performance / time_result.name)
    assert res.returncode == 10


@pytest.mark.performance
def test_kissat_syscalls(output_dir_performance, tmp_path, lib_path, progs_dir):
    input_file = Path("./test/assets/cnf.out")
    shutil.copyfile(input_file, tmp_path / input_file.name)
    prepared_kissat = prepare_env(tmp_path, progs_dir / "kissat", lib_path)
    res = subprocess.run(
        ['/usr/bin/strace', '-c', '--output=strace.kissat',
            str(prepared_kissat), "-q", input_file.name],
        env={"SEALLOC_SEED": "1234"},
        capture_output=True,
        timeout=120,
        cwd=tmp_path
    )
    strace_result = tmp_path / "strace.kissat"
    if strace_result.exists():
        shutil.copyfile(
            strace_result, output_dir_performance / strace_result.name)
    assert res.returncode == 10


@pytest.mark.performance
def test_kissat_massif(output_dir_performance, tmp_path, lib_path, progs_dir):
    input_file = Path("./test/assets/cnf.out")
    shutil.copyfile(input_file, tmp_path / input_file.name)
    prepared_kissat = prepare_env(tmp_path, progs_dir / "kissat", lib_path)
    res = subprocess.run(
        ['/usr/bin/valgrind', '--tool=massif', '--massif-out-file=massif.kissat', '--pages-as-heap=yes',
            str(prepared_kissat), "-q", input_file.name],
        env={"SEALLOC_SEED": "1234"},
        capture_output=True,
        timeout=120,
        cwd=tmp_path
    )
    massif_result = tmp_path / "massif.kissat"
    if massif_result.exists():
        shutil.copyfile(
            massif_result, output_dir_performance / massif_result.name)
    assert res.returncode == 10
