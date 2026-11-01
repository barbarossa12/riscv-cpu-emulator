#include "execute.h"
#include "log.h"

static uint32_t execute_op_imm(CPU *cpu, Memory *mem, DecodedInstruction *instr, uint32_t pc);
static uint32_t execute_op(CPU *cpu, Memory *mem, DecodedInstruction *instr, uint32_t pc);
static uint32_t execute_lui(CPU *cpu, Memory *mem, DecodedInstruction *instr, uint32_t pc);
static uint32_t execute_auipc(CPU *cpu, Memory *mem, DecodedInstruction *instr, uint32_t pc);
static uint32_t execute_load(CPU *cpu, Memory *mem, DecodedInstruction *instr, uint32_t pc);
static uint32_t execute_store(CPU *cpu, Memory *mem, DecodedInstruction *instr, uint32_t pc);
static uint32_t execute_branch(CPU *cpu, Memory *mem, DecodedInstruction *instr, uint32_t pc);
static uint32_t execute_jal(CPU *cpu, Memory *mem, DecodedInstruction *instr, uint32_t pc);
static uint32_t execute_jalr(CPU *cpu, Memory *mem, DecodedInstruction *instr, uint32_t pc);
static uint32_t execute_fence(CPU *cpu, Memory *mem, DecodedInstruction *instr, uint32_t pc);
static uint32_t execute_system(CPU *cpu, Memory *mem, DecodedInstruction *instr, uint32_t pc);

static uint32_t execute_op_imm(CPU *cpu, Memory *mem, DecodedInstruction *instr, uint32_t pc) {
    (void)mem;
    switch(instr->funct3) {
        case 0x000: // ADDI
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            cpu_write_reg(cpu, instr->rd, rs1_val + instr->imm);
            return pc + 4;
        }
        case 0x02: // SLTI
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            cpu_write_reg(cpu, instr->rd, ((int32_t)rs1_val < instr->imm) ? 1 : 0);
            return pc + 4;
        }
        case 0x03: // SLTIU
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            cpu_write_reg(cpu, instr->rd, (rs1_val < (uint32_t)instr->imm) ? 1 : 0);
            return pc + 4;
        }
        case 0x04: // XORI
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            cpu_write_reg(cpu, instr->rd, rs1_val ^ (uint32_t)instr->imm);
            return pc + 4;
        }
        case 0x06: // ORI
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            cpu_write_reg(cpu, instr->rd, rs1_val | (uint32_t)instr->imm);
            return pc + 4;
        }
        case 0x07: // ANDI
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            cpu_write_reg(cpu, instr->rd, rs1_val & (uint32_t)instr->imm);
            return pc + 4;
        }
        case 0x1: // SLLI
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            cpu_write_reg(cpu, instr->rd, rs1_val << instr->rs2);
            return pc + 4;
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
            return pc + 4;
        }
        default:
            LOG_FATAL("unimplemented funct3 for opcode 0x13: funct3=0x%x", instr->funct3);
        break;
    }
    return pc + 4;
}

static uint32_t execute_op(CPU *cpu, Memory *mem, DecodedInstruction *instr, uint32_t pc) {
    (void)mem;
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
            return pc + 4;
        }
        case 0x1: // SLL
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            uint32_t rs2_val = cpu_read_reg(cpu, instr->rs2);
            cpu_write_reg(cpu, instr->rd, rs1_val << (rs2_val & 0x1F));
            return pc + 4;
        }
        case 0x2: // SLT
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            uint32_t rs2_val = cpu_read_reg(cpu, instr->rs2);
            cpu_write_reg(cpu, instr->rd, ((int32_t)rs1_val < (int32_t)rs2_val) ? 1 : 0);
            return pc + 4;
        }
        case 0x3: // SLTU
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            uint32_t rs2_val = cpu_read_reg(cpu, instr->rs2);
            cpu_write_reg(cpu, instr->rd, (rs1_val < rs2_val) ? 1 : 0);
            return pc + 4;
        }
        case 0x4: // XOR
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            uint32_t rs2_val = cpu_read_reg(cpu, instr->rs2);
            cpu_write_reg(cpu, instr->rd, rs1_val ^ rs2_val);
            return pc + 4;
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
            return pc + 4;
        }
        case 0x6: // OR
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            uint32_t rs2_val = cpu_read_reg(cpu, instr->rs2);
            cpu_write_reg(cpu, instr->rd, rs1_val | rs2_val);
            return pc + 4;
        }
        case 0x7: // AND
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            uint32_t rs2_val = cpu_read_reg(cpu, instr->rs2);
            cpu_write_reg(cpu, instr->rd, rs1_val & rs2_val);
            return pc + 4;
        }
        default:
            LOG_FATAL("unimplemented funct3 for opcode 0x33: funct3=0x%x", instr->funct3);
        break;
    }
    return pc + 4;
}

