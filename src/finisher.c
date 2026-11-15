#include "finisher.h"

void finisher_reset(Finisher *f) {
	*f = (Finisher){ 0 };
}

static bool finisher_read(void *ctx, uint32_t offset, uint32_t size, uint32_t *value) {
	(void)ctx;
	(void)size;   // registered for 4-byte accesses only
	if (offset != 0) {
		return false;
	}
	*value = 0;
	return true;
}

static bool finisher_write(void *ctx, uint32_t offset, uint32_t size, uint32_t value) {
	Finisher *f = ctx;
	(void)size;
	if (offset != 0) {
		return false;
	}
	uint32_t status = value & 0xFFFFu;
	if (!f->done && (status == FINISHER_PASS || status == FINISHER_FAIL || status == FINISHER_RESET)) {
		f->done = true;
		f->status = status;
		f->code = value >> 16;
	}
	return true;
}

Device finisher_device(Finisher *f) {
	return (Device){ .base = FINISHER_BASE, .size = FINISHER_SIZE, .sizes = DEV_ACCESS_32,
	                 .read = finisher_read, .write = finisher_write, .ctx = f };
}
