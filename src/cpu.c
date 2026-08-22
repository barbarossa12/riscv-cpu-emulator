#include <stdlib.h>
#include <stdint.h>
#include "log.h"
#include "cpu.h"
uint32_t cpu_read_reg(CPU *cpu, uint8_t index) {
	if(index > 31) {
		LOG_FATAL("CPU read register value larger than 32: %d", index);
	}

	if(index == 0) {
		return (uint32_t) 0;
	}

	return cpu->regs[index];

}

void cpu_write_reg(CPU *cpu, uint8_t index, uint32_t value) {
	if(index > 31) {
		LOG_FATAL("CPU write register value larger than 32: %d", index);
	}

	if(index == 0) {
		return;
	}

	cpu->regs[index] = value;
}

