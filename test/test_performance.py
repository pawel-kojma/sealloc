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
@pytest.mark.parametrize("cmd", [
    ['/usr/bin/time', '-v', '--output=time.kissat'],
    ['/usr/bin/strace', '-c', '--output=strace.kissat'],
    ['/usr/bin/valgrind', '--tool=callgrind',
     '--callgrind-out-file=callgrind.kissat'],
    ['/usr/bin/valgrind', '--tool=massif',
     '--massif-out-file=massif.kissat', '--pages-as-heap=yes']
])
def test_performance_on_kissat(cmd, output_dir_performance, tmp_path, lib_path, progs_dir):
    input_file = Path("./test/assets/cnf_small.out")
    shutil.copyfile(input_file, tmp_path / input_file.name)
    prepared_kissat = prepare_env(tmp_path, progs_dir / "kissat", lib_path)
    res = subprocess.run(cmd + [
        str(prepared_kissat), "-q", input_file.name],
        env={"SEALLOC_SEED": "1234"},
        capture_output=True,
        timeout=120,
        cwd=tmp_path
    )
    report_file = next(tmp_path.glob('*.kissat'), Path())
    if report_file.exists():
        (output_dir_performance / "kissat").mkdir(exist_ok=True)
        shutil.copyfile(
            report_file, output_dir_performance / "kissat" / report_file.name)
    assert res.returncode == 10


@pytest.mark.performance
@pytest.mark.parametrize("cmd", [
    ['/usr/bin/time', '-v', '--output=time.cfrac'],
    ['/usr/bin/strace', '-c', '--output=strace.cfrac'],
    ['/usr/bin/valgrind', '--tool=callgrind',
     '--callgrind-out-file=callgrind.cfrac'],
    ['/usr/bin/valgrind', '--tool=massif',
     '--massif-out-file=massif.cfrac', '--pages-as-heap=yes']
])
def test_performance_on_cfrac(cmd, output_dir_performance, tmp_path, lib_path, progs_dir):
    prepared_cfrac = prepare_env(tmp_path, progs_dir / "cfrac", lib_path)
    res = subprocess.run(cmd + [
        str(prepared_cfrac), "3707030275882252342412325295197136712092001"],
        env={"SEALLOC_SEED": "1234"},
        capture_output=True,
        timeout=120,
        cwd=tmp_path
    )
    report_file = next(tmp_path.glob('*.cfrac'), Path())
    if report_file.exists():
        (output_dir_performance / "cfrac").mkdir(exist_ok=True)
        shutil.copyfile(
            report_file, output_dir_performance / "cfrac" / report_file.name)
    assert res.returncode == 10
