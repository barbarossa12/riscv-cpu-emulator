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
	LOG_DEBUG("Memory freed!");
}

/* True if all n bytes starting at addr lie inside the buffer.
 * The sum is done in 64 bits so addr + n can't wrap around 2^32. */
static bool mem_in_bounds(const Memory *mem, uint32_t addr, uint32_t n) {
	return (uint64_t)addr + n <= mem->size;
}

/*memory read functions*/
uint8_t mem_read8(Memory *mem, uint32_t addr) {
	if(!mem_in_bounds(mem, addr, 1)) {
		LOG_FATAL("out-of-bound read at address 0x%08x (buffer size %zu)", addr, mem->size);
	}
	return mem->buffer[addr];
}

uint16_t mem_read16(Memory *mem, uint32_t addr) {
	if(!mem_in_bounds(mem, addr, 2)) {
		LOG_FATAL("out-of-bound read at address 0x%08x (buffer size %zu)", addr, mem->size);
	}
	uint16_t result = (mem->buffer[addr+1] << 8);
	result |= mem->buffer[addr];
	return result;
}

uint32_t mem_read32(Memory *mem, uint32_t addr) {
	if(!mem_in_bounds(mem, addr, 4)) {
		LOG_FATAL("out-of-bound read at address 0x%08x (buffer size %zu)", addr, mem->size);
	}
	uint32_t result = (uint32_t)(mem->buffer[addr + 3]) << 24;
	result |= (uint32_t)(mem->buffer[addr + 2]) << 16;
	result |= (uint32_t)(mem->buffer[addr + 1]) << 8;
	result |= (uint32_t)(mem->buffer[addr]);
	return result;
}

/*memory write functions*/
void mem_write8(Memory *mem, uint32_t addr, uint8_t val) {
	if(!mem_in_bounds(mem, addr, 1)) {
		LOG_FATAL("out-of-bound write at address 0x%08x (buffer size %zu)", addr, mem->size);
	}
	mem->buffer[addr] = val;
}

void mem_write16(Memory *mem, uint32_t addr, uint16_t val) {
	if(!mem_in_bounds(mem, addr, 2)) {
		LOG_FATAL("out-of-bound write at address 0x%08x (buffer size %zu)", addr, mem->size);
	}
	mem->buffer[addr] = val & 0xFF;
	mem->buffer[addr + 1] = (val >> 8) & 0xFF;
}

void mem_write32(Memory *mem, uint32_t addr, uint32_t val) {
	if(!mem_in_bounds(mem, addr, 4)) {
		LOG_FATAL("out-of-bound write at address 0x%08x (buffer size %zu)", addr, mem->size);
	}
	mem->buffer[addr] = val & 0xFF;
	mem->buffer[addr + 1] = (val >> 8) & 0xFF;
	mem->buffer[addr + 2] = (val >> 16) & 0xFF;
	mem->buffer[addr + 3] = (val >> 24) & 0xFF;
}

bool mem_try_read8(Memory *mem, uint32_t addr, uint8_t *out) {
    if (!mem_in_bounds(mem, addr, 1)) {
        return false;
    }
    *out = mem->buffer[addr];
    return true;
}

bool mem_try_write8(Memory *mem, uint32_t addr, uint8_t val) {
    if (!mem_in_bounds(mem, addr, 1)) {
        return false;
    }
    mem->buffer[addr] = val;
    return true;
}

bool mem_try_read16(Memory *mem, uint32_t addr, uint16_t *out) {
    if (!mem_in_bounds(mem, addr, 2)) {
        return false;
    }
    *out = (uint16_t)(mem->buffer[addr] |
                      (mem->buffer[addr + 1] << 8));
    return true;
}

bool mem_try_read32(Memory *mem, uint32_t addr, uint32_t *out) {
    if (!mem_in_bounds(mem, addr, 4)) {
        return false;
    }
    *out = (uint32_t)mem->buffer[addr] |
           ((uint32_t)mem->buffer[addr + 1] << 8) |
           ((uint32_t)mem->buffer[addr + 2] << 16) |
           ((uint32_t)mem->buffer[addr + 3] << 24);
    return true;
}

bool mem_try_write16(Memory *mem, uint32_t addr, uint16_t val) {
    if (!mem_in_bounds(mem, addr, 2)) {
        return false;
    }
    mem->buffer[addr]     = (uint8_t)(val & 0xFF);
    mem->buffer[addr + 1] = (uint8_t)(val >> 8);
    return true;
}

bool mem_try_write32(Memory *mem, uint32_t addr, uint32_t val) {
    if (!mem_in_bounds(mem, addr, 4)) {
        return false;
    }
    mem->buffer[addr]     = (uint8_t)(val & 0xFF);
    mem->buffer[addr + 1] = (uint8_t)((val >> 8) & 0xFF);
    mem->buffer[addr + 2] = (uint8_t)((val >> 16) & 0xFF);
    mem->buffer[addr + 3] = (uint8_t)((val >> 24) & 0xFF);
    return true;
}