static uint32_t execute_lui(CPU *cpu, Memory *mem, DecodedInstruction *instr, uint32_t pc) {
    (void)mem;
    cpu_write_reg(cpu, instr->rd, (uint32_t)instr->imm);
    return pc + 4;
}

static uint32_t execute_auipc(CPU *cpu, Memory *mem, DecodedInstruction *instr, uint32_t pc) {
    (void)mem;
    cpu_write_reg(cpu, instr->rd, pc + (uint32_t)instr->imm);
    return pc + 4;
}

static uint32_t execute_load(CPU *cpu, Memory *mem, DecodedInstruction *instr, uint32_t pc) {
    switch(instr->funct3) {
        case 0x0: // LB
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            uint32_t addr = rs1_val + (uint32_t)instr->imm;
            int8_t byte = (int8_t)mem_read8(mem, addr);
            cpu_write_reg(cpu, instr->rd, (uint32_t)(int32_t)byte);  // sign-extend
            return pc + 4;
        }
        case 0x1: // LH
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            uint32_t addr = rs1_val + (uint32_t)instr->imm;
            int16_t half = (int16_t)mem_read16(mem, addr);
            cpu_write_reg(cpu, instr->rd, (uint32_t)(int32_t)half); // sign extended
            return pc + 4;
        }
        case 0x2: // LW
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            uint32_t addr = rs1_val + (uint32_t)instr->imm;
            uint32_t word = mem_read32(mem, addr);
            cpu_write_reg(cpu, instr->rd, word);
            return pc + 4;
        }
        case 0x4: // LBU
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            uint32_t addr = rs1_val + (uint32_t)instr->imm;
            uint8_t byte = mem_read8(mem, addr);
            cpu_write_reg(cpu, instr->rd, (uint32_t)byte);           // zero-extend
            return pc + 4;
        }
        case 0x5: // LHU
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            uint32_t addr = rs1_val + (uint32_t)instr->imm;
            uint16_t half = mem_read16(mem, addr);
            cpu_write_reg(cpu, instr->rd, (uint32_t)half); //zero-extended
            return pc + 4;
        }
        default:
            LOG_FATAL("unimplemented funct3 for opcode 0x03: funct3=0x%x", instr->funct3);
        break;
    }
    return pc + 4;
}

static uint32_t execute_store(CPU *cpu, Memory *mem, DecodedInstruction *instr, uint32_t pc) {
    switch(instr->funct3) {
        case 0x0: // SB
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            uint32_t addr = rs1_val + (uint32_t)instr->imm;
            uint32_t rs2_val = cpu_read_reg(cpu, instr->rs2);
            mem_write8(mem, addr, (uint8_t)rs2_val);
            return pc + 4;
        }
        case 0x1: // SH
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            uint32_t addr = rs1_val + (uint32_t)instr->imm;
            uint32_t rs2_val = cpu_read_reg(cpu, instr->rs2);
            mem_write16(mem, addr, (uint16_t)rs2_val);
            return pc + 4;
        }
        case 0x2: // SW
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            uint32_t addr = rs1_val + (uint32_t)instr->imm;
            uint32_t rs2_val = cpu_read_reg(cpu, instr->rs2);
            mem_write32(mem, addr, rs2_val);
            return pc + 4;
        }
        default:
            LOG_FATAL("unimplemented funct3 for opcode 0x23: funct3=0x%x", instr->funct3);
        break;
    }
    return pc + 4;
}

