#pragma once
#include <stdint.h>

typedef struct CPU {
	uint32_t regs[32];
	uint32_t pc;
} CPU;

uint32_t cpu_read_reg(CPU *cpu, uint8_t index);
void cpu_write_reg(CPU *cpu, uint8_t index, uint32_t value);

