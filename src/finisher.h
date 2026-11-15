#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "mem.h"

// The test finisher: one register with which the guest tells the emulator to stop. It is not real hardware; it is the convention of
// QEMU's virt machine (the sifive,test0 device) and of the SiFive test finisher, and OpenSBI's syscon poweroff and reboot drivers
// write to it through the device tree. A write of the 32-bit register at offset 0 whose low 16 bits are:
//   FINISHER_PASS   the guest succeeded
//   FINISHER_FAIL   the guest failed, with the code in bits 31:16
//   FINISHER_RESET  the guest asks for a reboot
// Any other value is ignored. OpenSBI reads the register before it writes it (a regmap update), so a read returns 0.
#define FINISHER_BASE   0x00100000u
#define FINISHER_SIZE   0x1000u
#define FINISHER_PASS   0x5555u
#define FINISHER_FAIL   0x3333u
#define FINISHER_RESET  0x7777u

typedef struct Finisher {
	bool     done;     // a valid value was written; the first one stays
	uint32_t status;   // FINISHER_PASS, FINISHER_FAIL or FINISHER_RESET
	uint32_t code;     // the failure code (bits 31:16 of the write)
} Finisher;

void finisher_reset(Finisher *f);
// 32-bit accesses to offset 0 only; every other access is an access fault, like the other devices.
Device finisher_device(Finisher *f);   // for mem_add_device
