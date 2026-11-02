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

void test_bne_taken(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_reg(&cpu, 1, 5); // x1 = 5
    cpu_write_reg(&cpu, 2, 9); // x2 = 9
    mem_write32(&mem, 0, 0x00209463); // BNE x1, x2, 8

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(8, cpu.pc);
    mem_free(&mem);
}

void test_bne_not_taken(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_reg(&cpu, 1, 5); // x1 = 5
    cpu_write_reg(&cpu, 2, 5); // x2 = 5
    mem_write32(&mem, 0, 0x00209463); // BNE x1, x2, 8

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(4, cpu.pc);
    mem_free(&mem);
}

void test_blt_taken(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_reg(&cpu, 1, 0xFFFFFFFF); // x1 = -1
    cpu_write_reg(&cpu, 2, 1);          // x2 = 1
    mem_write32(&mem, 0, 0x0020C463);   // BLT x1, x2, 8

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(8, cpu.pc);
    mem_free(&mem);
}

void test_blt_not_taken(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_reg(&cpu, 1, 1);          // x1 = 1
    cpu_write_reg(&cpu, 2, 0xFFFFFFFF); // x2 = -1
    mem_write32(&mem, 0, 0x0020C463);   // BLT x1, x2, 8

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(4, cpu.pc);
    mem_free(&mem);
}

void test_bge_taken(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_reg(&cpu, 1, 1);          // x1 = 1
    cpu_write_reg(&cpu, 2, 0xFFFFFFFF); // x2 = -1
    mem_write32(&mem, 0, 0x0020D463);   // BGE x1, x2, 8

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(8, cpu.pc);
    mem_free(&mem);
}

void test_bge_not_taken(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_reg(&cpu, 1, 0xFFFFFFFF); // x1 = -1
    cpu_write_reg(&cpu, 2, 1);          // x2 = 1
    mem_write32(&mem, 0, 0x0020D463);   // BGE x1, x2, 8

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(4, cpu.pc);
    mem_free(&mem);
}

void test_bltu_taken(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_reg(&cpu, 1, 1);          // x1 = 1
    cpu_write_reg(&cpu, 2, 0xFFFFFFFF); // x2 = 0xFFFFFFFF
    mem_write32(&mem, 0, 0x0020E463);   // BLTU x1, x2, 8

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(8, cpu.pc);
    mem_free(&mem);
}

void test_bltu_not_taken(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_reg(&cpu, 1, 0xFFFFFFFF); // x1 = 0xFFFFFFFF
    cpu_write_reg(&cpu, 2, 1);          // x2 = 1
    mem_write32(&mem, 0, 0x0020E463);   // BLTU x1, x2, 8

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(4, cpu.pc);
    mem_free(&mem);
}

void test_bgeu_taken(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_reg(&cpu, 1, 0xFFFFFFFF); // x1 = 0xFFFFFFFF
    cpu_write_reg(&cpu, 2, 1);          // x2 = 1
    mem_write32(&mem, 0, 0x0020F463);   // BGEU x1, x2, 8

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(8, cpu.pc);
    mem_free(&mem);
}

void test_bgeu_not_taken(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_reg(&cpu, 1, 1);          // x1 = 1
    cpu_write_reg(&cpu, 2, 0xFFFFFFFF); // x2 = 0xFFFFFFFF
    mem_write32(&mem, 0, 0x0020F463);   // BGEU x1, x2, 8

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(4, cpu.pc);
    mem_free(&mem);
}

void test_jal(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 32);

    mem_write32(&mem, 0, 0x010000EF); // JAL x1, 16

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(4, cpu_read_reg(&cpu, 1));
    TEST_ASSERT_EQUAL_UINT32(16, cpu.pc);
    mem_free(&mem);
}

void test_jalr(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 32);

    cpu_write_reg(&cpu, 2, 0x11);     // x2 = 0x11 (odd, to check bit-0 clearing)
    mem_write32(&mem, 0, 0x004100E7); // JALR x1, 4(x2)

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(4, cpu_read_reg(&cpu, 1));
    TEST_ASSERT_EQUAL_UINT32(0x14, cpu.pc);
    mem_free(&mem);
}

void test_fence(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    mem_write32(&mem, 0, 0x0000000F); // FENCE

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(4, cpu.pc);
    mem_free(&mem);
}

void test_csrrw(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_csr(&cpu, CSR_MSCRATCH, 0x11111111); // old CSR value
    cpu_write_reg(&cpu, 1, 0x22222222);            // x1 = new value to write
    mem_write32(&mem, 0, 0x34009173);              // CSRRW x2, mscratch, x1

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(0x11111111, cpu_read_reg(&cpu, 2));
    TEST_ASSERT_EQUAL_UINT32(0x22222222, cpu_read_csr(&cpu, CSR_MSCRATCH));
    TEST_ASSERT_EQUAL_UINT32(4, cpu.pc);
    mem_free(&mem);
}

