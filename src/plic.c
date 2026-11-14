#include "plic.h"
#include <string.h>
#include "log.h"

void plic_reset(Plic *p) {
	PlicLine line[PLIC_NSOURCES + 1];
	void *line_ctx[PLIC_NSOURCES + 1];
	memcpy(line, p->line, sizeof line);
	memcpy(line_ctx, p->line_ctx, sizeof line_ctx);
	*p = (Plic){ 0 };
	memcpy(p->line, line, sizeof line);
	memcpy(p->line_ctx, line_ctx, sizeof line_ctx);
}

void plic_set_source(Plic *p, uint32_t id, PlicLine line, void *ctx) {
	if (id < 1 || id > PLIC_NSOURCES) {
		LOG_FATAL("PLIC source id %u out of range 1..%u", id, PLIC_NSOURCES);
	}
	p->line[id] = line;
	p->line_ctx[id] = ctx;
}

// The gateways of level-triggered sources: the first assertion becomes one request, and no further request is
// forwarded until the source is completed (PLIC spec, "Interrupt Gateways"). A request cannot be retracted, so a
// line that drops before the claim leaves the pending bit set.
static void plic_sample(Plic *p) {
	for (uint32_t id = 1; id <= PLIC_NSOURCES; id++) {
		uint32_t bit = 1u << id;
		if (p->line[id] && !(p->in_flight & bit) && !(p->pending & bit) && p->line[id](p->line_ctx[id])) {
			p->pending |= bit;
		}
	}
}

// The source a claim would return: pending, enabled for the context and with priority above 0 ("never interrupt"),
// the highest priority first and the lowest ID on a tie. 0 means none. The threshold does not apply to a claim.
static uint32_t plic_best(const Plic *p, uint32_t context) {
	uint32_t best = 0;
	for (uint32_t id = 1; id <= PLIC_NSOURCES; id++) {
		if (((p->pending >> id) & 1u) && ((p->enable[context] >> id) & 1u) && p->priority[id] > 0 &&
		    (best == 0 || p->priority[id] > p->priority[best])) {
			best = id;
		}
	}
	return best;
}

bool plic_context_pending(Plic *p, uint32_t context) {
	plic_sample(p);
	uint32_t id = plic_best(p, context);
	return id != 0 && p->priority[id] > p->threshold[context];
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
		plic_sample(p);
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
			plic_sample(p);   // a claim is legal at any time, and returns 0 when nothing is eligible
			uint32_t id = plic_best(p, context);
			if (id) {
				p->pending &= ~(1u << id);
				p->in_flight |= 1u << id;
			}
			*value = id;
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
			// complete: only an ID that is enabled for this context counts, and it need not be the one claimed
			if (value >= 1 && value <= PLIC_NSOURCES && ((p->enable[context] >> value) & 1u)) {
				p->in_flight &= ~(1u << value);
			}
			return true;
		}
	}
	return false;
}

Device plic_device(Plic *p) {
	return (Device){ .base = PLIC_BASE, .size = PLIC_SIZE, .sizes = DEV_ACCESS_32,
	                 .read = plic_read, .write = plic_write, .ctx = p };
}