static uint32_t execute_branch(CPU *cpu, Memory *mem, DecodedInstruction *instr, uint32_t pc) {
    (void)mem;
    switch(instr->funct3) {
        case 0x0: // BEQ
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            uint32_t rs2_val = cpu_read_reg(cpu, instr->rs2);
            if(rs1_val == rs2_val) {
                return pc + (uint32_t)instr->imm;
            }
            return pc + 4;
        }
        case 0x1: // BNE
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            uint32_t rs2_val = cpu_read_reg(cpu, instr->rs2);
            if(rs1_val != rs2_val) {
                return pc + (uint32_t)instr->imm;
            }
            return pc + 4;
        }
        case 0x4: // BLT
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            uint32_t rs2_val = cpu_read_reg(cpu, instr->rs2);
            if((int32_t)rs1_val < (int32_t)rs2_val) {
                return pc + (uint32_t)instr->imm;
            }
            return pc + 4;
        }
        case 0x5: // BGE
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            uint32_t rs2_val = cpu_read_reg(cpu, instr->rs2);
            if((int32_t)rs1_val >= (int32_t)rs2_val) {
                return pc + (uint32_t)instr->imm;
            }
            return pc + 4;
        }
        case 0x6: // BLTU
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            uint32_t rs2_val = cpu_read_reg(cpu, instr->rs2);
            if(rs1_val < rs2_val) {
                return pc + (uint32_t)instr->imm;
            }
            return pc + 4;
        }
        case 0x7: // BGEU
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            uint32_t rs2_val = cpu_read_reg(cpu, instr->rs2);
            if(rs1_val >= rs2_val) {
                return pc + (uint32_t)instr->imm;
            }
            return pc + 4;
        }
        default:
            LOG_FATAL("unimplemented funct3 for opcode 0x63: funct3=0x%x", instr->funct3);
        break;
    }
    return pc + 4;
}

static uint32_t execute_jal(CPU *cpu, Memory *mem, DecodedInstruction *instr, uint32_t pc) {
    (void)mem;
    cpu_write_reg(cpu, instr->rd, pc + 4);
    return pc + (uint32_t)instr->imm;
}

static uint32_t execute_jalr(CPU *cpu, Memory *mem, DecodedInstruction *instr, uint32_t pc) {
    (void)mem;
    switch (instr->funct3) {
        case 0x0:
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            uint32_t target = (rs1_val + (uint32_t)instr->imm) & ~1u;
            cpu_write_reg(cpu, instr->rd, pc + 4);
            return target;
        }
        default:
            LOG_FATAL("unimplemented funct3 for opcode 0x67: funct3=0x%x", instr->funct3);
        break;
    }
    return pc + 4;
}

static uint32_t execute_fence(CPU *cpu, Memory *mem, DecodedInstruction *instr, uint32_t pc) {
    (void)cpu;
    (void)mem;
    switch (instr->funct3) {
        case 0x0:
            return pc + 4; // no-op: single-hart, in-order emulator
        default:
            LOG_FATAL("unimplemented funct3 for opcode 0x0F: funct3=0x%x", instr->funct3);
        break;
    }
    return pc + 4;
}

static uint32_t execute_system(CPU *cpu, Memory *mem, DecodedInstruction *instr, uint32_t pc) {
    (void)cpu;
    (void)mem;
    switch (instr->funct3) {
        case 0x0:
            LOG_FATAL("ECALL/EBREAK not yet implemented (imm=0x%x)", instr->imm);
        break;
        default:
            LOG_FATAL("unimplemented funct3 for opcode 0x73: funct3=0x%x", instr->funct3);
        break;
    }
    return pc + 4;
}

void execute(CPU *cpu, Memory *mem, DecodedInstruction *instr) {
    uint32_t pc = cpu->pc;
    switch(instr->opcode) {
        case 0x13: cpu->pc = execute_op_imm(cpu, mem, instr, pc); break;
        case 0x33: cpu->pc = execute_op(cpu, mem, instr, pc); break;
        case 0x37: cpu->pc = execute_lui(cpu, mem, instr, pc); break;
        case 0x17: cpu->pc = execute_auipc(cpu, mem, instr, pc); break;
        case 0x03: cpu->pc = execute_load(cpu, mem, instr, pc); break;
        case 0x23: cpu->pc = execute_store(cpu, mem, instr, pc); break;
        case 0x63: cpu->pc = execute_branch(cpu, mem, instr, pc); break;
        case 0x6F: cpu->pc = execute_jal(cpu, mem, instr, pc); break;
        case 0x67: cpu->pc = execute_jalr(cpu, mem, instr, pc); break;
        case 0x0F: cpu->pc = execute_fence(cpu, mem, instr, pc); break;
        case 0x73: cpu->pc = execute_system(cpu, mem, instr, pc); break;
        default:
            LOG_FATAL("unimplemented instruction: opcode=0x%x funct3=0x%x", instr->opcode, instr->funct3);
    }
}
