#include "unity.h"
#include "../src/execute.h"

void setUp(void) {} // Unity calles this before each test function
void tearDown(void) {} // Unity calls this after eacht test function


void test_addi(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);
    mem_write32(&mem, 0, 0x00500093);  // addi x1, x0, 5

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(5, cpu_read_reg(&cpu, 1));
    TEST_ASSERT_EQUAL_UINT32(4, cpu.pc);

    mem_free(&mem);
}

void test_slti_true(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_reg(&cpu, 1, 3);              // set up x1 directly, bypassing decode/execute
    mem_write32(&mem, 0, 0x0050A113);       // SLTI x2, x1, 5

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(1, cpu_read_reg(&cpu, 2));
    TEST_ASSERT_EQUAL_UINT32(4, cpu.pc);
    mem_free(&mem);
}

void test_slti_false(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_reg(&cpu, 1, 5); // set up x1 directly bypassing decode / execute
    mem_write32(&mem, 0, 0x0030A113); // SLTI x2, x1, 3

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(0, cpu_read_reg(&cpu, 2));
    TEST_ASSERT_EQUAL_UINT32(4, cpu.pc);
    mem_free(&mem);
}

void test_slti_signed_edge(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_reg(&cpu, 1, -1); // set up x1 directly bypassing decode / execute
    mem_write32(&mem, 0, 0x0000A113); // SLTI x2, x1, 0

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(1, cpu_read_reg(&cpu, 2));
    TEST_ASSERT_EQUAL_UINT32(4, cpu.pc);
    mem_free(&mem);
}

void test_sltiu_true(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_reg(&cpu, 1, 3); // set up x1 directly bypassing decode / execute
    mem_write32(&mem, 0, 0x0050B113); // SLTIU x2, x1, 5

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(1, cpu_read_reg(&cpu, 2));
    TEST_ASSERT_EQUAL_UINT32(4, cpu.pc);
    mem_free(&mem);
}

void test_sltiu_false(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_reg(&cpu, 1, 5); // set up x1 directly bypassing decode / execute
    mem_write32(&mem, 0, 0x0030B113); // SLTIU x2, x1, 3

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(0, cpu_read_reg(&cpu, 2));
    TEST_ASSERT_EQUAL_UINT32(4, cpu.pc);
    mem_free(&mem);

}

void test_sltiu_signed_edge(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_reg(&cpu, 1, -1); // set up x1 directly bypassing decode / execute
    mem_write32(&mem, 0, 0x0000B113); // SLTIU x2, x1, 0

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(0, cpu_read_reg(&cpu, 2));
    TEST_ASSERT_EQUAL_UINT32(4, cpu.pc);
    mem_free(&mem);
}



void test_xori(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_reg(&cpu, 1, 0xFF0);
    mem_write32(&mem, 0, 0x0FF0C113); // XORI x2, x1, 0x0FF

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(0xF0F, cpu_read_reg(&cpu,2));
    TEST_ASSERT_EQUAL_UINT32(4, cpu.pc);
    mem_free(&mem);
}

void test_ori(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_reg(&cpu, 1, 0xFF0);
    mem_write32(&mem, 0, 0x0FF0E113); // ORI x2, x1, 0x0FF

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(0xFFF, cpu_read_reg(&cpu,2));
    TEST_ASSERT_EQUAL_UINT32(4, cpu.pc);
    mem_free(&mem);
}

void test_andi(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_reg(&cpu, 1, 0xFF0);
    mem_write32(&mem, 0, 0x0FF0F113); // ANDI x2, x1, 0x0FF

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(0x0F0, cpu_read_reg(&cpu,2));
    TEST_ASSERT_EQUAL_UINT32(4, cpu.pc);
    mem_free(&mem);
}

void test_slli(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_reg(&cpu, 1, 0x0F);
    mem_write32(&mem, 0, 0x00409113); // SLLI x2, x1, 4

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(0xF0, cpu_read_reg(&cpu,2));
    TEST_ASSERT_EQUAL_UINT32(4, cpu.pc);
    mem_free(&mem);
}

