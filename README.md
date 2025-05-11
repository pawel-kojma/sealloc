# Security enhanced allocator

Prototype `malloc(3)` implementation which aims to improve security.
Key features are:
- Separation of allocator metadata and user data to avoid leakage and corruption.
- Double-free resistance.
- Linear overflow resistance using guard pages.
- Protection from freeing fake region.
- Use-after-free protection using one time allocation.
- Randomized region allocation.

## Build

To build the project run following commands:

```bash
$ cmake -S . -B build
$ cmake --build build -- -j
```

Additional flags may be specified to get specialized build:
```
-DLog=ON/OFF - Enable library logging
-DBuildType=Release/Debug - Build release version with optimization or debug with debugging symbols
-DBUILD_SHARED_LIBS=ON/OFF - Build shared library
-DTests=ON/OFF - Build tests
-DAssert=ON/OFF - Build with assertions
```

## Tests

To run unit tests, build the project with `-DTests=ON` and enter following command:
```
$ ctest --test-dir ./build/test/
```

To run E2E tests, enter following commands from top directory:
```
$ python -m venv venv
$ . venv/bin/activate
$ pip install pytest
$ pytest -m real_programs ./test/ --lib-path ./build/src/libsealloc.so --progs-dir <programs_dir>
```

To run security tests, enter following commands from top directory:
```
$ pytest -m security ./test/ --lib-path=./build/src/libsealloc.so
```

To generate performance reports you need:
- a directory with kissat, cfrac, espresso, barnes, ghostscript and gcc
- installed programs - strace, time, valgrind
```
$ pytest -m performance ./test/ --lib-path ./build/src/libsealloc.so --progs-dir <programs_dir>
```
