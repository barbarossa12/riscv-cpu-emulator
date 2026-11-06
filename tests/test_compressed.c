#include "unity.h"
#include "../src/compressed.h"

void setUp(void) {}
void tearDown(void) {}

#define SENTINEL 0xDEADBEEFu

/* Every row was made by assembling the compressed instruction (riscv64-unknown-elf-as, -march=rv32imac) for
 * the halfword, and the base-ISA instruction it stands for, with `.option norvc`, for the word. */
void test_expansions_match_the_assembler(void) {
    const struct { const char *name; uint16_t half; uint32_t word; } v[] = {
        {"c.addi4spn x8,x2,4", 0x0040, 0x00410413},
        {"c.addi4spn x10,x2,1020", 0x1FE8, 0x3FC10513},
        {"c.addi4spn x15,x2,340", 0x0ADC, 0x15410793},
        {"c.addi4spn x9,x2,680", 0x1524, 0x2A810493},
        {"c.lw x8,0(x9)", 0x4080, 0x0004A403},
        {"c.lw x15,124(x14)", 0x5F7C, 0x07C72783},
        {"c.lw x10,68(x11)", 0x41E8, 0x0445A503},
        {"c.lw x10,40(x11)", 0x5588, 0x0285A503},
        {"c.lw x10,4(x11)", 0x41C8, 0x0045A503},
        {"c.lw x10,64(x11)", 0x41A8, 0x0405A503},
        {"c.sw x8,0(x9)", 0xC080, 0x0084A023},
        {"c.sw x15,124(x14)", 0xDF7C, 0x06F72E23},
        {"c.sw x10,68(x11)", 0xC1E8, 0x04A5A223},
        {"c.sw x10,40(x11)", 0xD588, 0x02A5A423},
        {"c.sw x10,4(x11)", 0xC1C8, 0x00A5A223},
        {"c.sw x10,64(x11)", 0xC1A8, 0x04A5A023},
        {"c.nop", 0x0001, 0x00000013},
        {"c.addi x10,1", 0x0505, 0x00150513},
        {"c.addi x10,31", 0x057D, 0x01F50513},
        {"c.addi x10,-32", 0x1501, 0xFE050513},
        {"c.addi x31,-1", 0x1FFD, 0xFFFF8F93},
        {"c.jal .+2", 0x2009, 0x002000EF},
        {"c.jal .+2046", 0x2FFD, 0x7FE000EF},
        {"c.jal .-2048", 0x3001, 0x801FF0EF},
        {"c.jal .+682", 0x246D, 0x2AA000EF},
        {"c.jal .+1364", 0x2B91, 0x554000EF},
        {"c.jal .-682", 0x3B99, 0xD57FF0EF},
        {"c.li x10,0", 0x4501, 0x00000513},
        {"c.li x10,31", 0x457D, 0x01F00513},
        {"c.li x10,-32", 0x5501, 0xFE000513},
        {"c.li x31,-1", 0x5FFD, 0xFFF00F93},
        {"c.addi16sp x2,16", 0x6141, 0x01010113},
        {"c.addi16sp x2,496", 0x617D, 0x1F010113},
        {"c.addi16sp x2,-512", 0x7101, 0xE0010113},
        {"c.addi16sp x2,-16", 0x717D, 0xFF010113},
        {"c.addi16sp x2,176", 0x614D, 0x0B010113},
        {"c.lui x10,1", 0x6505, 0x00001537},
        {"c.lui x10,31", 0x657D, 0x0001F537},
        {"c.lui x10,0xfffe0", 0x7501, 0xFFFE0537},
        {"c.lui x10,0xfffe1", 0x7505, 0xFFFE1537},
        {"c.lui x31,0xfffff", 0x7FFD, 0xFFFFFFB7},
        {"c.srli x8,1", 0x8005, 0x00145413},
        {"c.srli x15,31", 0x83FD, 0x01F7D793},
        {"c.srai x8,1", 0x8405, 0x40145413},
        {"c.srai x15,31", 0x87FD, 0x41F7D793},
        {"c.andi x8,-32", 0x9801, 0xFE047413},
        {"c.andi x15,31", 0x8BFD, 0x01F7F793},
        {"c.andi x9,0", 0x8881, 0x0004F493},
        {"c.sub x8,x9", 0x8C05, 0x40940433},
        {"c.xor x10,x11", 0x8D2D, 0x00B54533},
        {"c.or x12,x13", 0x8E55, 0x00D66633},
        {"c.and x14,x15", 0x8F7D, 0x00F77733},
        {"c.j .+2", 0xA009, 0x0020006F},
        {"c.j .+2046", 0xAFFD, 0x7FE0006F},
        {"c.j .-2048", 0xB001, 0x801FF06F},
        {"c.j .+682", 0xA46D, 0x2AA0006F},
        {"c.j .-682", 0xBB99, 0xD57FF06F},
        {"c.beqz x8,.+2", 0xC009, 0x00040163},
        {"c.beqz x15,.+254", 0xCFFD, 0x0E078F63},
        {"c.beqz x10,.-256", 0xD101, 0xF00500E3},
        {"c.beqz x9,.+170", 0xC4CD, 0x0A048563},
        {"c.beqz x9,.-86", 0xD4CD, 0xFA0485E3},
        {"c.bnez x8,.+2", 0xE009, 0x00041163},
        {"c.bnez x15,.+254", 0xEFFD, 0x0E079F63},
        {"c.bnez x10,.-256", 0xF101, 0xF00510E3},
        {"c.bnez x9,.+84", 0xE8B1, 0x04049A63},
        {"c.slli x10,1", 0x0506, 0x00151513},
        {"c.slli x31,31", 0x0FFE, 0x01FF9F93},
        {"c.lwsp x10,0(x2)", 0x4502, 0x00012503},
        {"c.lwsp x31,252(x2)", 0x5FFE, 0x0FC12F83},
        {"c.lwsp x1,68(x2)", 0x4096, 0x04412083},
        {"c.lwsp x1,160(x2)", 0x508A, 0x0A012083},
        {"c.swsp x10,0(x2)", 0xC02A, 0x00A12023},
        {"c.swsp x31,252(x2)", 0xDFFE, 0x0FF12E23},
        {"c.swsp x1,68(x2)", 0xC286, 0x04112223},
        {"c.swsp x1,160(x2)", 0xD106, 0x0A112023},
        {"c.jr x1", 0x8082, 0x00008067},
        {"c.jr x31", 0x8F82, 0x000F8067},
        {"c.mv x10,x11", 0x852E, 0x00B00533},
        {"c.mv x31,x1", 0x8F86, 0x00100FB3},
        {"c.ebreak", 0x9002, 0x00100073},
        {"c.jalr x1", 0x9082, 0x000080E7},
        {"c.jalr x31", 0x9F82, 0x000F80E7},
        {"c.add x10,x11", 0x952E, 0x00B50533},
        {"c.add x31,x1", 0x9F86, 0x001F8FB3},
        {"c.addi x10,0", 0x0501, 0x00050513},
        {"c.addi x0,5", 0x0015, 0x00500013},
        {"c.li x0,7", 0x401D, 0x00700013},
        {"c.lui x0,3", 0x600D, 0x00003037},
        {"c.mv x0,x5", 0x8016, 0x00500033},
        {"c.add x0,x5", 0x9016, 0x00500033},
        {"c.slli x10,0", 0x0502, 0x00051513},
        {"c.slli x0,4", 0x0012, 0x00401013},
        {"c.srli x8,0", 0x8001, 0x00045413},
        {"c.srai x8,0", 0x8401, 0x40045413},
    };
    for (unsigned i = 0; i < sizeof v / sizeof v[0]; i++) {
        uint32_t word = SENTINEL;
        TEST_ASSERT_TRUE_MESSAGE(expand_compressed(v[i].half, &word), v[i].name);
        TEST_ASSERT_EQUAL_HEX32_MESSAGE(v[i].word, word, v[i].name);
    }
}

