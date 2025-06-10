#!/usr/bin/env python3

import click
from pathlib import Path
import os
import shutil
import subprocess


@click.command()
@click.option('-p', '--progs-dir', type=str, required=True)
@click.option('-a', '--allocator-path', type=str, required=True)
@click.option('-o', '--out-dir', type=str, default='.')
def main(progs_dir, allocator_path, out_dir):
    out = Path(out_dir)
    allocator = Path(allocator_path).resolve()
    progs_dir = Path(progs_dir)
    out.mkdir(exist_ok=True)
    for prog in filter(lambda f: os.access(f, os.X_OK) and f.is_file(), progs_dir.iterdir()):
        (out / prog.name).mkdir(exist_ok=True)
        patched_prog = out / prog.name / f"{prog.name}_{allocator.with_suffix('').name}"
        shutil.copyfile(prog, patched_prog, follow_symlinks=True)
        shutil.copystat(prog, patched_prog, follow_symlinks=True)
        subprocess.run(["patchelf", "--add-needed",
                       allocator.name, str(patched_prog)])
        subprocess.run(["patchelf", "--add-rpath",
                       allocator.parent, str(patched_prog)])


if __name__ == '__main__':
    main()
