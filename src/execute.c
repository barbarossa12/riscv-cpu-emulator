#include "execute.h"
#include "log.h"
#include "pmp.h"
#include "compressed.h"

static uint32_t execute_op_imm(CPU *cpu, Memory *mem, DecodedInstruction *instr, uint32_t pc);
static uint32_t execute_op(CPU *cpu, Memory *mem, DecodedInstruction *instr, uint32_t pc);
static uint32_t execute_lui(CPU *cpu, Memory *mem, DecodedInstruction *instr, uint32_t pc);
static uint32_t execute_auipc(CPU *cpu, Memory *mem, DecodedInstruction *instr, uint32_t pc);
static uint32_t execute_load(CPU *cpu, Memory *mem, DecodedInstruction *instr, uint32_t pc);
static uint32_t execute_store(CPU *cpu, Memory *mem, DecodedInstruction *instr, uint32_t pc);
static uint32_t execute_amo(CPU *cpu, Memory *mem, DecodedInstruction *instr, uint32_t pc);
static uint32_t execute_branch(CPU *cpu, Memory *mem, DecodedInstruction *instr, uint32_t pc);
static uint32_t execute_jal(CPU *cpu, Memory *mem, DecodedInstruction *instr, uint32_t pc);
static uint32_t execute_jalr(CPU *cpu, Memory *mem, DecodedInstruction *instr, uint32_t pc);
static uint32_t execute_fence(CPU *cpu, Memory *mem, DecodedInstruction *instr, uint32_t pc);
static uint32_t execute_system(CPU *cpu, Memory *mem, DecodedInstruction *instr, uint32_t pc);
static uint32_t trap_enter(CPU *cpu, uint32_t pc, uint32_t cause, uint32_t tval);
static uint32_t ecall_cause(PrivMode mode);
static const char *mode_name(PrivMode mode);
static bool csr_exists(uint16_t csr_addr);
static bool csr_is_read_only(uint16_t csr_addr);
static bool csr_is_debug_only(uint16_t csr_addr);
static bool csr_privilege_ok(PrivMode mode, uint16_t csr_addr);
static bool counter_access_ok(CPU *cpu, PrivMode mode, uint16_t csr_addr);
static bool csr_instr_writes(DecodedInstruction *instr);
static uint32_t take_branch(CPU *cpu, uint32_t pc, DecodedInstruction *instr);


static uint32_t execute_op_imm(CPU *cpu, Memory *mem, DecodedInstruction *instr, uint32_t pc) {
    (void)mem;
    switch(instr->funct3) {
        case 0x000: // ADDI
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            cpu_write_reg(cpu, instr->rd, rs1_val + instr->imm);
            return pc + instr->length;
        }
        case 0x02: // SLTI
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            cpu_write_reg(cpu, instr->rd, ((int32_t)rs1_val < instr->imm) ? 1 : 0);
            return pc + instr->length;
        }
        case 0x03: // SLTIU
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            cpu_write_reg(cpu, instr->rd, (rs1_val < (uint32_t)instr->imm) ? 1 : 0);
            return pc + instr->length;
        }
        case 0x04: // XORI
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            cpu_write_reg(cpu, instr->rd, rs1_val ^ (uint32_t)instr->imm);
            return pc + instr->length;
        }
        case 0x06: // ORI
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            cpu_write_reg(cpu, instr->rd, rs1_val | (uint32_t)instr->imm);
            return pc + instr->length;
        }
        case 0x07: // ANDI
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            cpu_write_reg(cpu, instr->rd, rs1_val & (uint32_t)instr->imm);
            return pc + instr->length;
        }
        case 0x1: // SLLI
        {
            if (instr->funct7 != 0x00) {
                return trap_enter(cpu, pc, 2, instr->raw); // illegal instruction
            }
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            cpu_write_reg(cpu, instr->rd, rs1_val << instr->rs2);
            return pc + instr->length;
        }
        case 0x05: // SRLI or SRAI
        {
            if (instr->funct7 != 0x00 && instr->funct7 != 0x20) {
                return trap_enter(cpu, pc, 2, instr->raw); // illegal instruction
            }
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
            return pc + instr->length;
        }
        default:
            return trap_enter(cpu, pc, 2, instr->raw); // illegal instruction
    }
    return pc + instr->length;
}

static uint32_t mulhu(uint32_t a, uint32_t b) {
    return (uint32_t)(((uint64_t)a * b) >> 32);
}

