#include "compressed.h"

// Fields of a compressed instruction, named as in the manual's tables.
#define BITS(h, hi, lo) (((uint32_t)(h) >> (lo)) & ((1u << ((hi) - (lo) + 1)) - 1u))
#define BIT(h, n)       (((uint32_t)(h) >> (n)) & 1u)

#define OP_LOAD   0x03u
#define OP_IMM    0x13u
#define OP_LUI    0x37u
#define OP_STORE  0x23u
#define OP_REG    0x33u
#define OP_BRANCH 0x63u
#define OP_JALR   0x67u
#define OP_JAL    0x6Fu

// 3-bit register fields name x8..x15
static uint32_t reg3(uint32_t field) { return 8u + field; }

// Sign-extend the low `bits` bits of `value`
static int32_t sext(uint32_t value, unsigned bits) {
    uint32_t sign = 1u << (bits - 1);
    return (int32_t)((value ^ sign) - sign);
}

// The base-ISA encodings the compressed instructions expand to
static uint32_t enc_i(int32_t imm, uint32_t rs1, uint32_t funct3, uint32_t rd, uint32_t op) {
    return ((uint32_t)imm & 0xFFFu) << 20 | rs1 << 15 | funct3 << 12 | rd << 7 | op;
}

static uint32_t enc_s(int32_t imm, uint32_t rs2, uint32_t rs1, uint32_t funct3) {
    uint32_t i = (uint32_t)imm & 0xFFFu;
    return (i >> 5) << 25 | rs2 << 20 | rs1 << 15 | funct3 << 12 | (i & 0x1Fu) << 7 | OP_STORE;
}

static uint32_t enc_r(uint32_t funct7, uint32_t rs2, uint32_t rs1, uint32_t funct3, uint32_t rd) {
    return funct7 << 25 | rs2 << 20 | rs1 << 15 | funct3 << 12 | rd << 7 | OP_REG;
}

static uint32_t enc_b(int32_t offset, uint32_t rs2, uint32_t rs1, uint32_t funct3) {
    uint32_t o = (uint32_t)offset;
    return ((o >> 12) & 1u) << 31 | ((o >> 5) & 0x3Fu) << 25 | rs2 << 20 | rs1 << 15 | funct3 << 12 |
           ((o >> 1) & 0xFu) << 8 | ((o >> 11) & 1u) << 7 | OP_BRANCH;
}

static uint32_t enc_j(int32_t offset, uint32_t rd) {
    uint32_t o = (uint32_t)offset;
    return ((o >> 20) & 1u) << 31 | ((o >> 1) & 0x3FFu) << 21 | ((o >> 11) & 1u) << 20 |
           ((o >> 12) & 0xFFu) << 12 | rd << 7 | OP_JAL;
}

static bool expand_quadrant0(uint16_t h, uint32_t *word) {
    uint32_t rd_rs2 = reg3(BITS(h, 4, 2));      // rd' or rs2'
    uint32_t rs1 = reg3(BITS(h, 9, 7));
    // C.LW and C.SW: uimm[5:3] = h[12:10], uimm[2] = h[6], uimm[6] = h[5]
    int32_t offset = (int32_t)(BITS(h, 12, 10) << 3 | BIT(h, 6) << 2 | BIT(h, 5) << 6);
    switch (BITS(h, 15, 13)) {
        case 0x0: {   // C.ADDI4SPN: nzuimm[5:4] = h[12:11], [9:6] = h[10:7], [2] = h[6], [3] = h[5]
            int32_t nzuimm = (int32_t)(BITS(h, 12, 11) << 4 | BITS(h, 10, 7) << 6 | BIT(h, 6) << 2 | BIT(h, 5) << 3);
            if (nzuimm == 0) {
                return false;                   // reserved; this includes the all-zero halfword
            }
            *word = enc_i(nzuimm, 2, 0x0, rd_rs2, OP_IMM);
            return true;
        }
        case 0x2:     // C.LW
            *word = enc_i(offset, rs1, 0x2, rd_rs2, OP_LOAD);
            return true;
        case 0x6:     // C.SW
            *word = enc_s(offset, rd_rs2, rs1, 0x2);
            return true;
        default:      // C.FLD, C.FLW, C.FSD, C.FSW (no F or D), C.LD, C.SD (RV64) and the reserved 100
            return false;
    }
}

