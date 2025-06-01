#!/usr/bin/env python3

import click
from pathlib import Path
import matplotlib.pyplot as plt
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


def make_plot(dirs):
    program_dirs = [Path(dir) for dir in dirs]
    allocators = ["Glibc", "Sealloc", "FFmalloc", "SlimGuard"]
    rename = {
        "glibc": "Glibc",
        "libffmallocnpst": "FFmalloc",
        "libsealloc": "Sealloc",
        "libSlimGuard": "SlimGuard"
    }
    programs = [dir.name for dir in program_dirs]

    fig, axs = plt.subplots(nrows=len(programs), ncols=1)
    for i, program in enumerate(program_dirs):
        plot_data = [None]*4
        for program_allocator in program.iterdir():
            allocator_name = rename[program_allocator.name.split('_')[1]]
            time_stats = [TimeStats(path)
                          for path in program_allocator.iterdir()]
            elapsed_time = np.array([st.elapsed_time for st in time_stats])
            max_rss = np.array([st.max_rss for st in time_stats])
            plot_data[allocators.index(allocator_name)] = elapsed_time
        axs.boxplot(plot_data, tick_labels=allocators)
    plt.show()


@click.command()
@click.argument("dirs", nargs=-1)
@click.option("--plot", is_flag=True)
def main(dirs, plot):
    if plot:
        make_plot(dirs)
    else:
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
