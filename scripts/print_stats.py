#!/usr/bin/env python3

import click
from pathlib import Path
import os
import subprocess
import tqdm
import re
from datetime import datetime
from itertools import chain
import numpy as np


def flatten(list_of_lists):
    "Flatten one level of nesting."
    return chain.from_iterable(list_of_lists)


class TimeStats:
    TIME = "Elapsed (wall clock) time (h:mm:ss or m:ss)"
    MAX_RSS = "Maximum resident set size (kbytes)"

    def __init__(self, path):
        g = filter(
            lambda x: len(x) == 2,
            [line.strip().split(": ")
             for line in path.read_text().splitlines()],
        )
        self.data_dict = dict(g)
        dt = datetime.strptime(self.data_dict[self.TIME], "%M:%S.%f")
        self.elapsed_time = dt.second + dt.microsecond / 10**6
        self.max_rss = int(self.data_dict[self.MAX_RSS])

def convert_mb(kb):
    return kb / 1000

@click.command()
@click.argument("dirs", nargs=-1)
def main(dirs):
    for dir in dirs:
        dir = Path(dir)
        time_stats = [TimeStats(path) for path in dir.iterdir()]
        elapsed_time = np.array([st.elapsed_time for st in time_stats])
        max_rss = np.array([st.max_rss for st in time_stats])
        print(f'Stats for {dir.name}')
        print(f'Mean time: {np.mean(elapsed_time):.3f}s')
        print(f'Std time: {np.std(elapsed_time):.3f}s')
        print(f'Mean mem: {convert_mb(np.mean(max_rss)):.3f} MB')
        print(f'Std mem: {convert_mb(np.std(max_rss)):.3f} MB')
        print()


if __name__ == "__main__":
    main()