void test_csrrs(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_csr(&cpu, CSR_MSCRATCH, 0xF0F0F0F0); // old CSR value
    cpu_write_reg(&cpu, 1, 0x0F0F0F0F);            // x1 = set-mask
    mem_write32(&mem, 0, 0x3400A173);              // CSRRS x2, mscratch, x1

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(0xF0F0F0F0, cpu_read_reg(&cpu, 2));
    TEST_ASSERT_EQUAL_UINT32(0xFFFFFFFF, cpu_read_csr(&cpu, CSR_MSCRATCH));
    TEST_ASSERT_EQUAL_UINT32(4, cpu.pc);
    mem_free(&mem);
}

void test_csrrc(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_csr(&cpu, CSR_MSCRATCH, 0xFFFFFFFF); // old CSR value
    cpu_write_reg(&cpu, 1, 0x0F0F0F0F);            // x1 = clear-mask
    mem_write32(&mem, 0, 0x3400B173);              // CSRRC x2, mscratch, x1

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(0xFFFFFFFF, cpu_read_reg(&cpu, 2));
    TEST_ASSERT_EQUAL_UINT32(0xF0F0F0F0, cpu_read_csr(&cpu, CSR_MSCRATCH));
    TEST_ASSERT_EQUAL_UINT32(4, cpu.pc);
    mem_free(&mem);
}

void test_csrrwi(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_csr(&cpu, CSR_MSCRATCH, 0x11111111); // old CSR value
    mem_write32(&mem, 0, 0x340FD173);              // CSRRWI x2, mscratch, 31

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(0x11111111, cpu_read_reg(&cpu, 2));
    TEST_ASSERT_EQUAL_UINT32(0x1F, cpu_read_csr(&cpu, CSR_MSCRATCH));
    TEST_ASSERT_EQUAL_UINT32(4, cpu.pc);
    mem_free(&mem);
}

void test_csrrsi(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_csr(&cpu, CSR_MSCRATCH, 0xFFFFFFE0); // old CSR value (low 5 bits clear)
    mem_write32(&mem, 0, 0x340FE173);              // CSRRSI x2, mscratch, 31

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(0xFFFFFFE0, cpu_read_reg(&cpu, 2));
    TEST_ASSERT_EQUAL_UINT32(0xFFFFFFFF, cpu_read_csr(&cpu, CSR_MSCRATCH));
    TEST_ASSERT_EQUAL_UINT32(4, cpu.pc);
    mem_free(&mem);
}

void test_csrrci(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_csr(&cpu, CSR_MSCRATCH, 0xFFFFFFFF); // old CSR value
    mem_write32(&mem, 0, 0x340FF173);              // CSRRCI x2, mscratch, 31

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(0xFFFFFFFF, cpu_read_reg(&cpu, 2));
    TEST_ASSERT_EQUAL_UINT32(0xFFFFFFE0, cpu_read_csr(&cpu, CSR_MSCRATCH));
    TEST_ASSERT_EQUAL_UINT32(4, cpu.pc);
    mem_free(&mem);
}

void test_ecall_traps(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_csr(&cpu, CSR_MTVEC, 0x1000);
    mem_write32(&mem, 0, 0x00000073); // ECALL

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(0x1000, cpu.pc);
    TEST_ASSERT_EQUAL_UINT32(0, cpu_read_csr(&cpu, CSR_MEPC));
    TEST_ASSERT_EQUAL_UINT32(11, cpu_read_csr(&cpu, CSR_MCAUSE));
    mem_free(&mem);
}

void test_ebreak_traps(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_csr(&cpu, CSR_MTVEC, 0x1000);
    mem_write32(&mem, 0, 0x00100073); // EBREAK

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(0x1000, cpu.pc);
    TEST_ASSERT_EQUAL_UINT32(0, cpu_read_csr(&cpu, CSR_MEPC));
    TEST_ASSERT_EQUAL_UINT32(3, cpu_read_csr(&cpu, CSR_MCAUSE));
    mem_free(&mem);
}

void test_ecall_mstatus_update(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_csr(&cpu, CSR_MTVEC, 0x1000);
    cpu_write_csr(&cpu, CSR_MSTATUS, MSTATUS_MIE); // MIE set before the trap
    mem_write32(&mem, 0, 0x00000073); // ECALL

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(MSTATUS_MPIE | MSTATUS_MPP, cpu_read_csr(&cpu, CSR_MSTATUS));
    mem_free(&mem);
}

