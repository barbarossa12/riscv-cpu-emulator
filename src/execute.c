#include "execute.h"
#include "log.h"

void execute(CPU *cpu, Memory *mem, DecodedInstruction *instr) {
    uint32_t pc = cpu->pc;
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
                case 0x5: // SRL or SRA
                {
                    uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
                    uint32_t rs2_val = cpu_read_reg(cpu, instr->rs2);
                    uint32_t shamt = rs2_val & 0x1F;
                    if(instr->funct7 == 0x00) {  // SRL
                        cpu_write_reg(cpu, instr->rd, rs1_val >> shamt);
                    } else {
                        uint32_t shifted = rs1_val >> shamt;
                        if(rs1_val & 0x80000000) {
                            shifted |= ~(0xFFFFFFFFu >> shamt);
                        }
                        cpu_write_reg(cpu, instr->rd, shifted);
                    }
                    cpu->pc = pc + 4;
                    break;
                }
                case 0x6:
                {
                    uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
                    uint32_t rs2_val = cpu_read_reg(cpu, instr->rs2);
                    cpu_write_reg(cpu, instr->rd, rs1_val | rs2_val);
                    cpu->pc = pc + 4;
                    break;
                }
                case 0x7:
                {
                    uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
                    uint32_t rs2_val = cpu_read_reg(cpu, instr->rs2);
                    cpu_write_reg(cpu, instr->rd, rs1_val & rs2_val);
                    cpu->pc = pc + 4;
                    break;
                }
            default:
                    LOG_FATAL("unimplemented funct3 for opcode 0x33: funct3=0x%x", instr->funct3);
            break;
            }
        break;

        case 0x37: // LUI
            {
                cpu_write_reg(cpu, instr->rd, (uint32_t)instr->imm);
                cpu->pc = pc + 4;
                break;
            }

        case 0x17: // AUIPC
        {
            cpu_write_reg(cpu, instr->rd, pc + (uint32_t)instr->imm);
            cpu->pc = pc + 4;
            break;
        }

        case 0x03:
        {
            switch(instr->funct3) {
                case 0x0: // LB
                    {
                        uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
                        uint32_t addr = rs1_val + (uint32_t)instr->imm;
                        int8_t byte = (int8_t)mem_read8(mem, addr);
                        cpu_write_reg(cpu, instr->rd, (uint32_t)(int32_t)byte);  // sign-extend
                        cpu->pc = pc + 4;
                        break;
                    }
                case 0x1: // LH
                    {
                        uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
                        uint32_t addr = rs1_val + (uint32_t)instr->imm;
                        int16_t half = (int16_t)mem_read16(mem, addr);
                        cpu_write_reg(cpu, instr->rd, (uint32_t)(int32_t)half); // sign extended
                        cpu->pc = pc+4;
                        break;
                    }
                case 0x2: // LW
                {
                    uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
                    uint32_t addr = rs1_val + (uint32_t)instr->imm;
                    uint32_t word = mem_read32(mem, addr);
                    cpu_write_reg(cpu, instr->rd, word);
                    cpu->pc = pc + 4;
                    break;
                }
                case 0x4: // LBU
                    {
                        uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
                        uint32_t addr = rs1_val + (uint32_t)instr->imm;
                        uint8_t byte = mem_read8(mem, addr);
                        cpu_write_reg(cpu, instr->rd, (uint32_t)byte);           // zero-extend
                        cpu->pc = pc + 4;
                        break;
                    }
                case 0x5: // LHU
                    {
                        uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
                        uint32_t addr = rs1_val + (uint32_t)instr->imm;
                        uint16_t half = mem_read16(mem, addr);
                        cpu_write_reg(cpu, instr->rd, (uint32_t)half); //zero-extended
                        cpu->pc = pc + 4;
                        break;
                    }
                default:
                    LOG_FATAL("unimplemented funct3 for opcode 0x03: funct3=0x%x", instr->funct3);
                break;
            }
            break;
        }
        case 0x23: // Stores
        {
            switch(instr->funct3) {
                case 0x0: // SB
                {
                    uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
                    uint32_t addr = rs1_val + (uint32_t)instr->imm;
                    uint32_t rs2_val = cpu_read_reg(cpu, instr->rs2);
                    mem_write8(mem, addr, (uint8_t)rs2_val);
                    cpu->pc = pc + 4;
                    break;
                }

                case 0x1: // SH
                {
                    uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
                    uint32_t addr = rs1_val + (uint32_t)instr->imm;
                    uint32_t rs2_val = cpu_read_reg(cpu, instr->rs2);
                    mem_write16(mem, addr, (uint16_t)rs2_val);
                    cpu->pc = pc + 4;
                    break;
                }

                case 0x2: // SW
                {
                    uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
                    uint32_t addr = rs1_val + (uint32_t)instr->imm;
                    uint32_t rs2_val = cpu_read_reg(cpu, instr->rs2);
                    mem_write32(mem, addr, (uint32_t)rs2_val);
                    cpu->pc = pc + 4;
                    break;
                }
                default:
                            LOG_FATAL("unimplemented funct3 for opcode 0x23: funct3=0x%x", instr->funct3);
                break;
            }
        break;
        }

        case 0x63: // branches
        {
            switch(instr->funct3) {
                case 0x0: // BEQ
                {
                    uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
                    uint32_t rs2_val = cpu_read_reg(cpu, instr->rs2);
                    if(rs1_val == rs2_val) {
                        cpu->pc = pc + (uint32_t)instr->imm;
                    } else {
                        cpu->pc = pc + 4;
                    }
                    break;
                }
                case 0x1: // BNE
                {
                    uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
                    uint32_t rs2_val = cpu_read_reg(cpu, instr->rs2);
                    if(rs1_val != rs2_val) {
                        cpu->pc = pc + (uint32_t)instr->imm;
                    } else {
                        cpu->pc = pc + 4;
                    }
                    break;
                }
                case 0x4: // BLT
                {
                    uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
                    uint32_t rs2_val = cpu_read_reg(cpu, instr->rs2);
                    if((int32_t)rs1_val < (int32_t)rs2_val) {
                        cpu->pc = pc + (uint32_t)instr->imm;
                    } else {
                        cpu->pc = pc + 4;
                    }
                    break;
                }
                case 0x5: // BGE
                {
                    uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
                    uint32_t rs2_val = cpu_read_reg(cpu, instr->rs2);
                    if((int32_t)rs1_val >= (int32_t)rs2_val) {
                        cpu->pc = pc + (uint32_t)instr->imm;
                    } else {
                        cpu->pc = pc + 4;
                    }
                    break;
                }
                case 0x6: // BLTU
                {
                    uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
                    uint32_t rs2_val = cpu_read_reg(cpu, instr->rs2);
                    if(rs1_val < rs2_val) {
                        cpu->pc = pc + (uint32_t)instr->imm;
                    } else {
                        cpu->pc = pc + 4;
                    }
                    break;
                }
                case 0x7: // BGEU
                {
                    uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
                    uint32_t rs2_val = cpu_read_reg(cpu, instr->rs2);
                    if(rs1_val >= rs2_val) {
                        cpu->pc = pc + (uint32_t)instr->imm;
                    } else {
                        cpu->pc = pc + 4;
                    }
                    break;
                }
                default:
                    LOG_FATAL("unimplemented funct3 for opcode 0x63: funct3=0x%x", instr->funct3);
                break;
            }
        break;
        }

        case 0x6F: // JAL
        {
            cpu_write_reg(cpu, instr->rd, pc+4);
            cpu->pc = pc + (uint32_t)instr->imm;
            break;
        }

        case 0x67: // JALR
        {
            switch (instr->funct3){
                case 0x0:
                {
                    uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
                    uint32_t target = (rs1_val + (uint32_t)instr->imm)&~1u;
                    cpu_write_reg(cpu, instr->rd, pc + 4);
                    cpu->pc = target;
                    break;
                }
                default:
                    LOG_FATAL("unimplemented funct3 for opcode 0x67: funct3=0x%x", instr->funct3);
                break;
            }
            break;
        }

        case 0x0F: // FENCE
        {
            switch(instr->funct3) {
                case 0x0:
                    cpu->pc = pc + 4; // no-op: single-hart, in-order emulator
                    break;
                default:
                    LOG_FATAL("unimplemented funct3 for opcode 0x0F: funct3=0x%x", instr->funct3);
                break;
            }
            break;
        }

        case 0x73: // ECALL / EBREAK
        {
            switch(instr->funct3) {
                case 0x0:
                    LOG_FATAL("ECALL/EBREAK not yet implemented (imm=0x%x)", instr->imm);
                    break;
                default:
                    LOG_FATAL("unimplemented funct3 for opcode 0x73: funct3=0x%x", instr->funct3);
                break;
            }
            break;
        }


        default:
            LOG_FATAL("unimplemented instruction: opcode=0x%x funct3=0x%x", instr->opcode, instr->funct3);

    }
}