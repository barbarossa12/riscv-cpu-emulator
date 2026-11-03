#!/usr/bin/env bash
# Build official riscv-tests ELFs (RV32, physical-mode "-p-" environment).
#
#   usage: build-riscv-tests.sh <riscv-tests-dir> <out-dir> <test-name>...
#   e.g.   build-riscv-tests.sh riscv-tests build/riscv-tests rv32ui-p-add rv32mi-p-csr
set -euo pipefail

if [ $# -lt 3 ]; then
	echo "usage: $0 <riscv-tests-dir> <out-dir> <test-name>..." >&2
	exit 2
fi
src=$(cd "$1" && pwd)
out=$2
shift 2

if ! command -v riscv64-unknown-elf-gcc >/dev/null && ! command -v riscv32-unknown-elf-gcc >/dev/null; then
	echo "error: no riscv64-unknown-elf-gcc or riscv32-unknown-elf-gcc on PATH" >&2
	exit 1
fi
if [ ! -f "$src/env/p/riscv_test.h" ]; then
	echo "error: $src/env is empty; run: git -C $src submodule update --init" >&2
	exit 1
fi

mkdir -p "$out"
cd "$out"
make -s -f "$src/isa/Makefile" src_dir="$src/isa" XLEN=32 -j"$(nproc)" "$@"
echo "built $# riscv-tests ELFs in $out"
