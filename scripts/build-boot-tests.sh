#!/usr/bin/env bash
# Build the boot stubs (tests/boot/*.S) into an ELF and a flat binary each, for the loader tests: RV32, no C library, no riscv-tests
# environment. A stub is linked at 0x80000000, the RAM base, where the firmware goes; a name ending in _kernel is linked at
# 0x80400000 instead, where the next stage goes.
#
#   usage: build-boot-tests.sh <out-dir> <source.S>...
#   e.g.   build-boot-tests.sh build/boot-tests tests/boot/boot_ok.S
set -euo pipefail

if [ $# -lt 2 ]; then
	echo "usage: $0 <out-dir> <source.S>..." >&2
	exit 2
fi
out=$1
shift

gcc=$(command -v riscv32-unknown-elf-gcc || command -v riscv64-unknown-elf-gcc || true)
objcopy=$(command -v riscv32-unknown-elf-objcopy || command -v riscv64-unknown-elf-objcopy || true)
if [ -z "$gcc" ] || [ -z "$objcopy" ]; then
	echo "error: no riscv64-unknown-elf-gcc / objcopy (or riscv32-) on PATH" >&2
	exit 1
fi

mkdir -p "$out"
for f in "$@"; do
	name=$(basename "$f" .S)
	text=0x80000000
	case "$name" in *_kernel) text=0x80400000 ;; esac
	"$gcc" -march=rv32imac_zicsr_zifencei -mabi=ilp32 -static -nostdlib -nostartfiles -Wl,--build-id=none -Wl,-N -Wl,--no-warn-rwx-segments \
		-Wl,-Ttext="$text" -I"$(dirname "$f")" "$f" -o "$out/$name.elf"
	"$objcopy" -O binary "$out/$name.elf" "$out/$name.bin"
done
echo "built $# boot stubs in $out"
