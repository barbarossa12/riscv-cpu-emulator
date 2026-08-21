#include "../src/mem.h"
#include "../src/log.h"


int main() {
    Memory testMemory;
    mem_init(&testMemory, 128);

    // deliberately read past the 128 limit
    mem_read8(&testMemory, 128);

    return 0;
}