void test_illegal_instruction_unknown_opcode(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_csr(&cpu, CSR_MTVEC, 0x1000);
    mem_write32(&mem, 0, 0x0000002B); // unrecognized opcode 0x2B

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(0x1000, cpu.pc);
    TEST_ASSERT_EQUAL_UINT32(2, cpu_read_csr(&cpu, CSR_MCAUSE));
    TEST_ASSERT_EQUAL_UINT32(0x2B, cpu_read_csr(&cpu, CSR_MTVAL));
    TEST_ASSERT_EQUAL_UINT32(0, cpu_read_csr(&cpu, CSR_MEPC));
    mem_free(&mem);
}

void test_illegal_instruction_unknown_funct3(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_csr(&cpu, CSR_MTVEC, 0x1000);
    mem_write32(&mem, 0, 0x00004073); // opcode 0x73, reserved funct3=4

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(0x1000, cpu.pc);
    TEST_ASSERT_EQUAL_UINT32(2, cpu_read_csr(&cpu, CSR_MCAUSE));
    TEST_ASSERT_EQUAL_UINT32(0x4073, cpu_read_csr(&cpu, CSR_MTVAL));
    TEST_ASSERT_EQUAL_UINT32(0, cpu_read_csr(&cpu, CSR_MEPC));
    mem_free(&mem);
}

void test_load_access_fault(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_csr(&cpu, CSR_MTVEC, 0x1000);
    mem_write32(&mem, 0, 0x0140A103); // LW x2, 20(x1), x1 = 0 -> addr 20 (OOB)

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(0x1000, cpu.pc);
    TEST_ASSERT_EQUAL_UINT32(5, cpu_read_csr(&cpu, CSR_MCAUSE));
    TEST_ASSERT_EQUAL_UINT32(20, cpu_read_csr(&cpu, CSR_MTVAL));
    TEST_ASSERT_EQUAL_UINT32(0, cpu_read_csr(&cpu, CSR_MEPC));
    mem_free(&mem);
}

void test_store_access_fault(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_csr(&cpu, CSR_MTVEC, 0x1000);
    cpu_write_reg(&cpu, 2, 0xDEADBEEF);
    mem_write32(&mem, 0, 0x0020AA23); // SW x2, 20(x1), x1 = 0 -> addr 20 (OOB)

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(0x1000, cpu.pc);
    TEST_ASSERT_EQUAL_UINT32(7, cpu_read_csr(&cpu, CSR_MCAUSE));
    TEST_ASSERT_EQUAL_UINT32(20, cpu_read_csr(&cpu, CSR_MTVAL));
    TEST_ASSERT_EQUAL_UINT32(0, cpu_read_csr(&cpu, CSR_MEPC));
    mem_free(&mem);
}

/* x1 = 0, so a negative offset gives an address near the top of the 32-bit space.
 * The address computation wraps (correct), but the access must still fault. */
void test_load_access_fault_wrap(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_csr(&cpu, CSR_MTVEC, 0x1000);
    cpu_write_reg(&cpu, 2, 0xDEADBEEF);
    mem_write32(&mem, 0, 0xFFC02103); // LW x2, -4(x0) -> addr 0xFFFFFFFC

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(0x1000, cpu.pc);
    TEST_ASSERT_EQUAL_UINT32(5, cpu_read_csr(&cpu, CSR_MCAUSE));
    TEST_ASSERT_EQUAL_UINT32(0xFFFFFFFC, cpu_read_csr(&cpu, CSR_MTVAL));
    TEST_ASSERT_EQUAL_UINT32(0, cpu_read_csr(&cpu, CSR_MEPC));
    TEST_ASSERT_EQUAL_UINT32(0xDEADBEEF, cpu_read_reg(&cpu, 2)); // rd untouched
    mem_free(&mem);
}

void test_load_halfword_fault_wrap(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_csr(&cpu, CSR_MTVEC, 0x1000);
    cpu_write_reg(&cpu, 2, 0xDEADBEEF);
    mem_write32(&mem, 0, 0xFFE01103); // LH x2, -2(x0) -> addr 0xFFFFFFFE

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(0x1000, cpu.pc);
    TEST_ASSERT_EQUAL_UINT32(5, cpu_read_csr(&cpu, CSR_MCAUSE));
    TEST_ASSERT_EQUAL_UINT32(0xFFFFFFFE, cpu_read_csr(&cpu, CSR_MTVAL));
    TEST_ASSERT_EQUAL_UINT32(0, cpu_read_csr(&cpu, CSR_MEPC));
    TEST_ASSERT_EQUAL_UINT32(0xDEADBEEF, cpu_read_reg(&cpu, 2)); // rd untouched
    mem_free(&mem);
}

