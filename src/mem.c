#include "log.h"
#include "mem.h"

void mem_init(Memory *mem, size_t size) {
	mem_init_at(mem, 0, size);
}

void mem_init_at(Memory *mem, uint32_t base, size_t size) {
	mem->buffer = (uint8_t*)calloc(size, 1); // zeroed, so unwritten memory reads as 0
	if(mem->buffer) {
		mem->size = size;
		mem->base = base;
		mem->device_count = 0;
	} else {
		LOG_FATAL("Failed to allocate memory: calloc failed");
	}
}

void mem_free(Memory *mem) {
	free(mem->buffer);
	mem->buffer = NULL;
	mem->size = 0;
	mem->base = 0;
	mem->device_count = 0;
	LOG_DEBUG("Memory freed!");
}

/* True if all n bytes starting at addr lie inside the region [base, base + size).
 * Done in 64 bits so neither the subtraction nor the sum can wrap around 2^32. */
static bool mem_in_bounds(const Memory *mem, uint32_t addr, uint32_t n) {
	return addr >= mem->base && (uint64_t)(addr - mem->base) + n <= mem->size;
}

/*memory read functions*/
uint8_t mem_read8(Memory *mem, uint32_t addr) {
	if(!mem_in_bounds(mem, addr, 1)) {
		LOG_FATAL("out-of-bound read at address 0x%08x (buffer size %zu)", addr, mem->size);
	}
	return mem->buffer[addr - mem->base];
}

uint16_t mem_read16(Memory *mem, uint32_t addr) {
	if(!mem_in_bounds(mem, addr, 2)) {
		LOG_FATAL("out-of-bound read at address 0x%08x (buffer size %zu)", addr, mem->size);
	}
	uint16_t result = (mem->buffer[addr - mem->base + 1] << 8);
	result |= mem->buffer[addr - mem->base];
	return result;
}

uint32_t mem_read32(Memory *mem, uint32_t addr) {
	if(!mem_in_bounds(mem, addr, 4)) {
		LOG_FATAL("out-of-bound read at address 0x%08x (buffer size %zu)", addr, mem->size);
	}
	uint32_t result = (uint32_t)(mem->buffer[addr - mem->base + 3]) << 24;
	result |= (uint32_t)(mem->buffer[addr - mem->base + 2]) << 16;
	result |= (uint32_t)(mem->buffer[addr - mem->base + 1]) << 8;
	result |= (uint32_t)(mem->buffer[addr - mem->base]);
	return result;
}

/*memory write functions*/
void mem_write8(Memory *mem, uint32_t addr, uint8_t val) {
	if(!mem_in_bounds(mem, addr, 1)) {
		LOG_FATAL("out-of-bound write at address 0x%08x (buffer size %zu)", addr, mem->size);
	}
	mem->buffer[addr - mem->base] = val;
}

void mem_write16(Memory *mem, uint32_t addr, uint16_t val) {
	if(!mem_in_bounds(mem, addr, 2)) {
		LOG_FATAL("out-of-bound write at address 0x%08x (buffer size %zu)", addr, mem->size);
	}
	mem->buffer[addr - mem->base] = val & 0xFF;
	mem->buffer[addr - mem->base + 1] = (val >> 8) & 0xFF;
}

void mem_write32(Memory *mem, uint32_t addr, uint32_t val) {
	if(!mem_in_bounds(mem, addr, 4)) {
		LOG_FATAL("out-of-bound write at address 0x%08x (buffer size %zu)", addr, mem->size);
	}
	mem->buffer[addr - mem->base] = val & 0xFF;
	mem->buffer[addr - mem->base + 1] = (val >> 8) & 0xFF;
	mem->buffer[addr - mem->base + 2] = (val >> 16) & 0xFF;
	mem->buffer[addr - mem->base + 3] = (val >> 24) & 0xFF;
}

bool mem_try_read8(Memory *mem, uint32_t addr, uint8_t *out) {
    if (!mem_in_bounds(mem, addr, 1)) {
        return false;
    }
    *out = mem->buffer[addr - mem->base];
    return true;
}

bool mem_try_write8(Memory *mem, uint32_t addr, uint8_t val) {
    if (!mem_in_bounds(mem, addr, 1)) {
        return false;
    }
    mem->buffer[addr - mem->base] = val;
    return true;
}

bool mem_try_read16(Memory *mem, uint32_t addr, uint16_t *out) {
    if (!mem_in_bounds(mem, addr, 2)) {
        return false;
    }
    *out = (uint16_t)(mem->buffer[addr - mem->base] |
                      (mem->buffer[addr - mem->base + 1] << 8));
    return true;
}