/* Reserved code points, instructions that need F, D or RV64, and anything that is not a 16-bit instruction. */
void test_illegal_halfwords_are_rejected_and_leave_the_word_alone(void) {
    const struct { const char *name; uint16_t half; } v[] = {
        {"all zero",                        0x0000},
        {"C.ADDI4SPN, nzuimm = 0",          0x0004},
        {"C.ADDI16SP, nzimm = 0",           0x6101},
        {"C.LUI, imm = 0, rd = x10",        0x6501},
        {"C.LUI, imm = 0, rd = x0",         0x6001},
        {"C.LWSP, rd = x0",                 0x4002},
        {"C.JR, rs1 = x0",                  0x8002},
        {"C.FLD (no D)",                    0x2000},
        {"C.FLW / C.LD (no F, RV32)",       0x6000},
        {"C.FSD (no D)",                    0xA000},
        {"C.FSW / C.SD (no F, RV32)",       0xE000},
        {"C.FLDSP (no D)",                  0x2002},
        {"C.FLWSP / C.LDSP (no F, RV32)",   0x6002},
        {"C.FSDSP (no D)",                  0xA002},
        {"C.FSWSP / C.SDSP (no F, RV32)",   0xE002},
        {"reserved, quadrant 0, funct3 100", 0x8000},
        {"C.ADDW (RV64 only)",              0x9C21},
        {"C.SUBW (RV64 only)",              0x9C01},
        {"reserved, 100 1 11 .. 10 01",     0x9C41},
        {"reserved, 100 1 11 .. 11 01",     0x9C61},
        {"C.SRLI, shamt = 32",              0x9001},
        {"C.SRAI, shamt = 32",              0x9401},
        {"C.SLLI, shamt = 32",              0x1502},
        {"C.SLLI, rd = x0, shamt = 32",     0x1002},
        {"all ones (bits 1:0 = 11)",        0xFFFF},
        {"start of a 32-bit instruction",   0x0003},
        {"start of a 32-bit instruction 2", 0x4003},
    };
    for (unsigned i = 0; i < sizeof v / sizeof v[0]; i++) {
        uint32_t word = SENTINEL;
        TEST_ASSERT_FALSE_MESSAGE(expand_compressed(v[i].half, &word), v[i].name);
        TEST_ASSERT_EQUAL_HEX32_MESSAGE(SENTINEL, word, v[i].name);
    }
}

