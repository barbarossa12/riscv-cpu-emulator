#pragma once
#include "../src/cpu.h"

// Reset the CPU, then put it in the given privilege mode. Tests use this to
// start in U or S-mode without writing mstatus.
static inline void test_cpu_reset_in_mode(CPU *cpu, PrivMode mode) {
	cpu_reset(cpu);
	cpu->privilege_mode = mode;
}