// M extension (funct7 = 0000001): MUL, MULH, MULHSU, MULHU, DIV, DIVU, REM, REMU
static uint32_t muldiv(uint32_t funct3, uint32_t a, uint32_t b) {
    bool a_neg = (a & 0x80000000u) != 0;
    bool b_neg = (b & 0x80000000u) != 0;
    switch (funct3) {
        case 0x0: return a * b;                                           // MUL: low 32 bits
        case 0x1: return mulhu(a, b) - (a_neg ? b : 0) - (b_neg ? a : 0); // MULH: signed x signed
        case 0x2: return mulhu(a, b) - (a_neg ? b : 0);                   // MULHSU: signed x unsigned
        case 0x3: return mulhu(a, b);                                     // MULHU
        case 0x4:                                                         // DIV
            if (b == 0) return 0xFFFFFFFFu;                               // division by zero: all ones
            if (a == 0x80000000u && b == 0xFFFFFFFFu) return a;           // overflow: -2^31 / -1
            return (uint32_t)((int32_t)a / (int32_t)b);
        case 0x5: return b == 0 ? 0xFFFFFFFFu : a / b;                    // DIVU
        case 0x6:                                                         // REM
            if (b == 0) return a;                                         // remainder of x / 0 is x
            if (a == 0x80000000u && b == 0xFFFFFFFFu) return 0;           // overflow: remainder 0
            return (uint32_t)((int32_t)a % (int32_t)b);
        default:  return b == 0 ? a : a % b;                              // REMU
    }
}

static uint32_t execute_op(CPU *cpu, Memory *mem, DecodedInstruction *instr, uint32_t pc) {
    (void)mem;
    if (instr->funct7 == 0x01) {             // M extension: all eight funct3 values are valid
        uint32_t a = cpu_read_reg(cpu, instr->rs1);
        uint32_t b = cpu_read_reg(cpu, instr->rs2);
        cpu_write_reg(cpu, instr->rd, muldiv(instr->funct3, a, b));
        return pc + instr->length;
    }
    // funct7 is 0x00 for every OP instruction except SUB and SRA, which use 0x20
    bool uses_alt = (instr->funct3 == 0x0 || instr->funct3 == 0x5);
    if (instr->funct7 != 0x00 && !(uses_alt && instr->funct7 == 0x20)) {
        return trap_enter(cpu, pc, 2, instr->raw); // illegal instruction
    }
    switch(instr->funct3) {
        case 0x0: // ADD or SUB
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            uint32_t rs2_val = cpu_read_reg(cpu, instr->rs2);
            if(instr->funct7 == 0x00) {
                cpu_write_reg(cpu, instr->rd, rs1_val + rs2_val); // ADD
            } else { // SUB (funct7 == 0x20)
                cpu_write_reg(cpu, instr->rd, rs1_val - rs2_val); // SUB
            }
            return pc + instr->length;
        }
        case 0x1: // SLL
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            uint32_t rs2_val = cpu_read_reg(cpu, instr->rs2);
            cpu_write_reg(cpu, instr->rd, rs1_val << (rs2_val & 0x1F));
            return pc + instr->length;
        }
        case 0x2: // SLT
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            uint32_t rs2_val = cpu_read_reg(cpu, instr->rs2);
            cpu_write_reg(cpu, instr->rd, ((int32_t)rs1_val < (int32_t)rs2_val) ? 1 : 0);
            return pc + instr->length;
        }
        case 0x3: // SLTU
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            uint32_t rs2_val = cpu_read_reg(cpu, instr->rs2);
            cpu_write_reg(cpu, instr->rd, (rs1_val < rs2_val) ? 1 : 0);
            return pc + instr->length;
        }
        case 0x4: // XOR
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            uint32_t rs2_val = cpu_read_reg(cpu, instr->rs2);
            cpu_write_reg(cpu, instr->rd, rs1_val ^ rs2_val);
            return pc + instr->length;
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
            return pc + instr->length;
        }
        case 0x6: // OR
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            uint32_t rs2_val = cpu_read_reg(cpu, instr->rs2);
            cpu_write_reg(cpu, instr->rd, rs1_val | rs2_val);
            return pc + instr->length;
        }
        case 0x7: // AND
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            uint32_t rs2_val = cpu_read_reg(cpu, instr->rs2);
            cpu_write_reg(cpu, instr->rd, rs1_val & rs2_val);
            return pc + instr->length;
        }
        default:
            return trap_enter(cpu, pc, 2, instr->raw); // illegal instruction
    }
    return pc + instr->length;
}

static uint32_t execute_lui(CPU *cpu, Memory *mem, DecodedInstruction *instr, uint32_t pc) {
    (void)mem;
    cpu_write_reg(cpu, instr->rd, (uint32_t)instr->imm);
    return pc + instr->length;
}

static uint32_t execute_auipc(CPU *cpu, Memory *mem, DecodedInstruction *instr, uint32_t pc) {
    (void)mem;
    cpu_write_reg(cpu, instr->rd, pc + (uint32_t)instr->imm);
    return pc + instr->length;
}

// The privilege mode a load or store is checked at. With MPRV set it is MPP, not the current mode
// (machine.adoc:3327). Fetches ignore MPRV.
static PrivMode data_access_mode(CPU *cpu) {
    uint32_t mstatus = cpu_read_csr(cpu, CSR_MSTATUS);
    if (mstatus & MSTATUS_MPRV) {
        return (PrivMode)((mstatus & MSTATUS_MPP) >> 11);
    }
    return cpu->privilege_mode;
}