void test_store_access_fault_wrap(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_csr(&cpu, CSR_MTVEC, 0x1000);
    cpu_write_reg(&cpu, 2, 0xDEADBEEF);
    mem_write32(&mem, 0, 0xFE202E23); // SW x2, -4(x0) -> addr 0xFFFFFFFC

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(0x1000, cpu.pc);
    TEST_ASSERT_EQUAL_UINT32(7, cpu_read_csr(&cpu, CSR_MCAUSE));
    TEST_ASSERT_EQUAL_UINT32(0xFFFFFFFC, cpu_read_csr(&cpu, CSR_MTVAL));
    TEST_ASSERT_EQUAL_UINT32(0, cpu_read_csr(&cpu, CSR_MEPC));
    TEST_ASSERT_EQUAL_UINT32(0xFE202E23, mem_read32(&mem, 0)); // wrapped store didn't hit address 0
    mem_free(&mem);
}

void test_store_halfword_fault_wrap(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_csr(&cpu, CSR_MTVEC, 0x1000);
    cpu_write_reg(&cpu, 2, 0xDEADBEEF);
    mem_write32(&mem, 0, 0xFE201F23); // SH x2, -2(x0) -> addr 0xFFFFFFFE

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(0x1000, cpu.pc);
    TEST_ASSERT_EQUAL_UINT32(7, cpu_read_csr(&cpu, CSR_MCAUSE));
    TEST_ASSERT_EQUAL_UINT32(0xFFFFFFFE, cpu_read_csr(&cpu, CSR_MTVAL));
    TEST_ASSERT_EQUAL_UINT32(0, cpu_read_csr(&cpu, CSR_MEPC));
    TEST_ASSERT_EQUAL_UINT32(0xFE201F23, mem_read32(&mem, 0)); // wrapped store didn't hit address 0
    mem_free(&mem);
}

/* Reserved funct7 values on OP / OP-IMM shifts must trap as illegal instructions
 * instead of silently executing as a neighbouring instruction. */
static void assert_illegal_instruction(uint32_t encoding) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_csr(&cpu, CSR_MTVEC, 0x1000);
    cpu_write_reg(&cpu, 1, 7);
    cpu_write_reg(&cpu, 2, 3);
    cpu_write_reg(&cpu, 3, 0xDEADBEEF);
    mem_write32(&mem, 0, encoding);

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(0x1000, cpu.pc);
    TEST_ASSERT_EQUAL_UINT32(2, cpu_read_csr(&cpu, CSR_MCAUSE));
    TEST_ASSERT_EQUAL_UINT32(encoding, cpu_read_csr(&cpu, CSR_MTVAL));
    TEST_ASSERT_EQUAL_UINT32(0, cpu_read_csr(&cpu, CSR_MEPC));
    TEST_ASSERT_EQUAL_UINT32(0xDEADBEEF, cpu_read_reg(&cpu, 3)); // rd untouched
    mem_free(&mem);
}

void test_illegal_op_funct7_in_add_slot(void) {
    assert_illegal_instruction(0x022081B3); // MUL x3, x1, x2: funct7=0x01, funct3=0
}

void test_illegal_op_funct7_in_sll_slot(void) {
    assert_illegal_instruction(0x020091B3); // MULH x3, x1, x2: funct7=0x01, funct3=1
}

void test_illegal_op_alt_funct7_in_slt_slot(void) {
    assert_illegal_instruction(0x4020A1B3); // funct7=0x20, funct3=2: only SUB/SRA may use 0x20
}

void test_illegal_slli_nonzero_funct7(void) {
    assert_illegal_instruction(0x02209193); // SLLI x3, x1, 2 with funct7=0x01 (shamt[5]=1)
}

void test_illegal_srli_unknown_funct7(void) {
    assert_illegal_instruction(0x0220D193); // SRLI x3, x1, 2 with funct7=0x01
}

/* IALIGN=32: a taken jump/branch to a target that isn't 4-byte aligned traps with
 * cause 0, mtval = the target, mepc = the jump itself, and rd is not written. */
