#include "plic.h"

void plic_reset(Plic *p) {
	*p = (Plic){ 0 };
}

// Reads and writes share the decoding: a register is found by offset, and false means an access fault.
static bool plic_read(void *ctx, uint32_t offset, uint32_t size, uint32_t *value) {
	Plic *p = ctx;
	(void)size;   // the device is registered for 4-byte accesses only
	if (offset >= 4 && offset <= 4 * PLIC_NSOURCES && (offset & 3) == 0) {
		*value = p->priority[offset / 4];
		return true;
	}
	if (offset == PLIC_PENDING) {
		*value = p->pending;
		return true;
	}
	if (offset >= PLIC_ENABLE && offset < PLIC_ENABLE + PLIC_NCONTEXTS * PLIC_ENABLE_STRIDE &&
	    (offset - PLIC_ENABLE) % PLIC_ENABLE_STRIDE == 0) {
		*value = p->enable[(offset - PLIC_ENABLE) / PLIC_ENABLE_STRIDE];
		return true;
	}
	if (offset >= PLIC_CONTEXT && offset < PLIC_CONTEXT + PLIC_NCONTEXTS * PLIC_CONTEXT_STRIDE) {
		uint32_t context = (offset - PLIC_CONTEXT) / PLIC_CONTEXT_STRIDE;
		uint32_t reg = (offset - PLIC_CONTEXT) % PLIC_CONTEXT_STRIDE;
		if (reg == 0) {
			*value = p->threshold[context];
			return true;
		}
		if (reg == 4) {
			*value = 0;   // claim: nothing is ever pending yet
			return true;
		}
	}
	return false;
}

static bool plic_write(void *ctx, uint32_t offset, uint32_t size, uint32_t value) {
	Plic *p = ctx;
	(void)size;
	if (offset >= 4 && offset <= 4 * PLIC_NSOURCES && (offset & 3) == 0) {
		p->priority[offset / 4] = value & PLIC_PRIO_MASK;
		return true;
	}
	if (offset == PLIC_PENDING) {
		return true;   // read-only: the spec does not define a write, so it is ignored
	}
	if (offset >= PLIC_ENABLE && offset < PLIC_ENABLE + PLIC_NCONTEXTS * PLIC_ENABLE_STRIDE &&
	    (offset - PLIC_ENABLE) % PLIC_ENABLE_STRIDE == 0) {
		p->enable[(offset - PLIC_ENABLE) / PLIC_ENABLE_STRIDE] = value & ~1u;   // source 0 does not exist
		return true;
	}
	if (offset >= PLIC_CONTEXT && offset < PLIC_CONTEXT + PLIC_NCONTEXTS * PLIC_CONTEXT_STRIDE) {
		uint32_t context = (offset - PLIC_CONTEXT) / PLIC_CONTEXT_STRIDE;
		uint32_t reg = (offset - PLIC_CONTEXT) % PLIC_CONTEXT_STRIDE;
		if (reg == 0) {
			p->threshold[context] = value & PLIC_PRIO_MASK;
			return true;
		}
		if (reg == 4) {
			return true;   // complete: ignored until the gateway exists
		}
	}
	return false;
}

Device plic_device(Plic *p) {
	return (Device){ .base = PLIC_BASE, .size = PLIC_SIZE, .sizes = DEV_ACCESS_32,
	                 .read = plic_read, .write = plic_write, .ctx = p };
}