static bool data_access_ok(CPU *cpu, uint32_t addr, uint32_t size, PmpAccess access) {
    return pmp_check(cpu, addr, size, access, data_access_mode(cpu));
}

static uint32_t execute_load(CPU *cpu, Memory *mem, DecodedInstruction *instr, uint32_t pc) {
    switch(instr->funct3) {
        case 0x0: // LB
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            uint32_t addr = rs1_val + (uint32_t)instr->imm;
            uint8_t byte_val;
            if (!data_access_ok(cpu, addr, 1, PMP_ACCESS_READ) || !mem_try_read8(mem, addr, &byte_val)) {
                return trap_enter(cpu, pc, 5, addr); // load access fault
            }
            cpu_write_reg(cpu, instr->rd, (uint32_t)(int32_t)(int8_t)byte_val);  // sign-extend
            return pc + instr->length;
        }
        case 0x1: // LH
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            uint32_t addr = rs1_val + (uint32_t)instr->imm;
            uint16_t half_val;
            if (!data_access_ok(cpu, addr, 2, PMP_ACCESS_READ) || !mem_try_read16(mem, addr, &half_val)) {
                return trap_enter(cpu, pc, 5, addr); // load access fault
            }
            cpu_write_reg(cpu, instr->rd, (uint32_t)(int32_t)(int16_t)half_val); // sign extended
            return pc + instr->length;
        }
        case 0x2: // LW
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            uint32_t addr = rs1_val + (uint32_t)instr->imm;
            uint32_t word;
            if (!data_access_ok(cpu, addr, 4, PMP_ACCESS_READ) || !mem_try_read32(mem, addr, &word)) {
                return trap_enter(cpu, pc, 5, addr); // load access fault
            }
            cpu_write_reg(cpu, instr->rd, word);
            return pc + instr->length;
        }
        case 0x4: // LBU
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            uint32_t addr = rs1_val + (uint32_t)instr->imm;
            uint8_t byte_val;
            if (!data_access_ok(cpu, addr, 1, PMP_ACCESS_READ) || !mem_try_read8(mem, addr, &byte_val)) {
                return trap_enter(cpu, pc, 5, addr); // load access fault
            }
            cpu_write_reg(cpu, instr->rd, (uint32_t)byte_val);           // zero-extend
            return pc + instr->length;
        }
        case 0x5: // LHU
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            uint32_t addr = rs1_val + (uint32_t)instr->imm;
            uint16_t half_val;
            if (!data_access_ok(cpu, addr, 2, PMP_ACCESS_READ) || !mem_try_read16(mem, addr, &half_val)) {
                return trap_enter(cpu, pc, 5, addr); // load access fault
            }
            cpu_write_reg(cpu, instr->rd, (uint32_t)half_val); //zero-extended
            return pc + instr->length;
        }
        default:
            return trap_enter(cpu, pc, 2, instr->raw); // illegal instruction
    }
    return pc + instr->length;
}

static uint32_t execute_store(CPU *cpu, Memory *mem, DecodedInstruction *instr, uint32_t pc) {
    switch(instr->funct3) {
        case 0x0: // SB
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            uint32_t addr = rs1_val + (uint32_t)instr->imm;
            uint32_t rs2_val = cpu_read_reg(cpu, instr->rs2);
            if (!data_access_ok(cpu, addr, 1, PMP_ACCESS_WRITE) || !mem_try_write8(mem, addr, (uint8_t)rs2_val)) {
                return trap_enter(cpu, pc, 7, addr); // store/AMO access fault
            }
            return pc + instr->length;
        }
        case 0x1: // SH
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            uint32_t addr = rs1_val + (uint32_t)instr->imm;
            uint32_t rs2_val = cpu_read_reg(cpu, instr->rs2);
            if (!data_access_ok(cpu, addr, 2, PMP_ACCESS_WRITE) || !mem_try_write16(mem, addr, (uint16_t)rs2_val)) {
                return trap_enter(cpu, pc, 7, addr); // store/AMO access fault
            }
            return pc + instr->length;
        }
        case 0x2: // SW
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            uint32_t addr = rs1_val + (uint32_t)instr->imm;
            uint32_t rs2_val = cpu_read_reg(cpu, instr->rs2);
            if (!data_access_ok(cpu, addr, 4, PMP_ACCESS_WRITE) || !mem_try_write32(mem, addr, rs2_val)) {
                return trap_enter(cpu, pc, 7, addr); // store/AMO access fault
            }
            return pc + instr->length;
        }
        default:
            return trap_enter(cpu, pc, 2, instr->raw); // illegal instruction
    }
    return pc + instr->length;
}

static bool amo_funct5_valid(uint32_t funct5) {
    switch (funct5) {
        case 0x00: case 0x01: case 0x04: case 0x08: case 0x0C:
        case 0x10: case 0x14: case 0x18: case 0x1C:
            return true;
        default:
            return false;
    }
}

