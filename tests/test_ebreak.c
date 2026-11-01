#include "../src/decode.h"
#include "../src/execute.h"

int main() {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    mem_write32(&mem, 0, 0x00100073); // EBREAK

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    return 0;
}
