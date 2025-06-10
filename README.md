# Security enhanced allocator

Prototype allocator implementation which aims to improve security.
Key features are:

- Separation of allocator metadata and user data to avoid leakage and corruption.
- Double-free resistance.
- Protection from freeing fake region.
- Use-after-free protection using one time allocation.
- Randomized region allocation.

## Build

To build the project run following commands:

```bash
$ cmake -S . -B build
$ cmake --build build -- -j
```

To build the project with memory tagging extension (MTE) you need to specify cross compiler paths:

```shell
$ export CXX=</path/to/cpp/compiler>
$ export CC=</path/to/c/compiler>
$ cmake -S . -B build -DMemtags=ON
```

Additional flags may be specified to get specialized build:

```text
-DCMAKE_BUILD_TYPE=Release/Debug - Build library in release / debug mode
-DLog=ON/OFF - Enable library logging
-DBUILD_SHARED_LIBS=ON/OFF - Build shared library
-DTests=ON/OFF - Build tests
-DAssert=ON/OFF - Build with assertions
-DMemtags=ON/OFF - Build for ARMv8.5-A with memory tagging extension support
```

## Tests

To run unit and integration tests, enter following command:

```shell
$ ctest --test-dir ./build/test/
```

To run security tests, enter following commands from top directory:

```shell
$ pytest -rx -m security ./test/ --lib-path=./build/src/libsealloc.so
```

## Resources

Final design was inspired by following resources and publications:
1\. [Beichen Liu, Pierre Olivier, and Binoy Ravindran. 2019. SlimGuard: A Secure
and Memory-Efficient Heap Allocator. In Middleware ’19: Middleware ’19: 20th
International Middleware Conference, December 8–13, 2019, Davis, CA, USA.
ACM, New York, NY, USA, 13 pages](https://doi.org/10.1145/3361525.3361532)
2\. [Brian Wickman, Hong Hu, Insu Yun, Daehee Jang, JungWon Lim, Sanidhya
Kashyap, Taesoo Kim, Preventing Use-After-Free Attacks with Fast Forward
Allocation.](https://www.usenix.org/system/files/sec21-wickman.pdf)
3\. [Jason Evans, A Scalable Concurrent malloc(3) Implementation for FreeBSD.](https://people.freebsd.org/~jasone/jemalloc/bsdcan2006/jemalloc.pdf)
4\. [ARM v8.5 ISA](http://kib.kiev.ua/x86docs/ARM/ARMARMv8/DDI0487F_a_armv8_arm.pdf)
5\. [Linux userspace MTE support.](https://docs.kernel.org/arch/arm64/memory-tagging-extension.html)
