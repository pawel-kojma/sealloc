#!/usr/bin/env python3

import click
from pathlib import Path
import os
import subprocess
import tqdm


def run_tests(prog_path, n, out, cmd_args, input_file=None, rc=0):
    i = 0
    base_out = out / prog_path.name
    base_out.mkdir(exist_ok=True)
    cmd = [str(prog_path)] + cmd_args.split()
    print(f'Testing {prog_path.name}')
    print(f"cmd: {cmd}")
    print(f"base_out: {str(base_out)}")
    print(f'input_file: {input_file}')
    print(f'rc: {rc}')
    while i < n:
        print(f'Iteration {i}')
        outfile = base_out / f"time.{prog_path.name}.{str(i)}"
        full_cmd = ["/usr/bin/time", "-v", f"--output={str(outfile)}"] + cmd
        if not input_file:
            process = subprocess.run(
                full_cmd, env={"SEALLOC_SEED": "1234"}, capture_output=True)
        else:
            process = subprocess.run(
                full_cmd, capture_output=True, env={"SEALLOC_SEED": "1234"}, input=input_file.read_bytes()
            )
        if process.returncode != rc:
            print(f"program {prog_path.name} failed at iteration {
                  i}, ({process.returncode=})")
            print(process)
        else:
            i += 1


@click.command()
@click.option("-p", "--progs-path", type=str, required=True)
@click.option("-n", "--prog-name", type=str, required=True)
@click.option("-c", "--cmd-args", type=str, required=True)
@click.option("-t", "--test-runs", type=int, default=50)
@click.option("-o", "--out-dir", type=str, default=".")
@click.option("-r", "--return-code", type=int, default=0)
@click.option("-i", "--input-file", type=str, default="")
def main(progs_path, prog_name, cmd_args, test_runs, out_dir, input_file, return_code):
    out = Path(out_dir) / prog_name
    progs_path = Path(progs_path)
    input_file = Path(input_file) if input_file != "" else None
    out.mkdir(exist_ok=True, parents=True)
    if progs_path.is_file():
        test_prog = progs_path
        run_tests(
            test_prog, test_runs, out, cmd_args, input_file=input_file, rc=return_code
        )
    else:
        for test_prog in filter(
            lambda f: os.access(f, os.X_OK) and f.is_file(), progs_path.iterdir()
        ):
            run_tests(
                test_prog, test_runs, out, cmd_args, input_file=input_file, rc=return_code
            )


if __name__ == "__main__":
    main()
