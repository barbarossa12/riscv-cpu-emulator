#pragma once
// The machine's fixed memory map and wiring that no device header owns. Shared by main.c and checked against
// dts/riscv-emu.dts by tests/check-dts.sh, so the device tree cannot silently drift from the emulator.
#define MACHINE_RAM_BASE   0x80000000u
#define MACHINE_RAM_SIZE   (128u * 1024 * 1024)
#define MACHINE_UART_IRQ   10u   // the UART's PLIC source