static uint32_t amo_result(uint32_t funct5, uint32_t old, uint32_t src) {
    switch (funct5) {
        case 0x00: return old + src;                                    // AMOADD
        case 0x01: return src;                                          // AMOSWAP
        case 0x04: return old ^ src;                                    // AMOXOR
        case 0x08: return old | src;                                    // AMOOR
        case 0x0C: return old & src;                                    // AMOAND
        case 0x10: return (int32_t)old < (int32_t)src ? old : src;      // AMOMIN
        case 0x14: return (int32_t)old > (int32_t)src ? old : src;      // AMOMAX
        case 0x18: return old < src ? old : src;                        // AMOMINU
        default:   return old > src ? old : src;                        // AMOMAXU (0x1C)
    }
}

// LR.W and SC.W (Zalrsc). One hart and no DMA, so only LR and SC themselves change the reservation:
// it is not cleared on trap entry or xRET, which the manual allows but does not require.
static uint32_t execute_lr_sc(CPU *cpu, Memory *mem, DecodedInstruction *instr, uint32_t pc, uint32_t funct5) {
    uint32_t addr = cpu_read_reg(cpu, instr->rs1);
    if (funct5 == 0x02) {                                  // LR.W: a load that registers a reservation
        if (instr->rs2 != 0) {
            return trap_enter(cpu, pc, 2, instr->raw);     // rs2 is a fixed 0 field
        }
        uint32_t value;
        if (addr & 0x3u) {
            return trap_enter(cpu, pc, 4, addr);           // load address-misaligned
        }
        if (!data_access_ok(cpu, addr, 4, PMP_ACCESS_READ) || !mem_try_read32(mem, addr, &value)) {
            return trap_enter(cpu, pc, 5, addr);           // load access fault
        }
        cpu->lr_valid = true;
        cpu->lr_addr = addr;
        cpu_write_reg(cpu, instr->rd, value);
        return pc + instr->length;
    }
    uint32_t src = cpu_read_reg(cpu, instr->rs2);          // SC.W: read both sources before rd is written
    if (addr & 0x3u) {
        return trap_enter(cpu, pc, 6, addr);               // store/AMO address-misaligned
    }
    bool success = cpu->lr_valid && cpu->lr_addr == addr;
    uint32_t probe;
    // A store-conditional must pass the store checks even when it is going to fail.
    if (!data_access_ok(cpu, addr, 4, PMP_ACCESS_WRITE) || !mem_try_read32(mem, addr, &probe) ||
        (success && !mem_try_write32(mem, addr, src))) {
        return trap_enter(cpu, pc, 7, addr);               // store/AMO access fault
    }
    cpu->lr_valid = false;                                 // success or failure
    cpu_write_reg(cpu, instr->rd, success ? 0 : 1);
    return pc + instr->length;
}

// A extension, atomic memory operations (opcode 0101111). funct7 = funct5:aq:rl; aq and rl only
// order accesses between harts, so they are ignored.
static uint32_t execute_amo(CPU *cpu, Memory *mem, DecodedInstruction *instr, uint32_t pc) {
    uint32_t funct5 = instr->funct7 >> 2;
    if (instr->funct3 != 0x2) {
        return trap_enter(cpu, pc, 2, instr->raw);         // only the word forms exist on RV32
    }
    if (funct5 == 0x02 || funct5 == 0x03) {
        return execute_lr_sc(cpu, mem, instr, pc, funct5);
    }
    if (!amo_funct5_valid(funct5)) {
        return trap_enter(cpu, pc, 2, instr->raw);
    }
    uint32_t addr = cpu_read_reg(cpu, instr->rs1);
    uint32_t src = cpu_read_reg(cpu, instr->rs2);          // read both sources before rd is written
    if (addr & 0x3u) {
        return trap_enter(cpu, pc, 6, addr);               // store/AMO address-misaligned
    }
    uint32_t old;
    // An AMO reads and writes; W without R cannot exist under our PMP rules, so a write check covers both.
    if (!data_access_ok(cpu, addr, 4, PMP_ACCESS_WRITE) || !mem_try_read32(mem, addr, &old) ||
        !mem_try_write32(mem, addr, amo_result(funct5, old, src))) {
        return trap_enter(cpu, pc, 7, addr);               // store/AMO access fault
    }
    cpu_write_reg(cpu, instr->rd, old);
    return pc + instr->length;
}

