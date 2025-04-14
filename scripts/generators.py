import jinja2
import math
from pathlib import Path

PAGE_SIZE = 2**12
RUN_SIZE = 4 * PAGE_SIZE
MIN_SIZE_CLASS_SMALL = 16
MAX_SIZE_CLASS_SMALL = 512
MIN_SIZE_CLASS_MEDIUM = 1024
MAX_SIZE_CLASS_MEDIUM = 8192

SIZE_CLASS_SMALL = [x for x in range(
    MIN_SIZE_CLASS_SMALL, MAX_SIZE_CLASS_SMALL + 1, MIN_SIZE_CLASS_SMALL)]
SIZE_CLASS_MEDIUM = [1024, 2048, 4096, 8192]


def coprime(n):
    """Elements from 1, ..., n coprime to n."""
    return list(filter(lambda x: math.gcd(x, n) == 1, range(1, n+1)))


def main():
    loader = jinja2.FileSystemLoader('templates/')
    sc_small_gen = [coprime(RUN_SIZE // n) for n in SIZE_CLASS_SMALL]
    sc_medium_gen = [coprime(RUN_SIZE // n) for n in SIZE_CLASS_MEDIUM]
    env = jinja2.Environment(
        loader=loader,
        trim_blocks=True,
        lstrip_blocks=True,
        autoescape=jinja2.select_autoescape()
    )
    env.globals.update(len=len)
    env.globals.update(max=max)
    max_gens_small = max(len(g) for g in sc_small_gen)
    max_gens_medium = max(len(g) for g in sc_medium_gen)
    gen_header = env.get_template('generator.j2').render(
        sc_small_gen=sc_small_gen, sc_medium_gen=sc_medium_gen, max_gens_small=max_gens_small, max_gens_medium=max_gens_medium)
    hdr = Path('src/sealloc/generator.h')
    hdr.touch(exist_ok=True)
    hdr.write_text(gen_header)


if __name__ == '__main__':
    main()