static void assert_misaligned_jump_trap(uint32_t encoding, uint32_t x1, uint32_t x2, uint32_t expected_target) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_csr(&cpu, CSR_MTVEC, 0x1000);
    cpu_write_csr(&cpu, CSR_MCAUSE, 0x77); // marker: cause 0 must be written by the trap
    cpu_write_reg(&cpu, 1, x1);
    cpu_write_reg(&cpu, 2, x2);
    mem_write32(&mem, 0, encoding);

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(0x1000, cpu.pc);
    TEST_ASSERT_EQUAL_UINT32(0, cpu_read_csr(&cpu, CSR_MCAUSE));
    TEST_ASSERT_EQUAL_UINT32(expected_target, cpu_read_csr(&cpu, CSR_MTVAL));
    TEST_ASSERT_EQUAL_UINT32(0, cpu_read_csr(&cpu, CSR_MEPC));
    TEST_ASSERT_EQUAL_UINT32(x1, cpu_read_reg(&cpu, 1)); // return address / x1 untouched
    mem_free(&mem);
}

void test_jal_misaligned_target(void) {
    assert_misaligned_jump_trap(0x002000EF, 0xDEADBEEF, 0, 2); // JAL x1, +2
}

void test_jalr_misaligned_target(void) {
    assert_misaligned_jump_trap(0x002100E7, 0xDEADBEEF, 0x100, 0x102); // JALR x1, 2(x2)
}

void test_jalr_misaligned_target_after_clearing_bit0(void) {
    // 0x100 + 3 = 0x103, bit 0 is cleared to 0x102, which is still misaligned
    assert_misaligned_jump_trap(0x003100E7, 0xDEADBEEF, 0x100, 0x102); // JALR x1, 3(x2)
}

void test_jalr_ignores_target_bit0(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_csr(&cpu, CSR_MCAUSE, 0x77); // marker: no trap must overwrite it
    cpu_write_reg(&cpu, 2, 0x100);
    mem_write32(&mem, 0, 0x001100E7); // JALR x1, 1(x2) -> 0x101 & ~1 = 0x100

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(0x100, cpu.pc);
    TEST_ASSERT_EQUAL_UINT32(4, cpu_read_reg(&cpu, 1));
    TEST_ASSERT_EQUAL_UINT32(0x77, cpu_read_csr(&cpu, CSR_MCAUSE));
    mem_free(&mem);
}

void test_beq_misaligned_target(void) {
    assert_misaligned_jump_trap(0x00208163, 5, 5, 2); // BEQ x1, x2, +2, taken
}

void test_bne_misaligned_target(void) {
    assert_misaligned_jump_trap(0x00209163, 5, 6, 2); // BNE x1, x2, +2, taken
}

void test_blt_misaligned_target(void) {
    assert_misaligned_jump_trap(0x0020C163, 1, 2, 2); // BLT x1, x2, +2, taken
}

void test_bge_misaligned_target(void) {
    assert_misaligned_jump_trap(0x0020D163, 2, 1, 2); // BGE x1, x2, +2, taken
}

void test_bltu_misaligned_target(void) {
    assert_misaligned_jump_trap(0x0020E163, 1, 2, 2); // BLTU x1, x2, +2, taken
}

void test_bgeu_misaligned_target(void) {
    assert_misaligned_jump_trap(0x0020F163, 2, 1, 2); // BGEU x1, x2, +2, taken
}

void test_branch_not_taken_misaligned_target_no_trap(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_csr(&cpu, CSR_MCAUSE, 0x77); // marker: no trap must overwrite it
    mem_write32(&mem, 0, 0x00001163); // BNE x0, x0, +2: never taken

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(4, cpu.pc);
    TEST_ASSERT_EQUAL_UINT32(0x77, cpu_read_csr(&cpu, CSR_MCAUSE));
    mem_free(&mem);
}

void test_csrrw_mtvec_clears_mode_bits(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_reg(&cpu, 1, 0x1001);
    mem_write32(&mem, 0, 0x30509073); // CSRRW x0, mtvec, x1

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(0x1000, cpu_read_csr(&cpu, CSR_MTVEC));
    TEST_ASSERT_EQUAL_UINT32(4, cpu.pc);
    mem_free(&mem);
}

void test_csrrwi_mtvec_clears_mode_bits(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_csr(&cpu, CSR_MTVEC, 0x2000);
    mem_write32(&mem, 0, 0x3051D073); // CSRRWI x0, mtvec, 3

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(0, cpu_read_csr(&cpu, CSR_MTVEC));
    mem_free(&mem);
}

void test_trap_jumps_to_mtvec_base(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_reg(&cpu, 1, 0x1001);
    mem_write32(&mem, 0, 0x30509073); // CSRRW x0, mtvec, x1
    mem_write32(&mem, 4, 0x00000073); // ECALL

    for (int i = 0; i < 2; i++) {
        uint32_t raw = mem_read32(&mem, cpu.pc);
        DecodedInstruction instr = decode_instruction(raw);
        execute(&cpu, &mem, &instr);
    }

    TEST_ASSERT_EQUAL_UINT32(0x1000, cpu.pc);
    TEST_ASSERT_EQUAL_UINT32(4, cpu_read_csr(&cpu, CSR_MEPC));
    TEST_ASSERT_EQUAL_UINT32(11, cpu_read_csr(&cpu, CSR_MCAUSE));
    mem_free(&mem);
}