static uint32_t execute_branch(CPU *cpu, Memory *mem, DecodedInstruction *instr, uint32_t pc) {
    (void)mem;
    switch(instr->funct3) {
        case 0x0: // BEQ
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            uint32_t rs2_val = cpu_read_reg(cpu, instr->rs2);
            if(rs1_val == rs2_val) {
                return take_branch(cpu, pc, instr);
            }
            return pc + instr->length;
        }
        case 0x1: // BNE
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            uint32_t rs2_val = cpu_read_reg(cpu, instr->rs2);
            if(rs1_val != rs2_val) {
                return take_branch(cpu, pc, instr);
            }
            return pc + instr->length;
        }
        case 0x4: // BLT
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            uint32_t rs2_val = cpu_read_reg(cpu, instr->rs2);
            if((int32_t)rs1_val < (int32_t)rs2_val) {
                return take_branch(cpu, pc, instr);
            }
            return pc + instr->length;
        }
        case 0x5: // BGE
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            uint32_t rs2_val = cpu_read_reg(cpu, instr->rs2);
            if((int32_t)rs1_val >= (int32_t)rs2_val) {
                return take_branch(cpu, pc, instr);
            }
            return pc + instr->length;
        }
        case 0x6: // BLTU
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            uint32_t rs2_val = cpu_read_reg(cpu, instr->rs2);
            if(rs1_val < rs2_val) {
                return take_branch(cpu, pc, instr);
            }
            return pc + instr->length;
        }
        case 0x7: // BGEU
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            uint32_t rs2_val = cpu_read_reg(cpu, instr->rs2);
            if(rs1_val >= rs2_val) {
                return take_branch(cpu, pc, instr);
            }
            return pc + instr->length;
        }
        default:
            return trap_enter(cpu, pc, 2, instr->raw); // illegal instruction
    }
    return pc + instr->length;
}

static uint32_t execute_jal(CPU *cpu, Memory *mem, DecodedInstruction *instr, uint32_t pc) {
    (void)mem;
    uint32_t target = pc + (uint32_t)instr->imm;
    cpu_write_reg(cpu, instr->rd, pc + instr->length);
    return target;
}

static uint32_t execute_jalr(CPU *cpu, Memory *mem, DecodedInstruction *instr, uint32_t pc) {
    (void)mem;
    switch (instr->funct3) {
        case 0x0:
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            uint32_t target = (rs1_val + (uint32_t)instr->imm) & ~1u;
            cpu_write_reg(cpu, instr->rd, pc + instr->length);
            return target;
        }
        default:
            return trap_enter(cpu, pc, 2, instr->raw); // illegal instruction
    }
    return pc + instr->length;
}

static uint32_t execute_fence(CPU *cpu, Memory *mem, DecodedInstruction *instr, uint32_t pc) {
    (void)cpu;
    (void)mem;
    switch (instr->funct3) {
        case 0x0:
            return pc + instr->length; // no-op: single-hart, in-order emulator
        case 0x1:
            // FENCE.I: no-op, there is no instruction cache and every fetch reads
            // memory, so stores are always visible to later fetches. The spec says
            // base implementations ignore funct12, rs1 and rd (zifencei.adoc).
            return pc + instr->length;
        default:
            return trap_enter(cpu, pc, 2, instr->raw); // illegal instruction
    }
    return pc + instr->length;
}


