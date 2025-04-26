import shutil
import subprocess
from pathlib import Path

import pytest


def prepare_env(dir: Path, prog: Path, library: Path):
    prog_name = prog.name
    patched_prog = dir / prog_name
    shutil.copyfile(prog, patched_prog, follow_symlinks=True)
    shutil.copystat(prog, patched_prog, follow_symlinks=True)
    if library:
        subprocess.run(["patchelf", "--add-needed", library.name, str(patched_prog)])
        subprocess.run(["patchelf", "--add-rpath", library.parent, str(patched_prog)])


def run_test(
    tmp_path,
    output_dir_performance,
    progs_dir,
    lib_path,
    prog_name,
    cmd,
    ret_val=0,
    input_file=None,
    as_input=False,
):
    if input_file:
        shutil.copyfile(input_file, tmp_path / input_file.name)
    prepare_env(tmp_path, progs_dir / prog_name, lib_path)
    if as_input:
        res = subprocess.run(
            cmd,
            env={"SEALLOC_SEED": "1234"},
            capture_output=True,
            cwd=tmp_path,
            input=input_file.read_bytes(),
        )
    else:
        res = subprocess.run(
            cmd, env={"SEALLOC_SEED": "1234"}, capture_output=True, cwd=tmp_path
        )
    report_file = next(tmp_path.glob(f"*.{prog_name}"), Path())
    if report_file.exists():
        (output_dir_performance / prog_name).mkdir(exist_ok=True)
        shutil.copyfile(
            report_file, output_dir_performance / prog_name / report_file.name
        )
    assert res.returncode == ret_val


@pytest.mark.performance
@pytest.mark.parametrize(
    "cmd",
    [
        ["/usr/bin/time", "-v", "--output=time.kissat"],
        ["/usr/bin/strace", "-c", "--output=strace.kissat"],
        [
            "/usr/bin/valgrind-3.24.0-more_vg_n_segments",
            "--tool=callgrind",
            "--callgrind-out-file=callgrind.kissat",
        ],
        [
            "/usr/bin/valgrind-3.24.0-more_vg_n_segments",
            "--tool=massif",
            "--massif-out-file=massif.kissat",
            "--pages-as-heap=yes",
        ],
    ],
)
def test_performance_on_kissat(
    cmd, output_dir_performance, tmp_path, lib_path, progs_dir
):
    input_file = Path("./test/assets/kissat_cnf.out")
    test_cmd = cmd + [str(tmp_path / "kissat"), "-q", input_file.name]
    run_test(
        tmp_path,
        output_dir_performance,
        progs_dir,
        lib_path,
        "kissat",
        test_cmd,
        ret_val=10,
        input_file=input_file,
    )


@pytest.mark.performance
@pytest.mark.parametrize(
    "cmd",
    [
        ["/usr/bin/time", "-v", "--output=time.espresso"],
        ["/usr/bin/strace", "-c", "--output=strace.espresso"],
        [
            "/usr/bin/valgrind-3.24.0-more_vg_n_segments",
            "--tool=callgrind",
            "--callgrind-out-file=callgrind.espresso",
        ],
        [
            "/usr/bin/valgrind-3.24.0-more_vg_n_segments",
            "--tool=massif",
            "--massif-out-file=massif.espresso",
            "--pages-as-heap=yes",
        ],
    ],
)
def test_performance_on_espresso(
    cmd, output_dir_performance, tmp_path, lib_path, progs_dir
):
    input_file = Path("./test/assets/largest_espresso")
    test_cmd = cmd + [str(tmp_path / "espresso"), input_file.name]
    run_test(
        tmp_path,
        output_dir_performance,
        progs_dir,
        lib_path,
        "espresso",
        test_cmd,
        input_file=input_file,
    )


@pytest.mark.performance
@pytest.mark.parametrize(
    "cmd",
    [
        ["/usr/bin/time", "-v", "--output=time.barnes"],
        ["/usr/bin/strace", "-c", "--output=strace.barnes"],
        [
            "/usr/bin/valgrind-3.24.0-more_vg_n_segments",
            "--tool=callgrind",
            "--callgrind-out-file=callgrind.barnes",
        ],
        [
            "/usr/bin/valgrind-3.24.0-more_vg_n_segments",
            "--tool=massif",
            "--massif-out-file=massif.barnes",
            "--pages-as-heap=yes",
        ],
    ],
)
def test_performance_on_barnes(
    cmd, output_dir_performance, tmp_path, lib_path, progs_dir
):
    input_file = Path("./test/assets/barnes_input")
    test_cmd = cmd + [str(tmp_path / "barnes"), input_file.name]
    run_test(
        tmp_path,
        output_dir_performance,
        progs_dir,
        lib_path,
        "barnes",
        test_cmd,
        input_file=input_file,
        as_input=True,
    )


