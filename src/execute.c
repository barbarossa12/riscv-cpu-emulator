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
static uint32_t trap_enter(CPU *cpu, uint32_t pc, uint32_t cause, uint32_t tval);
static uint32_t ecall_cause(PrivMode mode);
static const char *mode_name(PrivMode mode);
static bool target_misaligned(uint32_t target);
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
            if (instr->funct7 != 0x00) {
                return trap_enter(cpu, pc, 2, instr->raw); // illegal instruction
            }
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            cpu_write_reg(cpu, instr->rd, rs1_val << instr->rs2);
            return pc + 4;
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
            return pc + 4;
        }
        default:
            return trap_enter(cpu, pc, 2, instr->raw); // illegal instruction
    }
    return pc + 4;
}

static uint32_t execute_op(CPU *cpu, Memory *mem, DecodedInstruction *instr, uint32_t pc) {
    (void)mem;
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
            return trap_enter(cpu, pc, 2, instr->raw); // illegal instruction
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
            uint8_t byte_val;
            if (!mem_try_read8(mem, addr, &byte_val)) {
                return trap_enter(cpu, pc, 5, addr); // load access fault
            }
            cpu_write_reg(cpu, instr->rd, (uint32_t)(int32_t)(int8_t)byte_val);  // sign-extend
            return pc + 4;
        }
        case 0x1: // LH
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            uint32_t addr = rs1_val + (uint32_t)instr->imm;
            uint16_t half_val;
            if (!mem_try_read16(mem, addr, &half_val)) {
                return trap_enter(cpu, pc, 5, addr); // load access fault
            }
            cpu_write_reg(cpu, instr->rd, (uint32_t)(int32_t)(int16_t)half_val); // sign extended
            return pc + 4;
        }
        case 0x2: // LW
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            uint32_t addr = rs1_val + (uint32_t)instr->imm;
            uint32_t word;
            if (!mem_try_read32(mem, addr, &word)) {
                return trap_enter(cpu, pc, 5, addr); // load access fault
            }
            cpu_write_reg(cpu, instr->rd, word);
            return pc + 4;
        }
        case 0x4: // LBU
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            uint32_t addr = rs1_val + (uint32_t)instr->imm;
            uint8_t byte_val;
            if (!mem_try_read8(mem, addr, &byte_val)) {
                return trap_enter(cpu, pc, 5, addr); // load access fault
            }
            cpu_write_reg(cpu, instr->rd, (uint32_t)byte_val);           // zero-extend
            return pc + 4;
        }
        case 0x5: // LHU
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            uint32_t addr = rs1_val + (uint32_t)instr->imm;
            uint16_t half_val;
            if (!mem_try_read16(mem, addr, &half_val)) {
                return trap_enter(cpu, pc, 5, addr); // load access fault
            }
            cpu_write_reg(cpu, instr->rd, (uint32_t)half_val); //zero-extended
            return pc + 4;
        }
        default:
            return trap_enter(cpu, pc, 2, instr->raw); // illegal instruction
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
            if (!mem_try_write8(mem, addr, (uint8_t)rs2_val)) {
                return trap_enter(cpu, pc, 7, addr); // store/AMO access fault
            }
            return pc + 4;
        }
        case 0x1: // SH
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            uint32_t addr = rs1_val + (uint32_t)instr->imm;
            uint32_t rs2_val = cpu_read_reg(cpu, instr->rs2);
            if (!mem_try_write16(mem, addr, (uint16_t)rs2_val)) {
                return trap_enter(cpu, pc, 7, addr); // store/AMO access fault
            }
            return pc + 4;
        }
        case 0x2: // SW
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            uint32_t addr = rs1_val + (uint32_t)instr->imm;
            uint32_t rs2_val = cpu_read_reg(cpu, instr->rs2);
            if (!mem_try_write32(mem, addr, rs2_val)) {
                return trap_enter(cpu, pc, 7, addr); // store/AMO access fault
            }
            return pc + 4;
        }
        default:
            return trap_enter(cpu, pc, 2, instr->raw); // illegal instruction
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
                return take_branch(cpu, pc, instr);
            }
            return pc + 4;
        }
        case 0x1: // BNE
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            uint32_t rs2_val = cpu_read_reg(cpu, instr->rs2);
            if(rs1_val != rs2_val) {
                return take_branch(cpu, pc, instr);
            }
            return pc + 4;
        }
        case 0x4: // BLT
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            uint32_t rs2_val = cpu_read_reg(cpu, instr->rs2);
            if((int32_t)rs1_val < (int32_t)rs2_val) {
                return take_branch(cpu, pc, instr);
            }
            return pc + 4;
        }
        case 0x5: // BGE
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            uint32_t rs2_val = cpu_read_reg(cpu, instr->rs2);
            if((int32_t)rs1_val >= (int32_t)rs2_val) {
                return take_branch(cpu, pc, instr);
            }
            return pc + 4;
        }
        case 0x6: // BLTU
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            uint32_t rs2_val = cpu_read_reg(cpu, instr->rs2);
            if(rs1_val < rs2_val) {
                return take_branch(cpu, pc, instr);
            }
            return pc + 4;
        }
        case 0x7: // BGEU
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            uint32_t rs2_val = cpu_read_reg(cpu, instr->rs2);
            if(rs1_val >= rs2_val) {
                return take_branch(cpu, pc, instr);
            }
            return pc + 4;
        }
        default:
            return trap_enter(cpu, pc, 2, instr->raw); // illegal instruction
    }
    return pc + 4;
}

