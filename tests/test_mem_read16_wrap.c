#include "../src/mem.h"
#include "../src/log.h"


int main() {
    Memory testMemory;
    mem_init(&testMemory, 128);

    // addr + 2 wraps around 2^32 back to 1, which a 32-bit bounds check lets through
    mem_read16(&testMemory, 0xFFFFFFFF);

    return 0;
}