static uint32_t execute_system(CPU *cpu, Memory *mem, DecodedInstruction *instr, uint32_t pc) {
    (void)mem;
    uint16_t csr_addr = (uint16_t)(instr->imm & 0xFFF);
    // funct3 == 0 is ECALL/EBREAK/MRET, whose imm field is not a CSR address
    if (instr->funct3 != 0 && !csr_exists(csr_addr)) {
        return trap_enter(cpu, pc, 2, instr->raw); // non-existent CSR is reserved: illegal instruction
    }
    if (instr->funct3 != 0 && csr_is_debug_only(csr_addr)) {
        return trap_enter(cpu, pc, 2, instr->raw); // debug-mode-only CSR: illegal instruction
    }
    if (instr->funct3 != 0 && !csr_privilege_ok(cpu->privilege_mode, csr_addr)) {
        return trap_enter(cpu, pc, 2, instr->raw); // CSR needs a higher privilege mode
    }
    if (instr->funct3 != 0 && !counter_access_ok(cpu, cpu->privilege_mode, csr_addr)) {
        return trap_enter(cpu, pc, 2, instr->raw); // counter not enabled for this mode
    }
    if (instr->funct3 != 0 && csr_addr == CSR_SATP && cpu->privilege_mode == PRV_S &&
        (cpu_read_csr(cpu, CSR_MSTATUS) & MSTATUS_TVM)) {
        return trap_enter(cpu, pc, 2, instr->raw); // TVM = 1: S-mode may not touch satp
    }
    if (csr_is_read_only(csr_addr) && csr_instr_writes(instr)) {
        return trap_enter(cpu, pc, 2, instr->raw); // write to a read-only CSR: illegal instruction
    }
    switch (instr->funct3) {
        case 0x0: {
            // ECALL, EBREAK, MRET, SRET and WFI fix rd and rs1 to 0. SFENCE.VMA fixes rd to 0
            // and uses rs1 and rs2 for an address and an ASID. Anything else matches no instruction.
            bool is_sfence_vma = (instr->imm & ~0x1F) == 0x120;   // funct7 = 0001001, imm = funct7:rs2
            if (instr->rd != 0 || (instr->rs1 != 0 && !is_sfence_vma)) {
                return trap_enter(cpu, pc, 2, instr->raw);
            }
            if (instr->imm == 0) { // ECALL
                return trap_enter(cpu, pc, ecall_cause(cpu->privilege_mode), 0);
            } else if (instr->imm == 1) { // EBREAK
                return trap_enter(cpu, pc, 3, 0);
            } else if (instr->imm == 0x302) { // MRET
                if (cpu->privilege_mode != PRV_M) {
                    return trap_enter(cpu, pc, 2, instr->raw); // MRET below M-mode is illegal
                }
                uint32_t mstatus = cpu_read_csr(cpu, CSR_MSTATUS);
                PrivMode prev = (PrivMode)((mstatus & MSTATUS_MPP) >> 11);
                uint32_t mpie = (mstatus & MSTATUS_MPIE) ? 1 : 0;
                mstatus &= ~(MSTATUS_MIE | MSTATUS_MPP); // MPP = U, the least-privileged mode
                mstatus |= mpie ? MSTATUS_MIE : 0;       // MIE = old MPIE
                mstatus |= MSTATUS_MPIE;                 // MPIE = 1
                if (prev != PRV_M) {
                    mstatus &= ~MSTATUS_MPRV;            // returning below M clears MPRV
                }
                cpu_write_csr(cpu, CSR_MSTATUS, mstatus);
                cpu->privilege_mode = prev;
                LOG_DEBUG("mret: M -> %s, pc 0x%08x", mode_name(prev), cpu_read_csr(cpu, CSR_MEPC));
                return cpu_read_csr(cpu, CSR_MEPC); // jump back to where the trap occurred
            } else if (instr->imm == 0x102) { // SRET
                uint32_t mstatus = cpu_read_csr(cpu, CSR_MSTATUS);
                if (cpu->privilege_mode == PRV_U ||
                    (cpu->privilege_mode == PRV_S && (mstatus & MSTATUS_TSR))) {
                    return trap_enter(cpu, pc, 2, instr->raw); // U never; S only when TSR = 0
                }
                PrivMode from = cpu->privilege_mode;
                PrivMode prev = (mstatus & MSTATUS_SPP) ? PRV_S : PRV_U;
                uint32_t spie = (mstatus & MSTATUS_SPIE) ? 1 : 0;
                mstatus &= ~(MSTATUS_SIE | MSTATUS_SPP); // SPP = U, the least-privileged mode
                mstatus |= spie ? MSTATUS_SIE : 0;       // SIE = old SPIE
                mstatus |= MSTATUS_SPIE;                 // SPIE = 1
                mstatus &= ~MSTATUS_MPRV;                // SRET never returns to M
                cpu_write_csr(cpu, CSR_MSTATUS, mstatus);
                cpu->privilege_mode = prev;
                LOG_DEBUG("sret: %s -> %s, pc 0x%08x", mode_name(from), mode_name(prev), cpu_read_csr(cpu, CSR_SEPC));
                return cpu_read_csr(cpu, CSR_SEPC);
            } else if (instr->imm == 0x105) { // WFI
                if (cpu->privilege_mode == PRV_U ||
                    (cpu->privilege_mode == PRV_S && (cpu_read_csr(cpu, CSR_MSTATUS) & MSTATUS_TW))) {
                    return trap_enter(cpu, pc, 2, instr->raw); // U never; S only when TW = 0
                }
                return pc + instr->length; // a legal NOP: there is nothing to wait for yet
            } else if (is_sfence_vma) {
                if (cpu->privilege_mode == PRV_U ||
                    (cpu->privilege_mode == PRV_S && (cpu_read_csr(cpu, CSR_MSTATUS) & MSTATUS_TVM))) {
                    return trap_enter(cpu, pc, 2, instr->raw); // U never; S only when TVM = 0
                }
                return pc + instr->length; // no translation cache yet, so nothing to fence
            }
            return trap_enter(cpu, pc, 2, instr->raw); // any other encoding, DRET included, is illegal
        }
        case 0x1: // CSRRW
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            uint32_t old = cpu_read_csr(cpu, csr_addr);
            cpu_write_csr(cpu, csr_addr, rs1_val);
            cpu_write_reg(cpu, instr->rd, old);
            return pc + instr->length;
        }
        case 0x2: // CSRRS
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            uint32_t old = cpu_read_csr(cpu, csr_addr);
            if (instr->rs1 != 0) { // rs1 = x0: no write at all
                cpu_write_csr(cpu, csr_addr, old | rs1_val);
            }
            cpu_write_reg(cpu, instr->rd, old);
            return pc + instr->length;
        }
        case 0x3: // CSRRC
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            uint32_t old = cpu_read_csr(cpu, csr_addr);
            if (instr->rs1 != 0) { // rs1 = x0: no write at all
                cpu_write_csr(cpu, csr_addr, old & ~rs1_val);
            }
            cpu_write_reg(cpu, instr->rd, old);
            return pc + instr->length;
        }
        case 0x5: // CSRRWI
        {
            uint32_t old = cpu_read_csr(cpu, csr_addr);
            cpu_write_csr(cpu, csr_addr, (uint32_t)instr->rs1); // rs1 field holds uimm here
            cpu_write_reg(cpu, instr->rd, old);
            return pc + instr->length;
        }
        case 0x6: // CSRRSI
        {
            uint32_t old = cpu_read_csr(cpu, csr_addr);
            if (instr->rs1 != 0) { // uimm = 0: no write at all
                cpu_write_csr(cpu, csr_addr, old | (uint32_t)instr->rs1);
            }
            cpu_write_reg(cpu, instr->rd, old);
            return pc + instr->length;
        }
        case 0x7: // CSRRCI
        {
            uint32_t old = cpu_read_csr(cpu, csr_addr);
            if (instr->rs1 != 0) { // uimm = 0: no write at all
                cpu_write_csr(cpu, csr_addr, old & ~(uint32_t)instr->rs1);
            }
            cpu_write_reg(cpu, instr->rd, old);
            return pc + instr->length;
        }
        default:
            return trap_enter(cpu, pc, 2, instr->raw); // illegal instruction
    }
    return pc + instr->length;
}

