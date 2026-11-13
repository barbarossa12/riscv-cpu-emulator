#include "clint.h"

void clint_reset(Clint *c) {
	*c = (Clint){ .mtime = 0, .mtimecmp = UINT64_MAX, .msip = 0, .phase = 0 };
}

void clint_tick(Clint *c) {
	if (++c->phase >= CLINT_CYCLES_PER_TICK) {
		c->phase = 0;
		c->mtime++;   // wraps around on overflow (machine.adoc:2516)
	}
}

bool clint_idle_until_timer(Clint *c) {
	if (c->mtimecmp == UINT64_MAX || c->mtime >= c->mtimecmp) {
		return false;
	}
	c->mtime = c->mtimecmp;
	c->phase = 0;
	return true;
}

bool clint_timer_pending(const Clint *c) {
	return c->mtime >= c->mtimecmp;
}

bool clint_software_pending(const Clint *c) {
	return c->msip & 1u;
}

// Only the five 32-bit registers exist. Every other offset, including the slots a second hart would use,
// is an access fault (decision 2026-11-14), which the bus reports when a callback returns false.
static bool clint_read(void *ctx, uint32_t offset, uint32_t size, uint32_t *value) {
	Clint *c = ctx;
	(void)size;   // the device is registered for 4-byte accesses only
	switch (offset) {
		case CLINT_MSIP:         *value = c->msip & 1u;                       return true;
		case CLINT_MTIMECMP:     *value = (uint32_t)c->mtimecmp;              return true;
		case CLINT_MTIMECMP + 4: *value = (uint32_t)(c->mtimecmp >> 32);      return true;
		case CLINT_MTIME:        *value = (uint32_t)c->mtime;                 return true;
		case CLINT_MTIME + 4:    *value = (uint32_t)(c->mtime >> 32);         return true;
		default:                 return false;
	}
}

// On RV32 a memory-mapped write changes one 32-bit half (machine.adoc:2575)
static bool clint_write(void *ctx, uint32_t offset, uint32_t size, uint32_t value) {
	Clint *c = ctx;
	(void)size;
	switch (offset) {
		case CLINT_MSIP:         c->msip = value & 1u;                                                  return true;
		case CLINT_MTIMECMP:     c->mtimecmp = (c->mtimecmp & ~0xFFFFFFFFull) | value;                  return true;
		case CLINT_MTIMECMP + 4: c->mtimecmp = (c->mtimecmp & 0xFFFFFFFFull) | ((uint64_t)value << 32); return true;
		case CLINT_MTIME:        c->mtime = (c->mtime & ~0xFFFFFFFFull) | value;                        return true;
		case CLINT_MTIME + 4:    c->mtime = (c->mtime & 0xFFFFFFFFull) | ((uint64_t)value << 32);       return true;
		default:                 return false;
	}
}

Device clint_device(Clint *c) {
	return (Device){ .base = CLINT_BASE, .size = CLINT_SIZE, .sizes = DEV_ACCESS_32,
	                 .read = clint_read, .write = clint_write, .ctx = c };
}