/* Illegal CSR access: traps with cause 2, mtval = the raw instruction, and neither
 * rd nor the CSR is touched. The CSR is primed with 0x1234, x1 = x1_val, x2 is a sentinel. */
static void assert_csr_access_illegal(uint32_t encoding, uint16_t csr, uint32_t x1_val) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_csr(&cpu, CSR_MTVEC, 0x1000);
    cpu_write_csr(&cpu, csr, 0x1234);
    cpu_write_reg(&cpu, 1, x1_val);
    cpu_write_reg(&cpu, 2, 0xDEADBEEF);
    mem_write32(&mem, 0, encoding);

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(0x1000, cpu.pc);
    TEST_ASSERT_EQUAL_UINT32(2, cpu_read_csr(&cpu, CSR_MCAUSE));
    TEST_ASSERT_EQUAL_UINT32(encoding, cpu_read_csr(&cpu, CSR_MTVAL));
    TEST_ASSERT_EQUAL_UINT32(0, cpu_read_csr(&cpu, CSR_MEPC));
    TEST_ASSERT_EQUAL_UINT32(0xDEADBEEF, cpu_read_reg(&cpu, 2)); // rd untouched
    TEST_ASSERT_EQUAL_UINT32(0x1234, cpu_read_csr(&cpu, csr));   // CSR untouched
    mem_free(&mem);
}

/* Legal CSR read with no write: runs normally, rd gets the CSR value, CSR is unchanged. */
static void assert_csr_read_ok(uint32_t encoding, uint16_t csr) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_csr(&cpu, CSR_MCAUSE, 0x77); // marker: no trap must overwrite it
    cpu_write_csr(&cpu, csr, 0x1234);
    cpu_write_reg(&cpu, 2, 0xDEADBEEF);
    mem_write32(&mem, 0, encoding);

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(4, cpu.pc);
    TEST_ASSERT_EQUAL_UINT32(0x77, cpu_read_csr(&cpu, CSR_MCAUSE));
    TEST_ASSERT_EQUAL_UINT32(0x1234, cpu_read_reg(&cpu, 2));
    TEST_ASSERT_EQUAL_UINT32(0x1234, cpu_read_csr(&cpu, csr));
    mem_free(&mem);
}

/* Read-only CSR addresses (csr[11:10] == 0b11), here 0xC00 (cycle) */
void test_csrrw_read_only_csr_traps(void) {
    assert_csr_access_illegal(0xC0009073, 0xC00, 7); // CSRRW x0, cycle, x1
}

void test_csrrw_rs1_x0_read_only_csr_traps(void) {
    assert_csr_access_illegal(0xC0001173, 0xC00, 0); // CSRRW x2, cycle, x0: writes zero
}

void test_csrrs_read_only_csr_traps(void) {
    assert_csr_access_illegal(0xC000A173, 0xC00, 7); // CSRRS x2, cycle, x1
}

void test_csrrs_read_only_csr_traps_when_rs1_holds_zero(void) {
    assert_csr_access_illegal(0xC000A173, 0xC00, 0); // CSRRS x2, cycle, x1 with x1 = 0: rs1 != x0
}

void test_csrrc_read_only_csr_traps(void) {
    assert_csr_access_illegal(0xC000B173, 0xC00, 7); // CSRRC x2, cycle, x1
}

void test_csrrwi_read_only_csr_traps(void) {
    assert_csr_access_illegal(0xC0005173, 0xC00, 0); // CSRRWI x2, cycle, 0: always writes
}

void test_csrrsi_read_only_csr_traps(void) {
    assert_csr_access_illegal(0xC000E173, 0xC00, 0); // CSRRSI x2, cycle, 1
}

void test_csrrci_read_only_csr_traps(void) {
    assert_csr_access_illegal(0xC000F173, 0xC00, 0); // CSRRCI x2, cycle, 1
}

void test_csrrw_mhartid_traps(void) {
    assert_csr_access_illegal(0xF1409073, 0xF14, 7); // CSRRW x0, mhartid, x1
}

void test_csrrw_top_of_csr_space_traps(void) {
    assert_csr_access_illegal(0xFFF09073, 0xFFF, 7); // CSRRW x0, 0xFFF, x1
}

