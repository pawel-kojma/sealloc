# Notes on adding overflow detection using ARM-MTE

entry point: https://docs.kernel.org/arch/arm64/memory-tagging-extension.html
example usage: https://learn.arm.com/learning-paths/mobile-graphics-and-gaming/mte/mte/
ARM v8.5 ISA: http://kib.kiev.ua/x86docs/ARM/ARMARMv8/DDI0487F_a_armv8_arm.pdf

Build for ARM v8.5+memtag

```sh
$ CXX=/usr/bin/aarch64-linux-gnu-g++-12 CC=/usr/bin/aarch64-linux-gnu-gcc cmake -S . -B build -DMemtags=ON -DBuildType=Debug -DTests=ON -DAssert=ON -DLog=ON -DCMAKE_BUILD_TYPE=Debug -DBUILD_SHARED_LIBS=ON
```
