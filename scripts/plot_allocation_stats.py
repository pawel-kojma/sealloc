#!/usr/bin/env python

import sys
from collections import Counter
from pathlib import Path

import matplotlib.patches as mpatches
import matplotlib.pyplot as plt
import numpy as np

COLOR_MAP = {"S": "r", "M": "g", "L": "b", "H": "k"}
CLASS_MAP = {"r": "S", "g": "M", "b": "L", "k": "H"}

HANDLES = [
    mpatches.Patch(color="red", label="Small"),
    mpatches.Patch(color="green", label="Medium"),
    mpatches.Patch(color="blue", label="Large"),
    mpatches.Patch(color="black", label="Huge"),
]


def parse(line):
    s = line.split()
    return COLOR_MAP[s[0]], int(s[1])


def main():
    data_path = Path(sys.argv[1])
    if data_path.is_file():
        files = [data_path]
    elif data_path.is_dir():
        files = list(data_path.glob("alloc_stats.*"))
    else:
        files = []
    print("Detected files:\n" + "\n".join(map(str, files)))
    for file in files:
        print(f"Processing {file.name}...")
        print("Reading data...")
        allocs = list(map(parse, file.read_text().splitlines()))
        colors = list(map(lambda x: x[0], allocs))
        sizes = list(map(lambda x: x[1], allocs))
        x_axis = list(range(1, len(sizes) + 1))
        size_mb = file.stat().st_size / 2**20
        print(
            f"Read total of {len(allocs)} samples ({size_mb:.2f} MB). "
            "Creating the plot..."
        )
        fig, axs = plt.subplots()

        # Set scatter chart
        plt.yscale("log")
        scatter = axs.scatter(x=x_axis, y=sizes, c=colors, s=0.1)
        legend = axs.legend(
            handles=HANDLES, loc="upper left", title="Klasa alokacji")
        axs.add_artist(legend)
        axs.set_xlabel("Czas (w alokacjach)")
        axs.set_ylabel("Rozmiar")
        out_file = file.name.split(".")[1]
        print("Saving...")
        fig.savefig(out_file)
        print(f"Saved to {out_file}.png")
        plt.close()


if __name__ == "__main__":
    main()
