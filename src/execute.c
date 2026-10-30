#include "execute.h"
#include "log.h"

void execute(CPU *cpu, Memory *mem, DecodedInstruction *instr) {
    uint32_t pc = cpu->pc;
    (void)mem;
    switch(instr->opcode) {
        case 0x13: // ADDI
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            cpu_write_reg(cpu, instr->rd, rs1_val + instr->imm);
            cpu->pc = pc + 4;
            break;
        default:
            LOG_DEBUG("entered the default state");
    }
}