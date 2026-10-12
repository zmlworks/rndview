#!/bin/sh
# Random Buffer Viewer - Windows x64 GUI build (CRT-free) using MinGW-w64 cross toolchain
# Requires: gcc-mingw-w64-x86-64 binutils-mingw-w64-x86-64  (Debian/Ubuntu)
set -eu
cd "$(dirname "$0")"
CC=${CC:-x86_64-w64-mingw32-gcc}
WINDRES=${WINDRES:-x86_64-w64-mingw32-windres}

$WINDRES -O coff -i app.rc -o app.res.o

$CC -O2 -Wall -Wextra -std=c11 \
    -ffreestanding -fno-builtin -fno-tree-loop-distribute-patterns \
    -fno-stack-protector -fno-stack-check \
    -nostdlib -nostartfiles -mwindows \
    -Wl,-e,Entry -Wl,--subsystem,windows \
    -Wl,--dynamicbase -Wl,--nxcompat -Wl,--high-entropy-va \
    -s \
    -o rndview.exe rndview.c app.res.o \
    -lkernel32 -luser32 -lgdi32 -lbcrypt -lcomctl32

rm -f app.res.o
echo "built rndview.exe ($(stat -c %s rndview.exe) bytes)"
