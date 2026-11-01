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
	return UNITY_END();
}
