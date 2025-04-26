#!/usr/bin/env python

import re
from datetime import datetime
from itertools import chain
from pathlib import Path
import numpy as np
import click
import matplotlib.pyplot as plt


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


class StraceStats:
    SCALL_IDX = 5
    CALLS_IDX = 3
    ERRORS_IDX = 4
    LINE_LEN_ELEMS = 6

    def __init__(self, path):
        lines = [
            re.sub(r"[ ]+", " ", line).strip().split()
            for line in path.read_text().splitlines()
        ][2:-2]
        for line in lines:
            if len(line) != self.LINE_LEN_ELEMS:
                line.insert(4, "0")
        self.call_data = dict(
            (line[self.SCALL_IDX], int(line[self.CALLS_IDX])) for line in lines
        )
        self.errors = dict(
            (line[self.SCALL_IDX], int(line[self.ERRORS_IDX])) for line in lines
        )


def get_strace_stats(strace_stats, syscall):
    return (np.array([s.call_data.get(syscall, 0) for s in strace_stats]),
            np.array([s.errors.get(syscall, 0) for s in strace_stats]))


def create_plot(data_dirs, allocator_names, program_name):
    time_stats = [TimeStats(dir / f"time.{program_name}") for dir in data_dirs]
    strace_stats = [StraceStats(
        dir / f"strace.{program_name}") for dir in data_dirs]

    fig, (ax_time, ax_rss, ax_syscalls) = plt.subplots(
        3, 1, figsize=(7, 15), layout="constrained"
    )
    fig.suptitle(
        f"Porównanie alokatorów dla programu: {program_name}", fontsize=16)
    rects = ax_time.bar(allocator_names, [
        t.elapsed_time for t in time_stats], width=0.1)
    ax_time.bar_label(rects, padding=3)
    ax_time.set_ylabel("Czas (s)")
    ax_time.set_title("Czas wykonania")
    rects = ax_rss.bar(allocator_names, [
                       t.max_rss for t in time_stats], width=0.1)
    ax_rss.bar_label(rects, padding=3)
    ax_rss.set_ylabel("Max. zbiór rezydentny (Kb)")
    ax_rss.set_title("Maksymalne zużycie pamięci")

    syscall_stats = {
        'mmap': get_strace_stats(strace_stats, 'mmap'),
        'munmap': get_strace_stats(strace_stats, 'munmap'),
        'mprotect': get_strace_stats(strace_stats, 'mprotect'),
        'madvise': get_strace_stats(strace_stats, 'madvise'),
    }
    x = np.arange(len(allocator_names))
    idx = 0
    width = 0.25
    for scall_name, measurements in syscall_stats.items():
        call_data, errors_calls = measurements
        success_calls = call_data - errors_calls
        bottom = np.zeros(len(allocator_names))
        offset = width * idx
        ax_syscalls.bar(x + offset, success_calls,
                        width=width, label=scall_name, bottom=bottom)
        bottom += success_calls
        rects = ax_syscalls.bar(x + offset, errors_calls,
                                width=width, label=scall_name, bottom=bottom)
        ax_syscalls.bar_label(rects, padding=3)
        idx += 1
    ax_syscalls.set_ylabel('Ilość wywołań')
    ax_syscalls.set_xticks(x + width, allocator_names)
    ax_syscalls.set_title(
        'Ilość wykonanych wywołań systemowych')
    return fig


@click.command()
@click.option(
    "--output_dir",
    "-o",
    type=click.Path(file_okay=False, path_type=Path),
    default=Path("out_performance_plots/"),
    help="Plots output directory",
)
@click.argument("test_dirs", nargs=-1, type=click.Path(file_okay=False, path_type=Path))
def main(test_dirs: list[Path], output_dir: Path):
    print("Detected dirs:\n" + "\n".join(map(str, test_dirs)))
    allocator_names = [test_dir.name.replace(
        "test_out_", "") for test_dir in test_dirs]
    test_dirs = [dir / "performance" for dir in test_dirs]
    h = flatten(map(lambda path: path.iterdir(), test_dirs))
    all_programs = set(path.name for path in h)
    output_dir.mkdir(exist_ok=True)
    for program in all_programs:
        m = [dir / program for dir in test_dirs]
        fig = create_plot(
            list(filter(lambda dir: dir.exists(), m)), allocator_names, program
        )
        fig.savefig(output_dir / program)


if __name__ == "__main__":
    main()
