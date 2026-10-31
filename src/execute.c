#include "execute.h"
#include "log.h"

void execute(CPU *cpu, Memory *mem, DecodedInstruction *instr) {
    uint32_t pc = cpu->pc;
    (void)mem;
    switch(instr->opcode) {
        case 0x13: // I type ALU group

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
                    {
                        uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
                        cpu_write_reg(cpu, instr->rd, (rs1_val < (uint32_t)instr->imm) ? 1 : 0);
                        cpu->pc = pc + 4;
                        break;
                    }

                case 0x04: // XORI
                    {
                        uint32_t rs1_val = cpu_read_reg(cpu,instr->rs1);
                        cpu_write_reg(cpu, instr->rd, rs1_val ^ (uint32_t)instr->imm);
                        cpu->pc = pc + 4;
                        break;
                    }

                case 0x06: // ORI
                    {
                        uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
                        cpu_write_reg(cpu, instr->rd, rs1_val | (uint32_t)instr->imm);
                        cpu->pc = pc + 4;
                        break;
                    }

                case 0x07: // ANDI
                    {
                        uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
                        cpu_write_reg(cpu, instr->rd, rs1_val & (uint32_t)instr->imm);
                        cpu-> pc = pc + 4;
                        break;
                    }

                case 0x1: // SLLI
                    {
                        uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
                        cpu_write_reg(cpu, instr->rd, rs1_val << instr->rs2);
                        cpu->pc = pc + 4;
                        break;
                    }

                case 0x05: // SRLI or SRAI
                    {
                        uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
                        if(instr->funct7 == 0x00) { // SRLI case
                            cpu_write_reg(cpu, instr->rd, rs1_val >> instr->rs2);
                        } else { // SRAI (funct7 == 0x20)
                            uint32_t shifted = rs1_val >> instr->rs2;       // unsigned shift: standard-guaranteed zero-fill
                            if (rs1_val & 0x80000000) {                     // original sign bit was set
                                shifted |= ~(0xFFFFFFFFu >> instr->rs2);    // manually sign-fill the vacated top bits
                            }
                            cpu_write_reg(cpu, instr->rd, shifted);
                        }
                        cpu->pc = pc + 4;
                        break;
                    }

                default:
                        LOG_FATAL("unimplemented funct3 for opcode 0x13: funct3=0x%x", instr->funct3);
                break;
            }
        break;

        case 0x33:
            switch(instr->funct3) {
                case 0x0: // ADD or SUB
                {   
                    uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
                    uint32_t rs2_val = cpu_read_reg(cpu, instr->rs2);
                    if(instr->funct7 == 0x00) {
                        cpu_write_reg(cpu, instr->rd, rs1_val + rs2_val); // ADD
                    } else {
                        cpu_write_reg(cpu, instr->rd, rs1_val - rs2_val); // SUB
                    }
                    cpu->pc = pc + 4;
                    break;
                }
                case 0x1: // SLL
                {
                    uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
                    uint32_t rs2_val = cpu_read_reg(cpu, instr->rs2);
                    cpu_write_reg(cpu, instr->rd, rs1_val << (rs2_val & 0x1F));
                    cpu->pc = pc + 4;
                    break;
                }
                case 0x2: // SLT
                {
                    uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
                    uint32_t rs2_val = cpu_read_reg(cpu, instr->rs2);
                    cpu_write_reg(cpu, instr->rd, ((int32_t)rs1_val < (int32_t)rs2_val) ? 1 : 0);
                    cpu->pc = pc + 4;
                    break;
                }
                case 0x3: // SLTU
                {
                    uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
                    uint32_t rs2_val = cpu_read_reg(cpu, instr->rs2);
                    cpu_write_reg(cpu, instr->rd, (rs1_val < rs2_val) ? 1 : 0);
                    cpu->pc = pc + 4;
                    break;
                }
                case 0x4: // XOR
                {
                    uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
                    uint32_t rs2_val = cpu_read_reg(cpu, instr->rs2);
                    cpu_write_reg(cpu, instr->rd, rs1_val ^ rs2_val);
                    cpu->pc = pc + 4;
                    break;
                }
            default:
                    LOG_FATAL("unimplemented funct3 for opcode 0x33: funct3=0x%x", instr->funct3);
            break;
            }
        break;

        default:
            LOG_FATAL("unimplemented instruction: opcode=0x%x funct3=0x%x", instr->opcode, instr->funct3);

    }
}