/* rs1 = x0 / uimm = 0 never writes, so reading a read-only CSR is legal */
void test_csrr_read_only_csr_does_not_trap(void) {
    assert_csr_read_ok(0xC0002173, 0xC00); // CSRRS x2, cycle, x0 (csrr)
}

void test_csrrsi_uimm0_read_only_csr_does_not_trap(void) {
    assert_csr_read_ok(0xC0006173, 0xC00); // CSRRSI x2, cycle, 0
}

void test_csrrci_uimm0_read_only_csr_does_not_trap(void) {
    assert_csr_read_ok(0xC0007173, 0xC00); // CSRRCI x2, cycle, 0
}

void test_csrr_mhartid_does_not_trap(void) {
    assert_csr_read_ok(0xF1402173, 0xF14); // CSRRS x2, mhartid, x0
}

/* 0xBFF has csr[11:10] == 0b10 (read/write), the last address before the read-only range */
void test_csrrw_last_writable_csr_below_read_only_range(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_csr(&cpu, CSR_MCAUSE, 0x77); // marker: no trap must overwrite it
    cpu_write_reg(&cpu, 1, 7);
    mem_write32(&mem, 0, 0xBFF09073); // CSRRW x0, 0xBFF, x1

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(4, cpu.pc);
    TEST_ASSERT_EQUAL_UINT32(7, cpu_read_csr(&cpu, 0xBFF));
    TEST_ASSERT_EQUAL_UINT32(0x77, cpu_read_csr(&cpu, CSR_MCAUSE));
    mem_free(&mem);
}

/* rs1 = x0 / uimm = 0 must not write the CSR back at all. mtvec's raw storage is set to
 * 0x1003 directly (bypassing the WARL accessor), so a write-back would show up as 0x1000. */
static void assert_csr_no_write_back(uint32_t encoding) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu.csr[CSR_MTVEC] = 0x1003;
    mem_write32(&mem, 0, encoding);

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(4, cpu.pc);
    TEST_ASSERT_EQUAL_UINT32(0x1003, cpu_read_reg(&cpu, 2));    // read the old value
    TEST_ASSERT_EQUAL_UINT32(0x1003, cpu.csr[CSR_MTVEC]);       // and did not write it back
    mem_free(&mem);
}

void test_csrrs_rs1_x0_does_not_write(void) {
    assert_csr_no_write_back(0x30502173); // CSRRS x2, mtvec, x0
}

void test_csrrc_rs1_x0_does_not_write(void) {
    assert_csr_no_write_back(0x30503173); // CSRRC x2, mtvec, x0
}

void test_csrrsi_uimm0_does_not_write(void) {
    assert_csr_no_write_back(0x30506173); // CSRRSI x2, mtvec, 0
}

void test_csrrci_uimm0_does_not_write(void) {
    assert_csr_no_write_back(0x30507173); // CSRRCI x2, mtvec, 0
}

/* 0x7B0-0x7BF are debug-mode-only: any M-mode access traps, reads included */
void test_debug_csr_read_traps(void) {
    assert_csr_access_illegal(0x7B002173, 0x7B0, 0); // CSRRS x2, 0x7B0, x0 (csrr): read only, still illegal
}

void test_debug_csr_write_traps(void) {
    assert_csr_access_illegal(0x7B009073, 0x7B0, 7); // CSRRW x0, 0x7B0, x1
}

void test_debug_csr_range_upper_bound_traps(void) {
    assert_csr_access_illegal(0x7BF09073, 0x7BF, 7); // CSRRW x0, 0x7BF, x1
}

void test_csr_just_below_debug_range_is_accessible(void) {
    assert_csr_read_ok(0x7AF02173, 0x7AF); // CSRRS x2, 0x7AF, x0
}

void test_csr_just_above_debug_range_is_accessible(void) {
    assert_csr_read_ok(0x7C002173, 0x7C0); // CSRRS x2, 0x7C0, x0
}

void test_mret_returns_to_mepc(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_csr(&cpu, CSR_MEPC, 0x2000);
    mem_write32(&mem, 0, 0x30200073); // MRET

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(0x2000, cpu.pc);
    mem_free(&mem);
}