static bool expand_quadrant1(uint16_t h, uint32_t *word) {
    uint32_t rd = BITS(h, 11, 7);
    int32_t imm6 = sext(BIT(h, 12) << 5 | BITS(h, 6, 2), 6);
    switch (BITS(h, 15, 13)) {
        case 0x0:     // C.ADDI (C.NOP when rd = x0; the other rd = x0 or imm = 0 forms are HINTs)
            *word = enc_i(imm6, rd, 0x0, rd, OP_IMM);
            return true;
        case 0x1: {   // C.JAL: offset[11|4|9:8|10|6|7|3:1|5] = h[12:2]
            int32_t offset = sext(BIT(h, 12) << 11 | BIT(h, 11) << 4 | BITS(h, 10, 9) << 8 | BIT(h, 8) << 10 |
                                  BIT(h, 7) << 6 | BIT(h, 6) << 7 | BITS(h, 5, 3) << 1 | BIT(h, 2) << 5, 12);
            *word = enc_j(offset, 1);
            return true;
        }
        case 0x2:     // C.LI
            *word = enc_i(imm6, 0, 0x0, rd, OP_IMM);
            return true;
        case 0x3:
            if (rd == 2) {   // C.ADDI16SP: nzimm[9|4|6|8:7|5] = h[12|6|5|4:3|2]
                int32_t nzimm = sext(BIT(h, 12) << 9 | BIT(h, 6) << 4 | BIT(h, 5) << 6 |
                                     BITS(h, 4, 3) << 7 | BIT(h, 2) << 5, 10);
                if (nzimm == 0) {
                    return false;
                }
                *word = enc_i(nzimm, 2, 0x0, 2, OP_IMM);
                return true;
            }
            {   // C.LUI: nzimm[17] = h[12], nzimm[16:12] = h[6:2]; rd = x0 is a HINT
                uint32_t field = BIT(h, 12) << 5 | BITS(h, 6, 2);
                if (field == 0) {
                    return false;
                }
                *word = (uint32_t)sext(field, 6) << 12 | rd << 7 | OP_LUI;
                return true;
            }
        case 0x4: {
            uint32_t r = reg3(BITS(h, 9, 7));   // rd'/rs1'
            uint32_t shamt = BIT(h, 12) << 5 | BITS(h, 6, 2);
            switch (BITS(h, 11, 10)) {
                case 0x0:   // C.SRLI
                case 0x1:   // C.SRAI
                    if (BIT(h, 12)) {
                        return false;           // shamt[5] = 1 is not defined on RV32
                    }
                    *word = enc_i((int32_t)(shamt | (BITS(h, 11, 10) == 1 ? 0x400u : 0u)), r, 0x5, r, OP_IMM);
                    return true;
                case 0x2:   // C.ANDI
                    *word = enc_i(imm6, r, 0x7, r, OP_IMM);
                    return true;
                default: {  // C.SUB, C.XOR, C.OR, C.AND; with bit 12 set these are C.SUBW, C.ADDW and reserved
                    if (BIT(h, 12)) {
                        return false;
                    }
                    uint32_t rs2 = reg3(BITS(h, 4, 2));
                    static const uint32_t funct7[4] = {0x20, 0x00, 0x00, 0x00};
                    static const uint32_t funct3[4] = {0x0, 0x4, 0x6, 0x7};
                    uint32_t k = BITS(h, 6, 5);
                    *word = enc_r(funct7[k], rs2, r, funct3[k], r);
                    return true;
                }
            }
        }
        case 0x5: {   // C.J: same offset layout as C.JAL
            int32_t offset = sext(BIT(h, 12) << 11 | BIT(h, 11) << 4 | BITS(h, 10, 9) << 8 | BIT(h, 8) << 10 |
                                  BIT(h, 7) << 6 | BIT(h, 6) << 7 | BITS(h, 5, 3) << 1 | BIT(h, 2) << 5, 12);
            *word = enc_j(offset, 0);
            return true;
        }
        default: {    // C.BEQZ (110), C.BNEZ (111): offset[8|4:3] = h[12:10], offset[7:6|2:1|5] = h[6:2]
            int32_t offset = sext(BIT(h, 12) << 8 | BITS(h, 11, 10) << 3 | BITS(h, 6, 5) << 6 |
                                  BITS(h, 4, 3) << 1 | BIT(h, 2) << 5, 9);
            *word = enc_b(offset, 0, reg3(BITS(h, 9, 7)), BIT(h, 13) ? 0x1u : 0x0u);
            return true;
        }
    }
}

static bool expand_quadrant2(uint16_t h, uint32_t *word) {
    uint32_t rd = BITS(h, 11, 7);               // rd, rs1 or rd/rs1
    uint32_t rs2 = BITS(h, 6, 2);
    switch (BITS(h, 15, 13)) {
        case 0x0:     // C.SLLI (rd = x0 or shamt = 0 are HINTs)
            if (BIT(h, 12)) {
                return false;                   // shamt[5] = 1 is not defined on RV32
            }
            *word = enc_i((int32_t)rs2, rd, 0x1, rd, OP_IMM);
            return true;
        case 0x2:     // C.LWSP: uimm[5] = h[12], uimm[4:2] = h[6:4], uimm[7:6] = h[3:2]
            if (rd == 0) {
                return false;
            }
            *word = enc_i((int32_t)(BIT(h, 12) << 5 | BITS(h, 6, 4) << 2 | BITS(h, 3, 2) << 6), 2, 0x2, rd, OP_LOAD);
            return true;
        case 0x4:
            if (!BIT(h, 12)) {
                if (rs2 == 0) {                 // C.JR
                    if (rd == 0) {
                        return false;
                    }
                    *word = enc_i(0, rd, 0x0, 0, OP_JALR);
                } else {                        // C.MV (rd = x0 is a HINT)
                    *word = enc_r(0, rs2, 0, 0x0, rd);
                }
            } else if (rs2 == 0) {
                if (rd == 0) {                  // C.EBREAK
                    *word = 0x00100073u;
                } else {                        // C.JALR
                    *word = enc_i(0, rd, 0x0, 1, OP_JALR);
                }
            } else {                            // C.ADD (rd = x0 is a HINT)
                *word = enc_r(0, rs2, rd, 0x0, rd);
            }
            return true;
        case 0x6:     // C.SWSP: uimm[5:2] = h[12:9], uimm[7:6] = h[8:7]
            *word = enc_s((int32_t)(BITS(h, 12, 9) << 2 | BITS(h, 8, 7) << 6), rs2, 2, 0x2);
            return true;
        default:      // C.FLDSP, C.FLWSP, C.FSDSP, C.FSWSP (no F or D), C.LDSP, C.SDSP (RV64)
            return false;
    }
}

bool expand_compressed(uint16_t half, uint32_t *word) {
    switch (half & 0x3u) {
        case 0x0: return expand_quadrant0(half, word);
        case 0x1: return expand_quadrant1(half, word);
        case 0x2: return expand_quadrant2(half, word);
        default:  return false;                 // 11: a 32-bit or longer instruction
    }
}