void test_srli(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_reg(&cpu, 1, 0x80000000);
    mem_write32(&mem, 0, 0x0040D113); // SRLI x2, x1, 4

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(0x08000000, cpu_read_reg(&cpu,2));
    TEST_ASSERT_EQUAL_UINT32(4, cpu.pc);
    mem_free(&mem);
}

void test_srai(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_reg(&cpu, 1, 0x80000000);
    mem_write32(&mem, 0, 0x4040D113); // SRAI x2, x1, 4

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(0xF8000000, cpu_read_reg(&cpu,2));
    TEST_ASSERT_EQUAL_UINT32(4, cpu.pc);
    mem_free(&mem);
}

void test_add(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_reg(&cpu, 1, 3);
    cpu_write_reg(&cpu, 2, 4);
    mem_write32(&mem, 0, 0x002081B3); // ADD x3, x1, x2

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(7, cpu_read_reg(&cpu,3));
    TEST_ASSERT_EQUAL_UINT32(4, cpu.pc);
    mem_free(&mem);
}


void test_sub(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_reg(&cpu, 1, 10);
    cpu_write_reg(&cpu, 2, 4);
    mem_write32(&mem, 0, 0x402081B3); // SUB x3, x1, x2

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(6, cpu_read_reg(&cpu,3));
    TEST_ASSERT_EQUAL_UINT32(4, cpu.pc);
    mem_free(&mem);
}


void test_sll(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_reg(&cpu, 1, 0x0F);
    cpu_write_reg(&cpu, 2, 4);
    mem_write32(&mem, 0, 0x002091B3); // SLL x3, x1, x2

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(0xF0, cpu_read_reg(&cpu,3));
    TEST_ASSERT_EQUAL_UINT32(4, cpu.pc);
    mem_free(&mem);
}

void test_slt_true(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_reg(&cpu, 1, 3);
    cpu_write_reg(&cpu, 2, 5);
    mem_write32(&mem, 0, 0x0020A1B3); // SLT x3, x1, x2

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(1, cpu_read_reg(&cpu,3));
    TEST_ASSERT_EQUAL_UINT32(4, cpu.pc);
    mem_free(&mem);
}

void test_slt_false(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_reg(&cpu, 1, 5);
    cpu_write_reg(&cpu, 2, 3);
    mem_write32(&mem, 0, 0x0020A1B3); // SLT x3, x1, x2

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(0, cpu_read_reg(&cpu,3));
    TEST_ASSERT_EQUAL_UINT32(4, cpu.pc);
    mem_free(&mem);
}

void test_slt_signed_edge(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_reg(&cpu, 1, -1);
    cpu_write_reg(&cpu, 2, 0);
    mem_write32(&mem, 0, 0x0020A1B3); // SLT x3, x1, x2

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(1, cpu_read_reg(&cpu,3));
    TEST_ASSERT_EQUAL_UINT32(4, cpu.pc);
    mem_free(&mem);
}

void test_sltu_true(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_reg(&cpu, 1, 3);
    cpu_write_reg(&cpu, 2, 5);
    mem_write32(&mem, 0, 0x0020B1B3); // SLTU x3, x1, x2

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(1, cpu_read_reg(&cpu,3));
    TEST_ASSERT_EQUAL_UINT32(4, cpu.pc);
    mem_free(&mem);
}

void test_sltu_false(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_reg(&cpu, 1, 5);
    cpu_write_reg(&cpu, 2, 3);
    mem_write32(&mem, 0, 0x0020B1B3); // SLTU x3, x1, x2

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(0, cpu_read_reg(&cpu,3));
    TEST_ASSERT_EQUAL_UINT32(4, cpu.pc);
    mem_free(&mem);
}

void test_sltu_signed_edge(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_reg(&cpu, 1, -1);
    cpu_write_reg(&cpu, 2, 0);
    mem_write32(&mem, 0, 0x0020B1B3); // SLTU x3, x1, x2

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(0, cpu_read_reg(&cpu,3));
    TEST_ASSERT_EQUAL_UINT32(4, cpu.pc);
    mem_free(&mem);
}