// Only the CSRs we implement exist; every other address is reserved and traps
static bool csr_exists(uint16_t csr_addr) {
    switch (csr_addr) {
        case CSR_MSTATUS: case CSR_MISA: case CSR_MIE: case CSR_MTVEC:
        case CSR_MSCRATCH: case CSR_MEPC: case CSR_MCAUSE: case CSR_MTVAL:
        case CSR_MIP:
        case CSR_TSELECT: case CSR_TDATA1: case CSR_TDATA2:
        case CSR_MVENDORID: case CSR_MARCHID: case CSR_MIMPID: case CSR_MHARTID:
        case CSR_MCYCLE: case CSR_MINSTRET: case CSR_MCYCLEH: case CSR_MINSTRETH:
        case CSR_CYCLE: case CSR_INSTRET: case CSR_CYCLEH: case CSR_INSTRETH:
        case CSR_MCOUNTEREN: case CSR_SCOUNTEREN:
        case CSR_SSTATUS: case CSR_SIE: case CSR_STVEC: case CSR_SSCRATCH:
        case CSR_SEPC: case CSR_SCAUSE: case CSR_STVAL: case CSR_SIP: case CSR_SATP:
        case CSR_MEDELEG: case CSR_MIDELEG: case CSR_MEDELEGH: case CSR_MSTATUSH:
            return true;
        default:
            return pmp_csr_exists(csr_addr);
    }
}

// csr[9:8] is the lowest privilege mode that may access the CSR. The encodings match
// PrivMode (00 = U, 01 = S, 11 = M); 10 is the hypervisor range, which does not exist.
static bool csr_privilege_ok(PrivMode mode, uint16_t csr_addr) {
    return (uint32_t)mode >= ((csr_addr >> 8) & 0x3u);
}

// cycle/instret and their h halves: S-mode needs the mcounteren bit, U-mode needs both the
// mcounteren and the scounteren bit. M-mode is never gated.
static bool counter_access_ok(CPU *cpu, PrivMode mode, uint16_t csr_addr) {
    uint32_t bit;
    switch (csr_addr) {
        case CSR_CYCLE:   case CSR_CYCLEH:   bit = COUNTEREN_CY; break;
        case CSR_INSTRET: case CSR_INSTRETH: bit = COUNTEREN_IR; break;
        default: return true;
    }
    if (mode == PRV_M) {
        return true;
    }
    if (!(cpu_read_csr(cpu, CSR_MCOUNTEREN) & bit)) {
        return false;
    }
    return mode == PRV_S || (cpu_read_csr(cpu, CSR_SCOUNTEREN) & bit);
}

// csr[11:10] == 0b11 marks a read-only CSR address
static bool csr_is_read_only(uint16_t csr_addr) {
    return ((csr_addr >> 10) & 0x3) == 0x3;
}

// 0x7B0-0x7BF are only visible to debug mode, so any M-mode access is illegal
static bool csr_is_debug_only(uint16_t csr_addr) {
    return csr_addr >= 0x7B0 && csr_addr <= 0x7BF;
}

// Whether a Zicsr instruction writes the CSR. CSRRW/CSRRWI always do; the
// set/clear forms only do when rs1 (register or uimm) is non-zero.
static bool csr_instr_writes(DecodedInstruction *instr) {
    switch (instr->funct3) {
        case 0x1: // CSRRW
        case 0x5: // CSRRWI
            return true;
        case 0x2: // CSRRS
        case 0x3: // CSRRC
        case 0x6: // CSRRSI
        case 0x7: // CSRRCI
            return instr->rs1 != 0;
        default:
            return false;
    }
}

// Target of a taken conditional branch. With IALIGN=16 and even offsets it is always aligned.
static uint32_t take_branch(CPU *cpu, uint32_t pc, DecodedInstruction *instr) {
    (void)cpu;
    return pc + (uint32_t)instr->imm;
}

static const char *mode_name(PrivMode mode) {
    switch (mode) {
        case PRV_U: return "U";
        case PRV_S: return "S";
        case PRV_M: return "M";
        default:    return "?";
    }
}