void test_mret_restores_mie_from_mpie(void) {
    CPU cpu = {0};
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_csr(&cpu, CSR_MSTATUS, MSTATUS_MPIE); // MPIE set, MIE clear
    mem_write32(&mem, 0, 0x30200073); // MRET

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(MSTATUS_MIE | MSTATUS_MPIE, cpu_read_csr(&cpu, CSR_MSTATUS));
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
    RUN_TEST(test_bne_taken);
    RUN_TEST(test_bne_not_taken);
    RUN_TEST(test_blt_taken);
    RUN_TEST(test_blt_not_taken);
    RUN_TEST(test_bge_taken);
    RUN_TEST(test_bge_not_taken);
    RUN_TEST(test_bltu_taken);
    RUN_TEST(test_bltu_not_taken);
    RUN_TEST(test_bgeu_taken);
    RUN_TEST(test_bgeu_not_taken);
    RUN_TEST(test_jal);
    RUN_TEST(test_jalr);
    RUN_TEST(test_fence);
    RUN_TEST(test_csrrw);
    RUN_TEST(test_csrrs);
    RUN_TEST(test_csrrc);
    RUN_TEST(test_csrrwi);
    RUN_TEST(test_csrrsi);
    RUN_TEST(test_csrrci);
    RUN_TEST(test_ecall_traps);
    RUN_TEST(test_ebreak_traps);
    RUN_TEST(test_ecall_mstatus_update);
    RUN_TEST(test_illegal_instruction_unknown_opcode);
    RUN_TEST(test_illegal_instruction_unknown_funct3);
    RUN_TEST(test_load_access_fault);
    RUN_TEST(test_store_access_fault);
    RUN_TEST(test_load_access_fault_wrap);
    RUN_TEST(test_load_halfword_fault_wrap);
    RUN_TEST(test_store_access_fault_wrap);
    RUN_TEST(test_store_halfword_fault_wrap);
    RUN_TEST(test_illegal_op_funct7_in_add_slot);
    RUN_TEST(test_illegal_op_funct7_in_sll_slot);
    RUN_TEST(test_illegal_op_alt_funct7_in_slt_slot);
    RUN_TEST(test_illegal_slli_nonzero_funct7);
    RUN_TEST(test_illegal_srli_unknown_funct7);
    RUN_TEST(test_jal_misaligned_target);
    RUN_TEST(test_jalr_misaligned_target);
    RUN_TEST(test_jalr_misaligned_target_after_clearing_bit0);
    RUN_TEST(test_jalr_ignores_target_bit0);
    RUN_TEST(test_beq_misaligned_target);
    RUN_TEST(test_bne_misaligned_target);
    RUN_TEST(test_blt_misaligned_target);
    RUN_TEST(test_bge_misaligned_target);
    RUN_TEST(test_bltu_misaligned_target);
    RUN_TEST(test_bgeu_misaligned_target);
    RUN_TEST(test_branch_not_taken_misaligned_target_no_trap);
    RUN_TEST(test_csrrw_mtvec_clears_mode_bits);
    RUN_TEST(test_csrrwi_mtvec_clears_mode_bits);
    RUN_TEST(test_trap_jumps_to_mtvec_base);
    RUN_TEST(test_csrrw_read_only_csr_traps);
    RUN_TEST(test_csrrw_rs1_x0_read_only_csr_traps);
    RUN_TEST(test_csrrs_read_only_csr_traps);
    RUN_TEST(test_csrrs_read_only_csr_traps_when_rs1_holds_zero);
    RUN_TEST(test_csrrc_read_only_csr_traps);
    RUN_TEST(test_csrrwi_read_only_csr_traps);
    RUN_TEST(test_csrrsi_read_only_csr_traps);
    RUN_TEST(test_csrrci_read_only_csr_traps);
    RUN_TEST(test_csrrw_mhartid_traps);
    RUN_TEST(test_csrrw_top_of_csr_space_traps);
    RUN_TEST(test_csrr_read_only_csr_does_not_trap);
    RUN_TEST(test_csrrsi_uimm0_read_only_csr_does_not_trap);
    RUN_TEST(test_csrrci_uimm0_read_only_csr_does_not_trap);
    RUN_TEST(test_csrr_mhartid_does_not_trap);
    RUN_TEST(test_csrrw_last_writable_csr_below_read_only_range);
    RUN_TEST(test_csrrs_rs1_x0_does_not_write);
    RUN_TEST(test_csrrc_rs1_x0_does_not_write);
    RUN_TEST(test_csrrsi_uimm0_does_not_write);
    RUN_TEST(test_csrrci_uimm0_does_not_write);
    RUN_TEST(test_debug_csr_read_traps);
    RUN_TEST(test_debug_csr_write_traps);
    RUN_TEST(test_debug_csr_range_upper_bound_traps);
    RUN_TEST(test_csr_just_below_debug_range_is_accessible);
    RUN_TEST(test_csr_just_above_debug_range_is_accessible);
    RUN_TEST(test_mret_returns_to_mepc);
    RUN_TEST(test_mret_restores_mie_from_mpie);
	return UNITY_END();
}
