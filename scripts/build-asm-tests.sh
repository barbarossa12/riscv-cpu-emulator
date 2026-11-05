#!/usr/bin/env bash
# Build our own assembly tests (tests/asm/*.S) into ELFs the emulator can run, the way riscv-tests
# builds its rv32mi tests: RV32, physical-mode "-p-" environment, the same flags and linker script.
#
#   usage: build-asm-tests.sh <riscv-tests-dir> <out-dir> <source.S>...
#   e.g.   build-asm-tests.sh riscv-tests build/riscv-tests tests/asm/pmp_denial.S
set -euo pipefail

if [ $# -lt 3 ]; then
	echo "usage: $0 <riscv-tests-dir> <out-dir> <source.S>..." >&2
	exit 2
fi
src=$(cd "$1" && pwd)
out=$2
shift 2

gcc=$(command -v riscv32-unknown-elf-gcc || command -v riscv64-unknown-elf-gcc || true)
if [ -z "$gcc" ]; then
	echo "error: no riscv64-unknown-elf-gcc or riscv32-unknown-elf-gcc on PATH" >&2
	exit 1
fi
if [ ! -f "$src/env/p/riscv_test.h" ]; then
	echo "error: $src/env is empty; run: git -C $src submodule update --init" >&2
	exit 1
fi

mkdir -p "$out"
for f in "$@"; do
	name=$(basename "$f" .S)
	"$gcc" -march=rv32g -mabi=ilp32 -static -mcmodel=medany -fvisibility=hidden -nostdlib -nostartfiles \
		-I"$src/env/p" -I"$src/isa/macros/scalar" -T"$src/env/p/link.ld" "$f" -o "$out/$name"
done
echo "built $# assembly tests in $out"
