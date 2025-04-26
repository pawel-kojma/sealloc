#!/usr/bin/env python

from datetime import datetime
from itertools import chain
from pathlib import Path
import re

import click
import matplotlib.patches as mpatches
import matplotlib.pyplot as plt

COLOR_MAP = {"S": "r", "M": "g", "L": "b", "H": "k"}
CLASS_MAP = {"r": "S", "g": "M", "b": "L", "k": "H"}

HANDLES = [
    mpatches.Patch(color="red", label="Small"),
    mpatches.Patch(color="green", label="Medium"),
    mpatches.Patch(color="blue", label="Large"),
    mpatches.Patch(color="black", label="Huge"),
]


def flatten(list_of_lists):
    "Flatten one level of nesting."
    return chain.from_iterable(list_of_lists)


class TimeStats:
    TIME = "Elapsed (wall clock) time (h:mm:ss or m:ss)"
    MAX_RSS = "Maximum resident set size (kbytes)"

    def __init__(self, path):
        g = filter(lambda x: len(x) == 2, [line.strip().split(": ")
                                           for line in path.read_text().splitlines()])
        self.data_dict = dict(g)
        dt = datetime.strptime(
            self.data_dict[self.TIME], "%M:%S.%f"
        )
        self.elapsed_time = dt.second + dt.microsecond / 10**6
        self.max_rss = int(self.data_dict[self.MAX_RSS])


class StraceStats:
    SCALL_IDX = 5
    CALLS_IDX = 3
    LINE_LEN_ELEMS = 6

    def __init__(self, path):
        lines = [re.sub(r'[ ]+', ' ', line).strip().split()
                 for line in path.read_text().splitlines()][2:-2]
        for line in lines:
            if len(line) != self.LINE_LEN_ELEMS:
                line.insert(4, '0')
        self.data_dict = dict(
            (line[self.SCALL_IDX], int(line[self.CALLS_IDX])) for line in lines)


def create_plot(data_dirs, program_name):
    time_stats = [TimeStats(dir / f"time.{program_name}") for dir in data_dirs]
    strace_stats = [StraceStats(
        dir / f"strace.{program_name}") for dir in data_dirs]


@click.command()
@click.argument("test_dirs", nargs=-1, type=click.Path(file_okay=False, path_type=Path))
def main(test_dirs: list[Path]):
    print("Detected dirs:\n" + "\n".join(map(str, test_dirs)))
    test_dirs = [dir / "performance" for dir in test_dirs]
    h = flatten(map(lambda path: path.iterdir(), test_dirs))
    all_programs = set(path.name for path in h)
    for program in all_programs:
        m = [dir / program for dir in test_dirs]
        create_plot(list(filter(lambda dir: dir.exists(), m)), program)


if __name__ == "__main__":
    main()
