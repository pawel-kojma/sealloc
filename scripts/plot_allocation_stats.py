#!/usr/bin/env python

import matplotlib.pyplot as plt
import numpy as np
import sys
from pathlib import Path

COLOR_MAP = {
    'S': 0,
    'M': 1,
    'L': 2,
    'H': 3
}


def parse(line):
    s = line.split()
    return COLOR_MAP[s[0]], int(s[1])


def main():
    data_dir = Path(sys.argv[1])
    files = list(data_dir.glob('alloc_stats.*'))
    print('Detected files:\n' + '\n'.join(map(str, files)))
    for file in files:
        print(f'Processing {file.name}...')
        print(f'Reading data...')
        allocs = list(map(parse, file.read_text().splitlines()))
        colors = list(map(lambda x: x[0], allocs))
        sizes = list(map(lambda x: x[1], allocs))
        x_axis = list(range(1, len(sizes)+1))
        size_mb = file.stat().st_size / 2**20
        print(
                f'Read total of {len(allocs)} samples ({size_mb:.2f} MB). Creating the plot...')
        fig, ax = plt.subplots()
        scatter = ax.scatter(x=x_axis, y=sizes, c=colors)
        out_file = file.name.split('.')[1]
        print('Saving...')
        fig.savefig(out_file)
        print(f'Saved to {out_file}.png')


if __name__ == '__main__':
    main()
