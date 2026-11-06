#include "log.h"
#include "decode.h"


/**
 * R-type:  funct7[31:25] rs2[24:20] rs1[19:15] funct3[14:12] rd[11:7]  opcode[6:0]
 * I-type:  imm[31:20]               rs1[19:15] funct3[14:12] rd[11:7]  opcode[6:0]
 * S-type:  imm[31:25]    rs2[24:20] rs1[19:15] funct3[14:12] imm[11:7] opcode[6:0]
 * B-type:  imm[31|7]     rs2[24:20] rs1[19:15] funct3[14:12] imm[.]    opcode[6:0]
 * U-type:  imm[31:12]                                        rd[11:7]  opcode[6:0]
 * J-type:  imm[31|19:12|20|30:21]                            rd[11:7]  opcode[6:0]
 */

int32_t get_immediate(InstrFormat fmt, uint32_t raw) {
    int32_t imm = 0;
    uint32_t N = 0; // immediate field width
    switch(fmt) {
    case(FMT_R):
        imm   = 0;
        break;
    case(FMT_I):
	// 12 immediate bits
        imm = (raw >> 20) & 0xFFF;
	    // calculate sign extension
	    N = 12;
	    if(imm & 1u << (N-1)) {
	    	imm |= ~((1u << N) - 1);
	    }
        break;
    case(FMT_S):
	// 12 immediate bits
        imm = (((raw >> 25) & 0x7F) << 0x5) | ((raw >> 7) & 0x1F);
	    N = 12;
	    if(imm & 1u << (N-1)) {
	    	imm |= ~((1u << N) - 1);
	    }
        break;
    case(FMT_B):
	    N = 13;
        imm  = ((raw >> 31) & 0x01) << 12;
        imm |= ((raw >> 7) & 0x01) << 11;
        imm |= ((raw >> 25) & 0x3F) << 5;
        imm |= ((raw >> 8) & 0xF) << 1;

	    if(imm & 1u << (N-1)) {
	    	imm |= ~((1u << N) - 1);
        }
        break;
    case(FMT_U):
	    imm = raw & 0xFFFFF000;
        break;
    case(FMT_J):
        N = 21;
        imm = ((raw >> 31) & 0x01) << 20;
        imm |= ((raw >> 12) & 0xFF) << 12;
        imm |= ((raw >> 20) & 0x01) << 11;
        imm |= ((raw >> 21) & 0x3FF) << 1;

        if (imm & 1u << (N-1)) {
            imm |= ~((1u << N) - 1);
        }
        break;
    default:
        imm = 0;
    }
    return imm;
}

DecodedInstruction decode_instruction(uint32_t raw) {
    DecodedInstruction ret;

    // save the raw instruction
    ret.raw = raw;

    // extract bit values
    ret.opcode = raw & 0x7F;
    ret.rd     = (raw >> 7) & 0x1F;
    ret.funct3 = (raw >> 12) & 0x07;
    ret.rs1    = (raw >> 15) & 0x1F;
    ret.rs2    = (raw >> 20) & 0x1F;
    ret.funct7 = (raw >> 25) & 0x7F;
    ret.imm = 0; // to be computed below

    switch(ret.opcode) {
    case 0x37:
            // U type
            // LUI
            ret.format = FMT_U;
	        ret.imm = get_immediate(ret.format, raw);
            break;
    case 0x17:
            // U type
            // AUIPC
            ret.format = FMT_U;
	        ret.imm = get_immediate(ret.format, raw);
        break;
    case 0x6F:
            // J type
            // JAL
            ret.format = FMT_J;
	        ret.imm = get_immediate(ret.format, raw);
        break;
    case 0x67:
            // I type
            // JALR
            ret.format = FMT_I;
	        ret.imm = get_immediate(ret.format, raw);
        break;
    case 0x63:
            // B type
            // BEQ, BNE, BLT. BGE, BLTU, BGEU
            ret.format = FMT_B;
	        ret.imm = get_immediate(ret.format, raw);
        break;
    case 0x03:
            // I type
            // LB, LH, LW, LBU, LHU
            ret.format = FMT_I;
	        ret.imm = get_immediate(ret.format, raw);
        break;
    case 0x23:
            // S type
            // SB, SH, SW
            ret.format = FMT_S;
	        ret.imm = get_immediate(ret.format, raw);
        break;
    case 0x13:
            //  I type
            // ADDI, SLTI, SLTIU, XORI, ORI, ANDI, SLLI, SRLI, SRAI
            ret.format = FMT_I;
            ret.imm = get_immediate(ret.format, raw);
        break;
    case 0x33:
            // R type
            // ADD, SUB, SLL, SLT, SLTU, XOR, SRL, SRA, OR, AND
            ret.format = FMT_R;
	        ret.imm = get_immediate(ret.format, raw);
        break;
    case 0x0F:
            // FENCE
            // FENCE
            ret.format = FMT_UNKNOWN;
	        ret.imm = get_immediate(ret.format, raw);
        break;
    case 0x2F:
            // R type
            // AMOSWAP.W, AMOADD.W, ... (LR.W and SC.W share the opcode)
            ret.format = FMT_R;
	        ret.imm = get_immediate(ret.format, raw);
        break;
    case 0x73:
            // I type
            // ECALL, EBREAK and CSR instructions.
            ret.format = FMT_I;
	        ret.imm = get_immediate(ret.format, raw);
        break;
    default:
            // unknown opcode
            ret.format = FMT_I;
            ret.imm = get_immediate(ret.format, raw);
        break;

    }

    return ret;

}
