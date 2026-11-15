#pragma once
// The machine's fixed memory map and wiring that no device header owns. Shared by main.c and checked against
// dts/riscv-emu.dts by tests/check-dts.sh, so the device tree cannot silently drift from the emulator.
#define MACHINE_RAM_BASE   0x80000000u
#define MACHINE_RAM_SIZE   (128u * 1024 * 1024)
#define MACHINE_UART_IRQ   10u   // the UART's PLIC source

// Where boot images go. The firmware is loaded at the RAM base. The next stage is 4 MiB above it: that is OpenSBI's generic
// fw_jump default for RV32 (FW_JUMP_OFFSET 0x400000) and the 4 MiB alignment Linux asks for on rv32 (boot.rst). The device tree
// goes in the last 4 MiB of RAM, where neither OpenSBI (which copies it to 0x82200000 by itself) nor a kernel will touch it.
#define MACHINE_KERNEL_ADDR  (MACHINE_RAM_BASE + 0x400000u)
#define MACHINE_DTB_ADDR     (MACHINE_RAM_BASE + MACHINE_RAM_SIZE - 0x400000u)
#define MACHINE_DTB_MAX      0x100000u
