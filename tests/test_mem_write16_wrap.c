#include "../src/mem.h"
#include "../src/log.h"


int main() {
    Memory testMemory;
    mem_init(&testMemory, 128);

    // addr + 1 wraps around 2^32 back to 0, which a 32-bit bounds check lets through
    mem_write16(&testMemory, 0xFFFFFFFF, 0x1234);

    return 0;
}