void test_xor(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_reg(&cpu, 1, 0xFF0);
    cpu_write_reg(&cpu, 2, 0x0FF);
    mem_write32(&mem, 0, 0x0020C1B3); // XOR x3, x1, x2

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(0xF0F, cpu_read_reg(&cpu,3));
    TEST_ASSERT_EQUAL_UINT32(4, cpu.pc);
    mem_free(&mem);
}

void test_srl(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_reg(&cpu, 1, 0x80000000);
    cpu_write_reg(&cpu, 2, 4);
    mem_write32(&mem, 0, 0x0020D1B3); // SRL x3, x1, x2

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(0x08000000, cpu_read_reg(&cpu,3));
    TEST_ASSERT_EQUAL_UINT32(4, cpu.pc);
    mem_free(&mem);
}

void test_sra(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_reg(&cpu, 1, 0x80000000);
    cpu_write_reg(&cpu, 2, 4);
    mem_write32(&mem, 0, 0x4020D1B3); // SRA x3, x1, x2

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(0xF8000000, cpu_read_reg(&cpu,3));
    TEST_ASSERT_EQUAL_UINT32(4, cpu.pc);
    mem_free(&mem);
}

void test_or(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_reg(&cpu, 1, 0xFF0);
    cpu_write_reg(&cpu, 2, 0x0FF);
    mem_write32(&mem, 0, 0x0020E1B3); // OR x3, x1, x2

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(0xFFF, cpu_read_reg(&cpu,3));
    TEST_ASSERT_EQUAL_UINT32(4, cpu.pc);
    mem_free(&mem);
}

void test_and(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_reg(&cpu, 1, 0xFF0);
    cpu_write_reg(&cpu, 2, 0x0FF);
    mem_write32(&mem, 0, 0x0020F1B3); // AND x3, x1, x2

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(0x0F0, cpu_read_reg(&cpu,3));
    TEST_ASSERT_EQUAL_UINT32(4, cpu.pc);
    mem_free(&mem);
}

void test_lui(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    mem_write32(&mem, 0, 0x123450B7); // LUI x1, 0x12345

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(0x12345000, cpu_read_reg(&cpu,1));
    TEST_ASSERT_EQUAL_UINT32(4, cpu.pc);
    mem_free(&mem);
}

void test_auipc(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    mem_write32(&mem, 0, 0x00002117); // AUIPC x2, 0x2

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(0x00002000, cpu_read_reg(&cpu,2));
    TEST_ASSERT_EQUAL_UINT32(4, cpu.pc);
    mem_free(&mem);
}

void test_lb(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_reg(&cpu, 1, 8); // x1 = base address 8
    mem_write8(&mem, 8, 0xFF);
    mem_write32(&mem, 0, 0x00008103); // LB x2, 0(x1)

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(0xFFFFFFFF, cpu_read_reg(&cpu,2));
    TEST_ASSERT_EQUAL_UINT32(4, cpu.pc);
    mem_free(&mem);
}

void test_lbu(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_reg(&cpu, 1, 8); // x1 = base address 8
    mem_write8(&mem, 8, 0xFF);
    mem_write32(&mem, 0, 0x0000C103); // LBU x2, 0(x1)

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(0x000000FF, cpu_read_reg(&cpu,2));
    TEST_ASSERT_EQUAL_UINT32(4, cpu.pc);
    mem_free(&mem);
}

void test_lh(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_reg(&cpu, 1, 8); // x1 = base address 8
    mem_write16(&mem, 8, 0xFFFF);
    mem_write32(&mem, 0, 0x00009103); // LH x2, 0(x1)

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(0xFFFFFFFF, cpu_read_reg(&cpu,2));
    TEST_ASSERT_EQUAL_UINT32(4, cpu.pc);
    mem_free(&mem);
}

void test_lhu(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_reg(&cpu, 1, 8); // x1 = base address 8
    mem_write16(&mem, 8, 0xFFFF);
    mem_write32(&mem, 0, 0x0000D103); // LHU x2, 0(x1)

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(0x0000FFFF, cpu_read_reg(&cpu,2));
    TEST_ASSERT_EQUAL_UINT32(4, cpu.pc);
    mem_free(&mem);
}