bool mem_try_read32(Memory *mem, uint32_t addr, uint32_t *out) {
    if (!mem_in_bounds(mem, addr, 4)) {
        return false;
    }
    *out = (uint32_t)mem->buffer[addr - mem->base] |
           ((uint32_t)mem->buffer[addr - mem->base + 1] << 8) |
           ((uint32_t)mem->buffer[addr - mem->base + 2] << 16) |
           ((uint32_t)mem->buffer[addr - mem->base + 3] << 24);
    return true;
}

bool mem_try_write16(Memory *mem, uint32_t addr, uint16_t val) {
    if (!mem_in_bounds(mem, addr, 2)) {
        return false;
    }
    mem->buffer[addr - mem->base]     = (uint8_t)(val & 0xFF);
    mem->buffer[addr - mem->base + 1] = (uint8_t)(val >> 8);
    return true;
}

bool mem_try_write32(Memory *mem, uint32_t addr, uint32_t val) {
    if (!mem_in_bounds(mem, addr, 4)) {
        return false;
    }
    mem->buffer[addr - mem->base]     = (uint8_t)(val & 0xFF);
    mem->buffer[addr - mem->base + 1] = (uint8_t)((val >> 8) & 0xFF);
    mem->buffer[addr - mem->base + 2] = (uint8_t)((val >> 16) & 0xFF);
    mem->buffer[addr - mem->base + 3] = (uint8_t)((val >> 24) & 0xFF);
    return true;
}

/* ---- the bus ---------------------------------------------------------------------------------- */

bool mem_in_ram(const Memory *mem, uint32_t addr, uint32_t n) {
	return mem_in_bounds(mem, addr, n);
}

// The device that holds all size bytes starting at addr, or NULL.
static const Device *device_for(const Memory *mem, uint32_t addr, uint32_t size) {
	for (int i = 0; i < mem->device_count; i++) {
		const Device *d = &mem->devices[i];
		if (addr >= d->base && (uint64_t)(addr - d->base) + size <= d->size) {
			return d;
		}
	}
	return NULL;
}

// A device takes only naturally aligned accesses of a width it lists
static bool device_takes(const Device *d, uint32_t addr, uint32_t size) {
	uint32_t width = size == 1 ? DEV_ACCESS_8 : size == 2 ? DEV_ACCESS_16 : size == 4 ? DEV_ACCESS_32 : 0;
	return width && (d->sizes & width) && addr % size == 0;
}

void mem_add_device(Memory *mem, Device dev) {
	if (dev.size == 0 || (uint64_t)dev.base + dev.size > 0x100000000ull) {
		LOG_FATAL("device at 0x%08x has a bad size 0x%x", dev.base, dev.size);
	}
	if (mem->device_count >= MEM_MAX_DEVICES) {
		LOG_FATAL("too many devices (at most %d)", MEM_MAX_DEVICES);
	}
	uint64_t lo = dev.base, hi = lo + dev.size;
	if (lo < (uint64_t)mem->base + mem->size && (uint64_t)mem->base < hi) {
		LOG_FATAL("device at 0x%08x overlaps RAM", dev.base);
	}
	for (int i = 0; i < mem->device_count; i++) {
		uint64_t other_lo = mem->devices[i].base, other_hi = other_lo + mem->devices[i].size;
		if (lo < other_hi && other_lo < hi) {
			LOG_FATAL("device at 0x%08x overlaps the device at 0x%08x", dev.base, mem->devices[i].base);
		}
	}
	mem->devices[mem->device_count++] = dev;
}

bool mem_bus_read(Memory *mem, uint32_t addr, uint32_t size, uint32_t *out) {
	if (mem_in_bounds(mem, addr, size)) {
		uint32_t value = 0;
		for (uint32_t i = 0; i < size; i++) {
			value |= (uint32_t)mem->buffer[addr - mem->base + i] << (8 * i);
		}
		*out = value;
		return true;
	}
	const Device *d = device_for(mem, addr, size);
	if (!d || !d->read || !device_takes(d, addr, size)) {
		return false;
	}
	return d->read(d->ctx, addr - d->base, size, out);
}

bool mem_bus_write(Memory *mem, uint32_t addr, uint32_t size, uint32_t val) {
	if (mem_in_bounds(mem, addr, size)) {
		for (uint32_t i = 0; i < size; i++) {
			mem->buffer[addr - mem->base + i] = (uint8_t)(val >> (8 * i));
		}
		return true;
	}
	const Device *d = device_for(mem, addr, size);
	if (!d || !d->write || !device_takes(d, addr, size)) {
		return false;
	}
	return d->write(d->ctx, addr - d->base, size, val);
}

bool mem_bus_writable(Memory *mem, uint32_t addr, uint32_t size) {
	if (mem_in_bounds(mem, addr, size)) {
		return true;
	}
	const Device *d = device_for(mem, addr, size);
	return d && d->write && device_takes(d, addr, size);
}
