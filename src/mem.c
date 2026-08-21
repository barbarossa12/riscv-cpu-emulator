#include "log.h"
#include "mem.h"

void mem_init(Memory *mem, size_t size) {
	mem->buffer = (uint8_t*)malloc(size);
	if(mem->buffer) {
		mem->size = size;
	} else {
		LOG_FATAL("Failed to allocate memory: malloc falied");
	}
}

void mem_free(Memory *mem) {
	free(mem->buffer);
	mem->buffer = NULL;
	mem->size = 0;
	LOG_INFO("Memory freed!");
}

/*memory read functions*/
uint8_t mem_read8(Memory *mem, uint32_t addr) {
	if(addr >= mem->size) {
		LOG_FATAL("out-of-bound read at address 0x%08x (buffer size %zu)", addr, mem->size);
	}
	return mem->buffer[addr];
}

uint16_t mem_read16(Memory *mem, uint32_t addr) {
	if(addr + 1 >= mem->size) {
		LOG_FATAL("out-of-bound read at address 0x%08x (buffer size %zu)", addr, mem->size);
	}
	uint16_t result = (mem->buffer[addr+1] << 8);
	result |= mem->buffer[addr];
	return result;
}

uint32_t mem_read32(Memory *mem, uint32_t addr) {
	if(addr + 3 >= mem->size) {
		LOG_FATAL("out-of-bound read at address 0x%08x (buffer size %zu)", addr, mem->size);
	}
	uint32_t result = (mem->buffer[addr + 3]) << 24;
	result |= (mem->buffer[addr + 2]) << 16;
	result |= (mem->buffer[addr + 1]) << 8;
	result |= (mem->buffer[addr]);
	return result;
}

/*memory write functions*/
void mem_write8(Memory *mem, uint32_t addr, uint8_t val) {
	if(addr >= mem->size) {
		LOG_FATAL("out-of-bound write at address 0x%08x (buffer size %zu)", addr, mem->size);
	}
	mem->buffer[addr] = val;
}

void mem_write16(Memory *mem, uint32_t addr, uint16_t val) {
	if(addr + 1 >= mem->size) {
		LOG_FATAL("out-of-bound write at address 0x%08x (buffer size %zu)", addr, mem->size);
	}
	mem->buffer[addr] = val & 0xFF;
	mem->buffer[addr + 1] = (val >> 8) & 0xFF;
}

void mem_write32(Memory *mem, uint32_t addr, uint32_t val) {
	if(addr + 3 >=  mem->size) {
		LOG_FATAL("out-of-bound write at address 0x%08x (buffer size %zu)", addr, mem->size);
	}
	mem->buffer[addr] = val & 0xFF;
	mem->buffer[addr + 1] = (val >> 8) & 0xFF;
	mem->buffer[addr + 2] = (val >> 16) & 0xFF;
	mem->buffer[addr + 3] = (val >> 24) & 0xFF;
}