void test_lw(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_reg(&cpu, 1, 8); // x1 = base address 8
    mem_write32(&mem, 8, 0xDEADBEEF);
    mem_write32(&mem, 0, 0x0000A103); // LW x2, 0(x1)

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(0xDEADBEEF, cpu_read_reg(&cpu,2));
    TEST_ASSERT_EQUAL_UINT32(4, cpu.pc);
    mem_free(&mem);
}

void test_sb(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_reg(&cpu, 1, 4);          // x1 = base address 4
    cpu_write_reg(&cpu, 2, 0x123456AB); // x2 = value to store
    mem_write32(&mem, 0, 0x00208423);   // SB x2, 8(x1)

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(0xAB, mem_read8(&mem, 12));
    TEST_ASSERT_EQUAL_UINT32(4, cpu.pc);
    mem_free(&mem);
}

void test_sh(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_reg(&cpu, 1, 4);          // x1 = base address 4
    cpu_write_reg(&cpu, 2, 0x1234BEEF); // x2 = value to store
    mem_write32(&mem, 0, 0x00209423);   // SH x2, 8(x1)

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(0xBEEF, mem_read16(&mem, 12));
    TEST_ASSERT_EQUAL_UINT32(4, cpu.pc);
    mem_free(&mem);
}

void test_sw(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_reg(&cpu, 1, 4);          // x1 = base address 4
    cpu_write_reg(&cpu, 2, 0xDEADBEEF); // x2 = value to store
    mem_write32(&mem, 0, 0x0020A423);   // SW x2, 8(x1)

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(0xDEADBEEF, mem_read32(&mem, 12));
    TEST_ASSERT_EQUAL_UINT32(4, cpu.pc);
    mem_free(&mem);
}

void test_beq_taken(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_reg(&cpu, 1, 5); // x1 = 5
    cpu_write_reg(&cpu, 2, 5); // x2 = 5
    mem_write32(&mem, 0, 0x00208463); // BEQ x1, x2, 8

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(8, cpu.pc);
    mem_free(&mem);
}

void test_beq_not_taken(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_reg(&cpu, 1, 5); // x1 = 5
    cpu_write_reg(&cpu, 2, 9); // x2 = 9
    mem_write32(&mem, 0, 0x00208463); // BEQ x1, x2, 8

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(4, cpu.pc);
    mem_free(&mem);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_addi);
    RUN_TEST(test_slti_true);
    RUN_TEST(test_slti_false);
    RUN_TEST(test_slti_signed_edge);
    RUN_TEST(test_sltiu_true);
    RUN_TEST(test_sltiu_false);
    RUN_TEST(test_sltiu_signed_edge);
    RUN_TEST(test_xori);
    RUN_TEST(test_ori);
    RUN_TEST(test_andi);
    RUN_TEST(test_slli);
    RUN_TEST(test_srli);
    RUN_TEST(test_srai);
    RUN_TEST(test_add);
    RUN_TEST(test_sub);
    RUN_TEST(test_sll);
    RUN_TEST(test_slt_true);
    RUN_TEST(test_slt_false);
    RUN_TEST(test_slt_signed_edge);
    RUN_TEST(test_sltu_true);
    RUN_TEST(test_sltu_false);
    RUN_TEST(test_sltu_signed_edge);
    RUN_TEST(test_xor);
    RUN_TEST(test_srl);
    RUN_TEST(test_sra);
    RUN_TEST(test_or);
    RUN_TEST(test_and);
    RUN_TEST(test_lui);
    RUN_TEST(test_auipc);
    RUN_TEST(test_lb);
    RUN_TEST(test_lbu);
    RUN_TEST(test_lh);
    RUN_TEST(test_lhu);
    RUN_TEST(test_lw);
    RUN_TEST(test_sb);
    RUN_TEST(test_sh);
    RUN_TEST(test_sw);
    RUN_TEST(test_beq_taken);
    RUN_TEST(test_beq_not_taken);
	return UNITY_END();
}
