#pragma once
#include "../src/cpu.h"

// Reset the CPU, then put it in the given privilege mode, with PMP entry 0 granting read, write
// and execute everywhere, as the riscv-tests harness does. Tests that need an empty PMP use
// cpu_reset() and set privilege_mode themselves.
static inline void test_cpu_reset_in_mode(CPU *cpu, PrivMode mode) {
	cpu_reset(cpu);
	cpu->privilege_mode = mode;
	cpu->pmpcfg[0] = 0x1F;          // NAPOT, R, W, X
	cpu->pmpaddr[0] = 0x7FFFFFFF;   // [0, 2^34)
}
