#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

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
void mem_write8(Memory *mem, uint32_t addr, uint8_t val);
void mem_write16(Memory *mem, uint32_t addr, uint16_t val);
void mem_write32(Memory *mem, uint32_t addr, uint32_t val);

/*memory read write check variants return false instead
of aborting an out of bound access*/
bool mem_try_read8(Memory *mem, uint32_t addr, uint8_t *out);
bool mem_try_read16(Memory *mem, uint32_t addr, uint16_t *out);
bool mem_try_read32(Memory *mem, uint32_t addr, uint32_t *out);
bool mem_try_write8(Memory *mem, uint32_t addr, uint8_t val);
bool mem_try_write16(Memory *mem, uint32_t addr, uint16_t val);
bool mem_try_write32(Memory *mem, uint32_t addr, uint32_t val);

