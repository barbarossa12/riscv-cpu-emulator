#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

// A memory-mapped device. Its registers are reached by loads and stores to [base, base + size).
// A device is an I/O region: it takes only naturally aligned accesses of the widths it lists, no
// instruction fetches, no atomics and no page-table accesses, and its reads and writes may have side
// effects, so they are never repeated or probed.
#define DEV_ACCESS_8  (1u << 0)
#define DEV_ACCESS_16 (1u << 1)
#define DEV_ACCESS_32 (1u << 2)
#define MEM_MAX_DEVICES 8

typedef struct Device {
	uint32_t base;
	uint32_t size;
	uint32_t sizes;   // DEV_ACCESS_* widths accepted
	// offset is relative to base. False means the access faults. A NULL callback means no such access.
	bool (*read)(void *ctx, uint32_t offset, uint32_t size, uint32_t *value);
	bool (*write)(void *ctx, uint32_t offset, uint32_t size, uint32_t value);
	void *ctx;
} Device;

typedef struct Memory {
	uint8_t *buffer;
	size_t size;
	uint32_t base; // guest address of buffer[0]
	Device devices[MEM_MAX_DEVICES];
	int device_count;
}Memory;

void mem_init(Memory *mem, size_t size); // base = 0
void mem_init_at(Memory *mem, uint32_t base, size_t size);
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

/*the bus: physical accesses that reach RAM or a device. They return false for an access fault.
Only the mem_try_* functions above are RAM-only, which is what instruction fetch, page-table walks
and the atomics use, so those fault on a device without extra checks.*/
void mem_add_device(Memory *mem, Device dev); // aborts if it overlaps RAM or another device
bool mem_in_ram(const Memory *mem, uint32_t addr, uint32_t n); // all n bytes are in RAM
bool mem_bus_read(Memory *mem, uint32_t addr, uint32_t size, uint32_t *out);   // size 1 to 4
bool mem_bus_write(Memory *mem, uint32_t addr, uint32_t size, uint32_t val);   // size 1 to 4
bool mem_bus_writable(Memory *mem, uint32_t addr, uint32_t size); // like a write, but touches nothing