@pytest.mark.performance
@pytest.mark.parametrize(
    "cmd",
    [
        ["/usr/bin/time", "-v", "--output=time.cfrac"],
        ["/usr/bin/strace", "-c", "--output=strace.cfrac"],
        [
            "/usr/bin/valgrind-3.24.0-more_vg_n_segments",
            "--tool=callgrind",
            "--callgrind-out-file=callgrind.cfrac",
        ],
        [
            "/usr/bin/valgrind-3.24.0-more_vg_n_segments",
            "--tool=massif",
            "--massif-out-file=massif.cfrac",
            "--pages-as-heap=yes",
        ],
    ],
)
def test_performance_on_cfrac(
    cmd, output_dir_performance, tmp_path, lib_path, progs_dir
):
    test_cmd = cmd + [
        str(tmp_path / "cfrac"),
        "3707030275882252342412325295197136712092001",
    ]
    run_test(tmp_path, output_dir_performance, progs_dir, lib_path, "cfrac", test_cmd)


@pytest.mark.performance
@pytest.mark.parametrize(
    "cmd",
    [
        ["/usr/bin/time", "-v", "--output=time.ghostscript"],
        ["/usr/bin/strace", "-c", "--output=strace.ghostscript"],
        [
            "/usr/bin/valgrind-3.24.0-more_vg_n_segments",
            "--tool=callgrind",
            "--callgrind-out-file=callgrind.ghostscript",
        ],
        [
            "/usr/bin/valgrind-3.24.0-more_vg_n_segments",
            "--tool=massif",
            "--massif-out-file=massif.ghostscript",
            "--pages-as-heap=yes",
        ],
    ],
)
def test_performance_on_ghostscript(
    cmd, output_dir_performance, tmp_path, lib_path, progs_dir
):
    input_file = Path("./test/assets/intel_sdm_vol1_small.pdf")
    test_cmd = cmd + [
        str(tmp_path / "ghostscript"),
        "-dBATCH",
        "-dNODISPLAY",
        input_file.name,
    ]
    run_test(
        tmp_path,
        output_dir_performance,
        progs_dir,
        lib_path,
        "ghostscript",
        test_cmd,
        input_file=input_file,
    )


@pytest.mark.performance
@pytest.mark.parametrize(
    "cmd",
    [
        ["/usr/bin/time", "-v", "--output=time.gcc"],
        ["/usr/bin/strace", "-c", "--output=strace.gcc"],
        [
            "/usr/bin/valgrind-3.24.0-more_vg_n_segments",
            "--tool=callgrind",
            "--callgrind-out-file=callgrind.gcc",
        ],
        [
            "/usr/bin/valgrind-3.24.0-more_vg_n_segments",
            "--tool=massif",
            "--massif-out-file=massif.gcc",
            "--pages-as-heap=yes",
        ],
    ],
)
def test_performance_on_gcc(cmd, output_dir_performance, tmp_path, lib_path, progs_dir):
    """GCC compiling lua interpreter source code."""
    input_dir = Path("./test/assets/lua/")
    work_dir = tmp_path
    gcc_libs_dir = work_dir / "lib" / "gcc" / "x86_64-linux-gnu"
    gcc_libs_dir.mkdir(parents=True, exist_ok=True)
    gcc_bin_dir = work_dir / "bin"
    gcc_bin_dir.mkdir(parents=True, exist_ok=True)
    prepare_env(gcc_bin_dir, progs_dir / "gcc", lib_path)
    lua_path = work_dir / "lua"
    shutil.copytree(input_dir, lua_path)
    shutil.copytree(Path("/usr/lib/gcc/x86_64-linux-gnu/12/"), gcc_libs_dir / "12")
    res = subprocess.run(
        cmd
        + [str(gcc_bin_dir / "gcc"), "-S", "-O2", "-std=c99", "-o", "lua", "onelua.c"],
        env={"SEALLOC_SEED": "1234"},
        capture_output=True,
        cwd=lua_path,
    )
    report_file = next(lua_path.glob("*.gcc"), None)
    if report_file:
        (output_dir_performance / "gcc").mkdir(exist_ok=True)
        shutil.copyfile(report_file, output_dir_performance / "gcc" / report_file.name)
    assert res.returncode == 0
