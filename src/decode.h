#pragma once
#include <stdint.h>

typedef enum {
    FMT_R,
    FMT_I,
    FMT_S,
    FMT_B,
    FMT_U,
    FMT_J,
    FMT_UNKNOWN, // opcode did not match any known formats
} InstrFormat;

typedef struct {
    uint32_t raw;    // the original instruction bits: 32 bits, or the 16-bit form of a compressed instruction
    uint8_t length;  // instruction length in bytes: 4 (2 for a compressed instruction, from step 3)

    InstrFormat format;

    uint8_t opcode; // bits [6:0] always present
    uint8_t rd;     // bits [11:7]  - valid for R/I/U/J
    uint8_t rs1;    // bits [19:15] - valid for R/I/S/B
    uint8_t rs2;    // bits [24:20] - valid for R/S/B
    uint8_t funct3; // bits [14:12] - valid for R/I/S/B
    uint8_t funct7; // bits [31:25] - valid for R only

    int32_t imm;    // sign extended immediate, format dependent extraction 
                    // 0/ unused for R-type
} DecodedInstruction;

DecodedInstruction decode_instruction(uint32_t raw);
int32_t get_immediate(InstrFormat fmt, uint32_t raw);