// Environment-call exception code for the mode ECALL runs in
static uint32_t ecall_cause(PrivMode mode) {
    switch (mode) {
        case PRV_U: return 8;
        case PRV_S: return 9;
        default:    return 11;
    }
}

static uint32_t trap_enter(CPU *cpu, uint32_t pc, uint32_t cause, uint32_t tval) {
    cpu->trapped = true; // a trapping instruction does not retire (minstret)
    PrivMode from = cpu->privilege_mode;
    // A trap taken in M is never delegated. From S or U, a set medeleg bit sends it to S.
    // (Interrupts have bit 31 set and do not come through here until Milestone 6.)
    bool to_s = from != PRV_M && cause < 32 && ((cpu_read_csr(cpu, CSR_MEDELEG) >> cause) & 1u);
    LOG_DEBUG("trap: %s -> %s, cause %u, epc 0x%08x, tval 0x%08x",
              mode_name(from), to_s ? "S" : "M", cause, pc, tval);

    uint32_t mstatus = cpu_read_csr(cpu, CSR_MSTATUS);
    if (to_s) {
        uint32_t sie = (mstatus & MSTATUS_SIE) ? 1 : 0;
        mstatus &= ~(MSTATUS_SPIE | MSTATUS_SIE | MSTATUS_SPP);
        mstatus |= sie ? MSTATUS_SPIE : 0;               // SPIE = old SIE
        mstatus |= (from == PRV_U) ? 0 : MSTATUS_SPP;    // SPP = U (0) or not U (1)
        cpu_write_csr(cpu, CSR_MSTATUS, mstatus);
        cpu->privilege_mode = PRV_S;
        cpu_write_csr(cpu, CSR_SEPC, pc);
        cpu_write_csr(cpu, CSR_SCAUSE, cause);
        cpu_write_csr(cpu, CSR_STVAL, tval);
        return cpu_read_csr(cpu, CSR_STVEC);
    }

    uint32_t mie = (mstatus & MSTATUS_MIE) ? 1 : 0;
    mstatus &= ~(MSTATUS_MPIE | MSTATUS_MIE | MSTATUS_MPP);
    mstatus |= mie ? MSTATUS_MPIE : 0;                   // MPIE = old MIE
    mstatus |= ((uint32_t)from << 11);                   // MPP = the mode we trapped from
    cpu_write_csr(cpu, CSR_MSTATUS, mstatus);
    cpu->privilege_mode = PRV_M;

    cpu_write_csr(cpu, CSR_MEPC, pc);
    cpu_write_csr(cpu, CSR_MCAUSE, cause);
    cpu_write_csr(cpu, CSR_MTVAL, tval);
    return cpu_read_csr(cpu, CSR_MTVEC);
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
        case 0x2F: cpu->pc = execute_amo(cpu, mem, instr, pc); break;
        default:
            cpu->pc = trap_enter(cpu, pc, 2, instr->raw); // illegal instruction
        break;
    }
}

// One 2-byte instruction fetch, PMP-checked and bounds-checked on its own.
static bool fetch_halfword(CPU *cpu, Memory *mem, uint32_t addr, uint16_t *out) {
    return pmp_check(cpu, addr, 2, PMP_ACCESS_EXEC, cpu->privilege_mode) &&
           mem_try_read16(mem, addr, out);
}

void cpu_step(CPU *cpu, Memory *mem) {
    cpu->trapped = false;
    cpu->mcycle_written = false;
    cpu->minstret_written = false;

    uint32_t pc = cpu->pc;
    uint16_t low;
    if (!fetch_halfword(cpu, mem, pc, &low)) {
        cpu->pc = trap_enter(cpu, pc, 1, pc); // instruction access fault
    } else if ((low & 0x3u) == 0x3u) {        // bits [1:0] = 11: a 32-bit instruction
        uint16_t high;
        if (!fetch_halfword(cpu, mem, pc + 2, &high)) {
            cpu->pc = trap_enter(cpu, pc, 1, pc + 2); // mtval is the part that faulted; mepc stays at the start
        } else {
            DecodedInstruction instr = decode_instruction((uint32_t)high << 16 | low);
            execute(cpu, mem, &instr);
        }
    } else {                                  // a compressed instruction
        uint32_t word;
        if (!expand_compressed(low, &word)) {
            cpu->pc = trap_enter(cpu, pc, 2, low); // illegal instruction; mtval is its 16 bits
        } else {
            DecodedInstruction instr = decode_instruction(word);
            instr.raw = low;                  // what an illegal-instruction trap reports
            instr.length = 2;
            execute(cpu, mem, &instr);
        }
    }

    // One cycle per step. An instruction that traps (this includes ECALL and
    // EBREAK) does not retire, and a counter write takes effect after its own
    // instruction, so it suppresses that counter's increment.
    if (!cpu->mcycle_written) {
        cpu->mcycle++;
    }
    if (!cpu->trapped && !cpu->minstret_written) {
        cpu->minstret++;
    }
}
