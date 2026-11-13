#include "../src/mem.h"

static bool dummy_read(void *ctx, uint32_t offset, uint32_t size, uint32_t *value) {
    (void)ctx; (void)offset; (void)size; *value = 0;
    return true;
}

static Device device_at(uint32_t base, uint32_t size) {
    return (Device){ .base = base, .size = size, .sizes = DEV_ACCESS_32, .read = dummy_read };
}

int main() {
    Memory mem;
    mem_init_at(&mem, 0x80000000u, 0x1000);
    // a device with no registers
    mem_add_device(&mem, device_at(0x02000000u, 0));
    return 0;
}
