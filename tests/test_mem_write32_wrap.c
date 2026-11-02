#include "../src/mem.h"
#include "../src/log.h"


int main() {
    Memory testMemory;
    mem_init(&testMemory, 128);

    // addr + 3 wraps around 2^32 back to 0, which a 32-bit bounds check lets through
    mem_write32(&testMemory, 0xFFFFFFFD, 0x12345678);

    return 0;
}