/* Properties of the whole 16-bit space. 28823 is the number of halfwords that scripts/check-compressed.py finds
 * both the assembler and this expander accept, so it pins the legal set. */
void test_whole_halfword_space(void) {
    unsigned legal = 0;
    for (uint32_t h = 0; h < 0x10000; h++) {
        uint32_t word = SENTINEL;
        bool ok = expand_compressed((uint16_t)h, &word);
        if ((h & 3u) == 3u) {
            TEST_ASSERT_FALSE_MESSAGE(ok, "bits 1:0 = 11 is not a compressed instruction");
        }
        if (!ok) {
            TEST_ASSERT_EQUAL_HEX32(SENTINEL, word);
            continue;
        }
        legal++;
        TEST_ASSERT_EQUAL_HEX32_MESSAGE(3u, word & 3u, "an expansion is a 32-bit instruction");
        switch (word & 0x7Fu) {
            case 0x03: case 0x13: case 0x23: case 0x33: case 0x37: case 0x63: case 0x67: case 0x6F: case 0x73:
                break;
            default:
                TEST_FAIL_MESSAGE("expansion has an opcode no compressed instruction maps to");
        }
    }
    TEST_ASSERT_EQUAL_UINT(28823, legal);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_expansions_match_the_assembler);
    RUN_TEST(test_illegal_halfwords_are_rejected_and_leave_the_word_alone);
    RUN_TEST(test_whole_halfword_space);
    return UNITY_END();
}
