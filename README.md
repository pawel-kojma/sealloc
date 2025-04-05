# Security enchanced allocator

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
```

## Tests

To run unit tests, build the project with `-DTests=ON` and enter following command:
```
$ ctest --test-dir ./build/test/
```
