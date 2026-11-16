#!/usr/bin/env bash
# Build the Linux image for the riscv-emu machine with buildroot, out of tree.
#
#   scripts/build-linux.sh <buildroot-dir> <linux-source-dir> <out-dir>
#
# <out-dir> must be outside this repository. The result is <out-dir>/images/Image (the kernel with the root
# filesystem linked in as an initramfs); run it with: riscv_emu --bios-bin <fw_jump.bin> --kernel-bin <Image> --dtb <dtb>.
# The configuration lives in linux/ (a buildroot external tree): linux/configs/riscv_emu_defconfig and
# linux/board/riscv_emu/kernel.fragment. The linux source is used as it is, nothing is written into it.
set -euo pipefail

if [ $# -ne 3 ]; then
    echo "usage: $0 <buildroot-dir> <linux-source-dir> <out-dir>" >&2
    exit 3
fi

repo=$(cd "$(dirname "$0")/.." && pwd)
buildroot=$(cd "$1" && pwd)
linux_src=$(cd "$2" && pwd)
mkdir -p "$3"
out=$(cd "$3" && pwd)

case "$out/" in
    "$repo"/*) echo "error: the output directory must be outside the repository ($repo)" >&2; exit 3 ;;
esac
[ -f "$buildroot/Makefile" ] || { echo "error: $buildroot is not a buildroot tree" >&2; exit 3; }
[ -f "$linux_src/Makefile" ] || { echo "error: $linux_src is not a linux source tree" >&2; exit 3; }

jobs=$(nproc)
make -C "$buildroot" O="$out" BR2_EXTERNAL="$repo/linux" riscv_emu_defconfig
make -C "$out" LINUX_OVERRIDE_SRCDIR="$linux_src" -j"$jobs"

echo "built: $out/images/Image"
