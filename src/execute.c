#include "execute.h"
#include "log.h"

void execute(CPU *cpu, Memory *mem, DecodedInstruction *instr) {
    uint32_t pc = cpu->pc;
    (void)mem;
    switch(instr->opcode) {
        case 0x13:

            switch(instr->funct3) {
                case 0x000: //ADDI
                    {
                        uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
                        cpu_write_reg(cpu, instr->rd, rs1_val + instr->imm);
                        cpu->pc = pc + 4;
                        break;
                    }
                case 0x02: // SLTI
                    {
                        uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
                        cpu_write_reg(cpu, instr->rd, ((int32_t)rs1_val < instr->imm) ? 1 : 0);
                        cpu->pc = pc + 4;
                        break;
                    }

                case 0x03: // SLTIU
                    LOG_FATAL("SLTIU not implemented yet");
                break;

                case 0x04: // XORI
                    LOG_FATAL("XORI not implemented yet");
                break;

                case 0x06: // ORI
                    LOG_FATAL("ORI not implemented yet");
                break;

                case 0x07: // ANDI
                    LOG_FATAL("ANDI not implemented yet");
                break;

                case 0x1: // SLLI
                    LOG_FATAL("SLLI not implemented yet");
                break;

                case 0x05: // SRLI or SRAI
                    LOG_FATAL("SRLI or SRAI not implemented yet");
                break;
                
                default:
                        LOG_FATAL("unimplemented funct3 for opcode 0x13: funct3=0x%x", instr->funct3);
                break;
            }
        break;
        default:
            LOG_FATAL("unimplemented instruction: opcode=0x%x funct3=0x%x", instr->opcode, instr->funct3);

    }
}