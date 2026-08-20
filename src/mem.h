#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct Memory {
	uint8_t *buffer;
	size_t size;
}Memory;

void mem_init(Memory *mem, size_t size);
void mem_free(Memory *mem);

/*memory read functions*/
uint8_t mem_read8(Memory *mem, uint32_t addr);
uint16_t mem_read16(Memory *mem, uint32_t addr);
uint32_t mem_read32(Memory *mem, uint32_t addr);

/*memory write functions*/
void mem_write8(Memory *mem, uint32_t addr, uint8_t data);
void mem_write16(Memory *mem, uint32_t addr, uint16_t data);
void mem_write32(Memory *mem, uint32_t addr, uint16_t data);