static uint32_t execute_jal(CPU *cpu, Memory *mem, DecodedInstruction *instr, uint32_t pc) {
    (void)mem;
    uint32_t target = pc + (uint32_t)instr->imm;
    if (target_misaligned(target)) {
        return trap_enter(cpu, pc, 0, target); // instruction address misaligned
    }
    cpu_write_reg(cpu, instr->rd, pc + 4);
    return target;
}

static uint32_t execute_jalr(CPU *cpu, Memory *mem, DecodedInstruction *instr, uint32_t pc) {
    (void)mem;
    switch (instr->funct3) {
        case 0x0:
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            uint32_t target = (rs1_val + (uint32_t)instr->imm) & ~1u;
            if (target_misaligned(target)) {
                return trap_enter(cpu, pc, 0, target); // instruction address misaligned
            }
            cpu_write_reg(cpu, instr->rd, pc + 4);
            return target;
        }
        default:
            return trap_enter(cpu, pc, 2, instr->raw); // illegal instruction
    }
    return pc + 4;
}

static uint32_t execute_fence(CPU *cpu, Memory *mem, DecodedInstruction *instr, uint32_t pc) {
    (void)cpu;
    (void)mem;
    switch (instr->funct3) {
        case 0x0:
            return pc + 4; // no-op: single-hart, in-order emulator
        case 0x1:
            // FENCE.I: no-op, there is no instruction cache and every fetch reads
            // memory, so stores are always visible to later fetches. The spec says
            // base implementations ignore funct12, rs1 and rd (zifencei.adoc).
            return pc + 4;
        default:
            return trap_enter(cpu, pc, 2, instr->raw); // illegal instruction
    }
    return pc + 4;
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
    if (csr_is_read_only(csr_addr) && csr_instr_writes(instr)) {
        return trap_enter(cpu, pc, 2, instr->raw); // write to a read-only CSR: illegal instruction
    }
    switch (instr->funct3) {
        case 0x0:
            if(instr->imm == 0) { // ECALL
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
            }
            LOG_FATAL("unexpected imm for opcode 0x73 funct3 0x0: imm=0x%x", instr->imm);

        break;
        case 0x1: // CSRRW
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            uint32_t old = cpu_read_csr(cpu, csr_addr);
            cpu_write_csr(cpu, csr_addr, rs1_val);
            cpu_write_reg(cpu, instr->rd, old);
            return pc + 4;
        }
        case 0x2: // CSRRS
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            uint32_t old = cpu_read_csr(cpu, csr_addr);
            if (instr->rs1 != 0) { // rs1 = x0: no write at all
                cpu_write_csr(cpu, csr_addr, old | rs1_val);
            }
            cpu_write_reg(cpu, instr->rd, old);
            return pc + 4;
        }
        case 0x3: // CSRRC
        {
            uint32_t rs1_val = cpu_read_reg(cpu, instr->rs1);
            uint32_t old = cpu_read_csr(cpu, csr_addr);
            if (instr->rs1 != 0) { // rs1 = x0: no write at all
                cpu_write_csr(cpu, csr_addr, old & ~rs1_val);
            }
            cpu_write_reg(cpu, instr->rd, old);
            return pc + 4;
        }
        case 0x5: // CSRRWI
        {
            uint32_t old = cpu_read_csr(cpu, csr_addr);
            cpu_write_csr(cpu, csr_addr, (uint32_t)instr->rs1); // rs1 field holds uimm here
            cpu_write_reg(cpu, instr->rd, old);
            return pc + 4;
        }
        case 0x6: // CSRRSI
        {
            uint32_t old = cpu_read_csr(cpu, csr_addr);
            if (instr->rs1 != 0) { // uimm = 0: no write at all
                cpu_write_csr(cpu, csr_addr, old | (uint32_t)instr->rs1);
            }
            cpu_write_reg(cpu, instr->rd, old);
            return pc + 4;
        }
        case 0x7: // CSRRCI
        {
            uint32_t old = cpu_read_csr(cpu, csr_addr);
            if (instr->rs1 != 0) { // uimm = 0: no write at all
                cpu_write_csr(cpu, csr_addr, old & ~(uint32_t)instr->rs1);
            }
            cpu_write_reg(cpu, instr->rd, old);
            return pc + 4;
        }
        default:
            return trap_enter(cpu, pc, 2, instr->raw); // illegal instruction
    }
    return pc + 4;
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
            return false;
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

// IALIGN=32: a taken branch/jump target must be 4-byte aligned
static bool target_misaligned(uint32_t target) {
    return (target & 0x3u) != 0;
}

// Target of a taken conditional branch, or a misaligned-instruction trap
static uint32_t take_branch(CPU *cpu, uint32_t pc, DecodedInstruction *instr) {
    uint32_t target = pc + (uint32_t)instr->imm;
    if (target_misaligned(target)) {
        return trap_enter(cpu, pc, 0, target); // instruction address misaligned
    }
    return target;
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
        default:
            cpu->pc = trap_enter(cpu, pc, 2, instr->raw); // illegal instruction
        break;
    }
}

void cpu_step(CPU *cpu, Memory *mem) {
    cpu->trapped = false;
    cpu->mcycle_written = false;
    cpu->minstret_written = false;

    uint32_t raw;
    if (!mem_try_read32(mem, cpu->pc, &raw)) {
        cpu->pc = trap_enter(cpu, cpu->pc, 1, cpu->pc); // instruction access fault
    } else {
        DecodedInstruction instr = decode_instruction(raw);
        execute(cpu, mem, &instr);
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
