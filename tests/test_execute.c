#include "unity.h"
#include "../src/execute.h"
#include "cpu_test_util.h"

void setUp(void) {} // Unity calles this before each test function
void tearDown(void) {} // Unity calls this after eacht test function


void test_addi(void) {
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_csr(&cpu, CSR_MTVEC, 0x1000);
    cpu.csr[csr] = 0x1234; // raw storage: some CSRs are hardwired through the accessors
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
    TEST_ASSERT_EQUAL_UINT32(0x1234, cpu.csr[csr]);              // CSR untouched
    mem_free(&mem);
}

/* Legal CSR read with no write: runs normally, rd gets the CSR value (expected_rd), CSR is
 * unchanged. The raw storage is primed with 0x1234, which hardwired CSRs ignore. */
static void assert_csr_read_ok(uint32_t encoding, uint16_t csr, uint32_t expected_rd) {
    CPU cpu; cpu_reset(&cpu);
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_csr(&cpu, CSR_MCAUSE, 0x77); // marker: no trap must overwrite it
    cpu.csr[csr] = 0x1234;
    cpu_write_reg(&cpu, 2, 0xDEADBEEF);
    mem_write32(&mem, 0, encoding);

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(4, cpu.pc);
    TEST_ASSERT_EQUAL_UINT32(0x77, cpu_read_csr(&cpu, CSR_MCAUSE));
    TEST_ASSERT_EQUAL_UINT32(expected_rd, cpu_read_reg(&cpu, 2));
    TEST_ASSERT_EQUAL_UINT32(0x1234, cpu.csr[csr]);
    mem_free(&mem);
}

/* ---- CSR privilege and counter gating ---------------------------------------------------
 * csr[9:8] is the lowest mode that may access a CSR; cycle/instret (and their h halves) are
 * further gated by mcounteren (S-mode) and mcounteren & scounteren (U-mode). */

/* Run the single CSR instruction `encoding` at address 0 on a fresh CPU in `mode`. Returns true if
 * it trapped as an illegal instruction, and checks the trap or no-trap state either way. */
static bool csr_traps(PrivMode mode, uint32_t mcounteren, uint32_t scounteren, uint32_t encoding) {
    CPU cpu; test_cpu_reset_in_mode(&cpu, mode);
    Memory mem;
    mem_init(&mem, 16);
    cpu_write_csr(&cpu, CSR_MCOUNTEREN, mcounteren);
    cpu_write_csr(&cpu, CSR_SCOUNTEREN, scounteren);
    cpu_write_csr(&cpu, CSR_MTVEC, 0x1000);
    cpu_write_csr(&cpu, CSR_MCAUSE, 0x77);     // marker: only a trap overwrites it
    cpu_write_reg(&cpu, 2, 0xDEADBEEF);
    mem_write32(&mem, 0, encoding);

    DecodedInstruction instr = decode_instruction(encoding);
    execute(&cpu, &mem, &instr);

    bool trapped = cpu_read_csr(&cpu, CSR_MCAUSE) == 2;
    if (trapped) {
        TEST_ASSERT_EQUAL_INT(PRV_M, cpu.privilege_mode);
        TEST_ASSERT_EQUAL_UINT32(0x1000, cpu.pc);
        TEST_ASSERT_EQUAL_UINT32(encoding, cpu_read_csr(&cpu, CSR_MTVAL));
        TEST_ASSERT_EQUAL_UINT32(0, cpu_read_csr(&cpu, CSR_MEPC));
        TEST_ASSERT_EQUAL_UINT32(0xDEADBEEF, cpu_read_reg(&cpu, 2)); // rd untouched
        TEST_ASSERT_EQUAL_UINT32((uint32_t)mode << 11, cpu_read_csr(&cpu, CSR_MSTATUS) & MSTATUS_MPP);
    } else {
        TEST_ASSERT_EQUAL_UINT32(0x77, cpu_read_csr(&cpu, CSR_MCAUSE));
        TEST_ASSERT_EQUAL_UINT32(4, cpu.pc);
        TEST_ASSERT_EQUAL_INT(mode, cpu.privilege_mode);
    }
    mem_free(&mem);
    return trapped;
}

static void assert_csr_traps(PrivMode mode, uint32_t m, uint32_t s, uint32_t encoding, bool expected, const char *what) {
    char msg[96];
    snprintf(msg, sizeof msg, "%s, mode %d, mcounteren 0x%x, scounteren 0x%x", what, (int)mode, m, s);
    TEST_ASSERT_EQUAL_INT_MESSAGE(expected, csr_traps(mode, m, s, encoding), msg);
}

/* CSRRS x2, csr, x0 reads the CSR without writing it. Counter-enable bits are all set, so only
 * the privilege level decides. */
void test_csr_read_privilege_by_mode(void) {
    const struct { const char *name; uint32_t encoding; bool u, s, m; } v[] = {
        {"mstatus",    0x30002173, true,  true,  false},
        {"mscratch",   0x34002173, true,  true,  false},
        {"mcycle",     0xB0002173, true,  true,  false},
        {"mvendorid",  0xF1102173, true,  true,  false},
        {"tselect",    0x7A002173, true,  true,  false},
        {"mcounteren", 0x30602173, true,  true,  false},
        {"scounteren", 0x10602173, true,  false, false},
        {"sstatus",    0x10002173, true,  false, false},
        {"sie",        0x10402173, true,  false, false},
        {"stvec",      0x10502173, true,  false, false},
        {"sscratch",   0x14002173, true,  false, false},
        {"sepc",       0x14102173, true,  false, false},
        {"scause",     0x14202173, true,  false, false},
        {"stval",      0x14302173, true,  false, false},
        {"sip",        0x14402173, true,  false, false},
        {"satp",       0x18002173, true,  false, false},
        {"medeleg",    0x30202173, true,  true,  false},
        {"mideleg",    0x30302173, true,  true,  false},
        {"mstatush",   0x31002173, true,  true,  false},
        {"medelegh",   0x31202173, true,  true,  false},
        {"cycle",      0xC0002173, false, false, false},
        {"instret",    0xC0202173, false, false, false},
    };
    for (unsigned i = 0; i < sizeof v / sizeof v[0]; i++) {
        assert_csr_traps(PRV_U, 0x5, 0x5, v[i].encoding, v[i].u, v[i].name);
        assert_csr_traps(PRV_S, 0x5, 0x5, v[i].encoding, v[i].s, v[i].name);
        assert_csr_traps(PRV_M, 0x5, 0x5, v[i].encoding, v[i].m, v[i].name);
    }
}

/* The write forms follow the same rule: mscratch (0x340) needs M-mode. */
void test_csr_write_forms_follow_privilege(void) {
    const struct { const char *name; uint32_t encoding; } v[] = {
        {"CSRRW x0, mscratch, x1", 0x34009073},
        {"CSRRWI x0, mscratch, 1", 0x3400D073},
        {"CSRRSI x0, mscratch, 1", 0x3400E073},
    };
    for (unsigned i = 0; i < sizeof v / sizeof v[0]; i++) {
        assert_csr_traps(PRV_U, 0, 0, v[i].encoding, true,  v[i].name);
        assert_csr_traps(PRV_S, 0, 0, v[i].encoding, true,  v[i].name);
        assert_csr_traps(PRV_M, 0, 0, v[i].encoding, false, v[i].name);
    }
}

/* (mcounteren, scounteren) -> does the read trap, from S then from U. M-mode never traps. */
void test_counter_gating(void) {
    const struct {
        uint32_t m, s;
        bool cycle_s, cycle_u, instret_s, instret_u;
    } v[] = {
        {0x5, 0x5, false, false, false, false},
        {0x0, 0x5, true,  true,  true,  true },
        {0x5, 0x0, false, true,  false, true },
        {0x1, 0x5, false, false, true,  true },
        {0x4, 0x5, true,  true,  false, false},
        {0x5, 0x4, false, true,  false, false},
    };
    for (unsigned i = 0; i < sizeof v / sizeof v[0]; i++) {
        /* cycle and cycleh share the CY bit; instret and instreth share the IR bit */
        const uint32_t cycle[]   = {0xC0002173, 0xC8002173};
        const uint32_t instret[] = {0xC0202173, 0xC8202173};
        for (unsigned h = 0; h < 2; h++) {
            assert_csr_traps(PRV_S, v[i].m, v[i].s, cycle[h],   v[i].cycle_s,   "cycle");
            assert_csr_traps(PRV_U, v[i].m, v[i].s, cycle[h],   v[i].cycle_u,   "cycle");
            assert_csr_traps(PRV_S, v[i].m, v[i].s, instret[h], v[i].instret_s, "instret");
            assert_csr_traps(PRV_U, v[i].m, v[i].s, instret[h], v[i].instret_u, "instret");
            assert_csr_traps(PRV_M, v[i].m, v[i].s, cycle[h],   false, "cycle");
            assert_csr_traps(PRV_M, v[i].m, v[i].s, instret[h], false, "instret");
        }
    }
}

/* Gating applies to reads only through the counter CSRs: the machine counters are not gated
 * by mcounteren, and scounteren is reachable from S-mode. */
void test_counter_enable_does_not_gate_machine_counters(void) {
    assert_csr_traps(PRV_M, 0, 0, 0xB0002173, false, "mcycle");  // CSRRS x2, mcycle, x0
    assert_csr_traps(PRV_S, 0, 0, 0xB0002173, true,  "mcycle");  // still M-only
    assert_csr_traps(PRV_S, 0, 0, 0x10602173, false, "scounteren"); // S-level CSR
}

/* ---- S-mode CSRs through real instructions ---------------------------------------------- */

/* Run the single instruction `encoding` at address 0 on `cpu`, which the caller has set up. */
static void run_on(CPU *cpu, uint32_t encoding) {
    Memory mem;
    mem_init(&mem, 16);
    mem_write32(&mem, 0, encoding);
    cpu->pc = 0;
    DecodedInstruction instr = decode_instruction(encoding);
    execute(cpu, &mem, &instr);
    mem_free(&mem);
}

/* satp is read-only 0 until Milestone 5: it never traps in S or M, and a write has no effect. */
void test_satp_access_does_not_trap_and_stays_zero(void) {
    const PrivMode modes[] = {PRV_S, PRV_M};
    for (unsigned i = 0; i < 2; i++) {
        CPU cpu; test_cpu_reset_in_mode(&cpu, modes[i]);
        cpu_write_reg(&cpu, 1, 0xFFFFFFFF);
        run_on(&cpu, 0x18009073);                       // CSRRW x0, satp, x1
        TEST_ASSERT_EQUAL_UINT32(4, cpu.pc);            // no trap
        TEST_ASSERT_EQUAL_INT(modes[i], cpu.privilege_mode);
        run_on(&cpu, 0x18002173);                       // CSRRS x2, satp, x0
        TEST_ASSERT_EQUAL_UINT32(0, cpu_read_reg(&cpu, 2));
    }
}

void test_csrrs_sstatus_sets_sie_in_mstatus(void) {
    CPU cpu; test_cpu_reset_in_mode(&cpu, PRV_S);
    cpu_write_reg(&cpu, 1, 0x2);
    run_on(&cpu, 0x1000A073);                           // CSRRS x0, sstatus, x1
    TEST_ASSERT_EQUAL_UINT32(4, cpu.pc);
    TEST_ASSERT_EQUAL_UINT32(0x2, cpu_read_csr(&cpu, CSR_MSTATUS));
}

void test_csrrc_sstatus_clears_only_s_bits(void) {
    CPU cpu; test_cpu_reset_in_mode(&cpu, PRV_S);
    cpu_write_csr(&cpu, CSR_MSTATUS, 0x19AA);           // S bits and M bits set, MPP = M
    cpu_write_reg(&cpu, 1, 0xFFFFFFFF);
    run_on(&cpu, 0x1000B073);                           // CSRRC x0, sstatus, x1
    TEST_ASSERT_EQUAL_UINT32(0x1888, cpu_read_csr(&cpu, CSR_MSTATUS)); // MIE, MPIE, MPP kept
}

void test_csrrw_sepc_drops_low_bits(void) {
    CPU cpu; test_cpu_reset_in_mode(&cpu, PRV_S);
    cpu_write_reg(&cpu, 1, 0x80000003);
    run_on(&cpu, 0x14109073);                           // CSRRW x0, sepc, x1
    run_on(&cpu, 0x14102173);                           // CSRRS x2, sepc, x0
    TEST_ASSERT_EQUAL_UINT32(0x80000000, cpu_read_reg(&cpu, 2));
}

/* ---- Trap delegation --------------------------------------------------------------------- */

#define ECALL_WORD   0x00000073u
#define ILLEGAL_WORD 0x0000007Fu   /* unknown opcode: illegal instruction, mtval/stval = the word */
#define LW_X1_0_X2   0x00012083u   /* LW x1, 0(x2) */

/* Run one instruction at pc 0x40 on a CPU the caller set up (mode, medeleg, ...), with
 * stvec = 0x2000 and mtvec = 0x1000. */
static void run_trap_case(CPU *cpu, uint32_t raw) {
    Memory mem;
    mem_init(&mem, 0x100);
    mem_write32(&mem, 0x40, raw);
    cpu_write_csr(cpu, CSR_STVEC, 0x2000);
    cpu_write_csr(cpu, CSR_MTVEC, 0x1000);
    cpu->pc = 0x40;
    DecodedInstruction instr = decode_instruction(raw);
    execute(cpu, &mem, &instr);
    mem_free(&mem);
}

/* (mode, medeleg, instruction, x2) -> which mode takes the trap, and the cause */
void test_trap_routing(void) {
    const struct {
        const char *name;
        PrivMode mode; uint32_t medeleg; uint32_t raw; uint32_t x2;
        PrivMode target; uint32_t cause;
    } v[] = {
        {"U ecall, nothing delegated",   PRV_U, 0x000, ECALL_WORD,   0,      PRV_M, 8},
        {"U ecall, bit 8 delegated",     PRV_U, 0x100, ECALL_WORD,   0,      PRV_S, 8},
        {"S ecall, bit 9 delegated",     PRV_S, 0x200, ECALL_WORD,   0,      PRV_S, 9},
        {"S ecall, nothing delegated",   PRV_S, 0x000, ECALL_WORD,   0,      PRV_M, 9},
        {"M ecall, everything set",      PRV_M, 0xB3FF, ECALL_WORD,  0,      PRV_M, 11},
        {"M illegal, everything set",    PRV_M, 0xB3FF, ILLEGAL_WORD, 0,     PRV_M, 2},
        {"U illegal, bit 2 delegated",   PRV_U, 0x004, ILLEGAL_WORD, 0,      PRV_S, 2},
        {"U illegal, only bit 8 set",    PRV_U, 0x100, ILLEGAL_WORD, 0,      PRV_M, 2},
        {"U load fault, bit 5 delegated", PRV_U, 0x020, LW_X1_0_X2,  0x1000, PRV_S, 5},
    };
    for (unsigned i = 0; i < sizeof v / sizeof v[0]; i++) {
        CPU cpu; test_cpu_reset_in_mode(&cpu, v[i].mode);
        cpu_write_csr(&cpu, CSR_MEDELEG, v[i].medeleg);
        cpu_write_reg(&cpu, 2, v[i].x2);
        run_trap_case(&cpu, v[i].raw);

        TEST_ASSERT_EQUAL_INT_MESSAGE(v[i].target, cpu.privilege_mode, v[i].name);
        TEST_ASSERT_EQUAL_UINT32_MESSAGE(v[i].target == PRV_S ? 0x2000 : 0x1000, cpu.pc, v[i].name);
        uint32_t cause_csr = v[i].target == PRV_S ? CSR_SCAUSE : CSR_MCAUSE;
        TEST_ASSERT_EQUAL_UINT32_MESSAGE(v[i].cause, cpu_read_csr(&cpu, cause_csr), v[i].name);
    }
}

/* A trap from U delegated to S: S registers are written, M registers and MPP/MPIE are not. */
void test_delegated_trap_writes_s_state_and_leaves_m_state(void) {
    CPU cpu; test_cpu_reset_in_mode(&cpu, PRV_U);
    cpu_write_csr(&cpu, CSR_MEDELEG, 0x100);
    cpu_write_csr(&cpu, CSR_MSTATUS, 0x188A);   // MIE, MPIE, MPP = M, SIE
    cpu_write_csr(&cpu, CSR_MEPC, 0x1110);      // markers: a trap into S must not touch these
    cpu_write_csr(&cpu, CSR_MCAUSE, 0x77);
    cpu_write_csr(&cpu, CSR_MTVAL, 0x66);
    cpu_write_csr(&cpu, CSR_STVEC, 0x2003);     // low bits are masked

    Memory mem;
    mem_init(&mem, 0x100);
    mem_write32(&mem, 0x40, ECALL_WORD);
    cpu.pc = 0x40;
    DecodedInstruction instr = decode_instruction(ECALL_WORD);
    execute(&cpu, &mem, &instr);
    mem_free(&mem);

    TEST_ASSERT_EQUAL_INT(PRV_S, cpu.privilege_mode);
    TEST_ASSERT_EQUAL_UINT32(0x2000, cpu.pc);
    TEST_ASSERT_EQUAL_UINT32(0x40, cpu_read_csr(&cpu, CSR_SEPC));
    TEST_ASSERT_EQUAL_UINT32(8, cpu_read_csr(&cpu, CSR_SCAUSE));
    TEST_ASSERT_EQUAL_UINT32(0, cpu_read_csr(&cpu, CSR_STVAL));
    TEST_ASSERT_EQUAL_UINT32(0x18A8, cpu_read_csr(&cpu, CSR_MSTATUS)); // SIE -> SPIE, SPP = 0, M bits kept
    TEST_ASSERT_EQUAL_UINT32(0x1110, cpu_read_csr(&cpu, CSR_MEPC));
    TEST_ASSERT_EQUAL_UINT32(0x77, cpu_read_csr(&cpu, CSR_MCAUSE));
    TEST_ASSERT_EQUAL_UINT32(0x66, cpu_read_csr(&cpu, CSR_MTVAL));
}

/* (mode, SIE before) -> sstatus after the delegated trap */
void test_delegated_trap_spp_and_interrupt_enable_stack(void) {
    const struct { PrivMode mode; uint32_t mstatus; uint32_t sstatus; } v[] = {
        {PRV_S, 0x0, 0x100},   // SPP = 1, SPIE = 0
        {PRV_S, 0x2, 0x120},   // SPP = 1, SPIE = 1, SIE cleared
        {PRV_U, 0x2, 0x020},   // SPP = 0, SPIE = 1
        {PRV_U, 0x0, 0x000},
    };
    for (unsigned i = 0; i < sizeof v / sizeof v[0]; i++) {
        CPU cpu; test_cpu_reset_in_mode(&cpu, v[i].mode);
        cpu_write_csr(&cpu, CSR_MEDELEG, 0x300);          // ECALL from U and from S
        cpu_write_csr(&cpu, CSR_MSTATUS, v[i].mstatus);
        run_trap_case(&cpu, ECALL_WORD);
        TEST_ASSERT_EQUAL_UINT32(v[i].sstatus, cpu_read_csr(&cpu, CSR_SSTATUS));
    }
}

void test_delegated_trap_stval_holds_the_instruction_or_address(void) {
    CPU cpu; test_cpu_reset_in_mode(&cpu, PRV_S);
    cpu_write_csr(&cpu, CSR_MEDELEG, 0x24);              // illegal (2) and load fault (5)
    run_trap_case(&cpu, ILLEGAL_WORD);
    TEST_ASSERT_EQUAL_UINT32(ILLEGAL_WORD, cpu_read_csr(&cpu, CSR_STVAL));

    test_cpu_reset_in_mode(&cpu, PRV_S);
    cpu_write_csr(&cpu, CSR_MEDELEG, 0x24);
    cpu_write_reg(&cpu, 2, 0x1000);
    run_trap_case(&cpu, LW_X1_0_X2);
    TEST_ASSERT_EQUAL_UINT32(5, cpu_read_csr(&cpu, CSR_SCAUSE));
    TEST_ASSERT_EQUAL_UINT32(0x1000, cpu_read_csr(&cpu, CSR_STVAL));
}

void test_delegated_trap_does_not_retire(void) {
    CPU cpu; test_cpu_reset_in_mode(&cpu, PRV_U);
    cpu_write_csr(&cpu, CSR_MEDELEG, 0x100);
    Memory mem;
    mem_init(&mem, 64);
    mem_write32(&mem, 0, ECALL_WORD);
    cpu_step(&cpu, &mem);
    TEST_ASSERT_EQUAL_UINT64(1, cpu.mcycle);
    TEST_ASSERT_EQUAL_UINT64(0, cpu.minstret);
    mem_free(&mem);
}

void test_delegation_csrs_through_instructions(void) {
    CPU cpu; cpu_reset(&cpu);
    cpu_write_reg(&cpu, 1, 0xFFFFFFFF);
    run_on(&cpu, 0x30209073);                            // CSRRW x0, medeleg, x1
    TEST_ASSERT_EQUAL_UINT32(4, cpu.pc);                 // no trap
    TEST_ASSERT_EQUAL_UINT32(0xB3FF, cpu_read_csr(&cpu, CSR_MEDELEG));
    run_on(&cpu, 0x30309073);                            // CSRRW x0, mideleg, x1
    TEST_ASSERT_EQUAL_UINT32(0x222, cpu_read_csr(&cpu, CSR_MIDELEG));
}

/* ---- SRET, WFI, SFENCE.VMA and reserved encodings --------------------------------------- */

#define SRET_WORD       0x10200073u
#define WFI_WORD        0x10500073u
#define SFENCE_X0_X0    0x12000073u   /* SFENCE.VMA x0, x0 */
#define SFENCE_X1_X2    0x12208073u   /* SFENCE.VMA x1, x2 */
#define MRET_WORD       0x30200073u
#define EBREAK_WORD     0x00100073u
#define DRET_WORD       0x7B200073u

/* The word at pc 0x40 traps as an illegal instruction in `mode`: mcause 2, mtval = the word,
 * taken in M with MPP = the mode it ran in, and nothing else changed. */
static void assert_illegal_in_mode(PrivMode mode, uint32_t mstatus, uint32_t raw, const char *what) {
    char msg[96];
    snprintf(msg, sizeof msg, "%s (mode %d, word 0x%08x)", what, (int)mode, raw);
    CPU cpu; test_cpu_reset_in_mode(&cpu, mode);
    cpu_write_csr(&cpu, CSR_MSTATUS, mstatus);
    cpu_write_reg(&cpu, 1, 0x1234);
    run_trap_case(&cpu, raw);
    TEST_ASSERT_EQUAL_INT_MESSAGE(PRV_M, cpu.privilege_mode, msg);
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(0x1000, cpu.pc, msg);
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(2, cpu_read_csr(&cpu, CSR_MCAUSE), msg);
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(raw, cpu_read_csr(&cpu, CSR_MTVAL), msg);
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(0x40, cpu_read_csr(&cpu, CSR_MEPC), msg);
    TEST_ASSERT_EQUAL_UINT32_MESSAGE((uint32_t)mode << 11, cpu_read_csr(&cpu, CSR_MSTATUS) & MSTATUS_MPP, msg);
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(0x1234, cpu_read_reg(&cpu, 1), msg);   // no register changed
}

/* (mode, mstatus before) -> (mode after, mstatus after); sepc = 0x40. A trap row has no result. */
void test_sret_table(void) {
    const struct {
        const char *name;
        PrivMode mode; uint32_t mstatus;
        bool traps; PrivMode after; uint32_t mstatus_after;
    } v[] = {
        {"S, SPP=1 SPIE=1",          PRV_S, 0x000120, false, PRV_S, 0x000022},
        {"S, SPP=0",                 PRV_S, 0x000000, false, PRV_U, 0x000020},
        {"M, SPP=0 SPIE=1 MPRV",     PRV_M, 0x020020, false, PRV_U, 0x000022},
        {"M, SPP=1 MPRV",            PRV_M, 0x020100, false, PRV_S, 0x000020},
        {"M, TSR set still allowed", PRV_M, 0x400120, false, PRV_S, 0x400022},
        {"S, TSR set",               PRV_S, 0x400120, true,  0,     0},
        {"U",                        PRV_U, 0x000120, true,  0,     0},
    };
    for (unsigned i = 0; i < sizeof v / sizeof v[0]; i++) {
        if (v[i].traps) {
            assert_illegal_in_mode(v[i].mode, v[i].mstatus, SRET_WORD, v[i].name);
            continue;
        }
        CPU cpu; test_cpu_reset_in_mode(&cpu, v[i].mode);
        cpu_write_csr(&cpu, CSR_MSTATUS, v[i].mstatus);
        cpu_write_csr(&cpu, CSR_SEPC, 0x40);
        run_trap_case(&cpu, SRET_WORD);
        TEST_ASSERT_EQUAL_INT_MESSAGE(v[i].after, cpu.privilege_mode, v[i].name);
        TEST_ASSERT_EQUAL_UINT32_MESSAGE(0x40, cpu.pc, v[i].name);
        TEST_ASSERT_EQUAL_UINT32_MESSAGE(v[i].mstatus_after, cpu_read_csr(&cpu, CSR_MSTATUS), v[i].name);
    }
}

void test_sret_returns_to_sepc_without_its_low_bits(void) {
    CPU cpu; test_cpu_reset_in_mode(&cpu, PRV_S);
    cpu_write_csr(&cpu, CSR_MSTATUS, 0x120);
    cpu_write_csr(&cpu, CSR_SEPC, 0x43);
    run_trap_case(&cpu, SRET_WORD);
    TEST_ASSERT_EQUAL_UINT32(0x40, cpu.pc);
}

void test_sret_leaves_the_m_registers_alone(void) {
    CPU cpu; test_cpu_reset_in_mode(&cpu, PRV_S);
    cpu_write_csr(&cpu, CSR_MSTATUS, 0x1920);       // SPP, SPIE, MPP = M, MPIE... only S bits may change
    cpu_write_csr(&cpu, CSR_MEPC, 0x1110);
    cpu_write_csr(&cpu, CSR_MCAUSE, 0x77);
    cpu_write_csr(&cpu, CSR_MTVAL, 0x66);
    cpu_write_csr(&cpu, CSR_SEPC, 0x40);
    run_trap_case(&cpu, SRET_WORD);
    TEST_ASSERT_EQUAL_UINT32(0x1110, cpu_read_csr(&cpu, CSR_MEPC));
    TEST_ASSERT_EQUAL_UINT32(0x77, cpu_read_csr(&cpu, CSR_MCAUSE));
    TEST_ASSERT_EQUAL_UINT32(0x66, cpu_read_csr(&cpu, CSR_MTVAL));
    TEST_ASSERT_EQUAL_UINT32(0x1800, cpu_read_csr(&cpu, CSR_MSTATUS) & MSTATUS_MPP);   // MPP untouched
}

void test_sret_retires(void) {
    CPU cpu; test_cpu_reset_in_mode(&cpu, PRV_S);
    cpu_write_csr(&cpu, CSR_MSTATUS, 0x120);
    cpu_write_csr(&cpu, CSR_SEPC, 0x20);
    Memory mem;
    mem_init(&mem, 64);
    mem_write32(&mem, 0, SRET_WORD);
    cpu_step(&cpu, &mem);
    TEST_ASSERT_EQUAL_UINT64(1, cpu.minstret);
    mem_free(&mem);
}

/* WFI is a NOP where it is legal: pc + 4 and nothing else changes. */
void test_wfi_by_mode_and_tw(void) {
    const struct { PrivMode mode; uint32_t mstatus; bool traps; } v[] = {
        {PRV_M, 0,                false},
        {PRV_M, MSTATUS_TW,       false},
        {PRV_S, 0,                false},
        {PRV_S, MSTATUS_TW,       true},
        {PRV_U, 0,                true},
        {PRV_U, MSTATUS_TW,       true},
    };
    for (unsigned i = 0; i < sizeof v / sizeof v[0]; i++) {
        if (v[i].traps) {
            assert_illegal_in_mode(v[i].mode, v[i].mstatus, WFI_WORD, "wfi");
            continue;
        }
        CPU cpu; test_cpu_reset_in_mode(&cpu, v[i].mode);
        cpu_write_csr(&cpu, CSR_MSTATUS, v[i].mstatus);
        run_trap_case(&cpu, WFI_WORD);
        TEST_ASSERT_EQUAL_UINT32(0x44, cpu.pc);
        TEST_ASSERT_EQUAL_INT(v[i].mode, cpu.privilege_mode);
        TEST_ASSERT_EQUAL_UINT32(v[i].mstatus, cpu_read_csr(&cpu, CSR_MSTATUS));
    }
}

/* satp is read-only 0, so SFENCE.VMA is illegal in S and U, and a NOP in M. */
void test_sfence_vma_by_mode(void) {
    const uint32_t words[] = {SFENCE_X0_X0, SFENCE_X1_X2};
    for (unsigned i = 0; i < 2; i++) {
        assert_illegal_in_mode(PRV_U, 0, words[i], "sfence.vma");
        assert_illegal_in_mode(PRV_S, 0, words[i], "sfence.vma");
        CPU cpu; cpu_reset(&cpu);
        cpu_write_reg(&cpu, 1, 0xABC);
        cpu_write_reg(&cpu, 2, 0x5);
        run_trap_case(&cpu, words[i]);
        TEST_ASSERT_EQUAL_UINT32(0x44, cpu.pc);
        TEST_ASSERT_EQUAL_INT(PRV_M, cpu.privilege_mode);
    }
}

/* Encodings with no instruction behind them, including DRET (debug mode only). */
void test_unknown_system_encodings_are_illegal(void) {
    const struct { const char *name; uint32_t raw; } v[] = {
        {"dret", DRET_WORD},
        {"uret (removed)", 0x00200073},
        {"imm 0xFFF", 0xFFF00073},
        {"imm 0x106", 0x10600073},
    };
    const PrivMode modes[] = {PRV_U, PRV_S, PRV_M};
    for (unsigned i = 0; i < sizeof v / sizeof v[0]; i++) {
        for (unsigned m = 0; m < 3; m++) {
            assert_illegal_in_mode(modes[m], 0, v[i].raw, v[i].name);
        }
    }
}

/* ECALL, EBREAK, MRET, SRET and WFI fix rd and rs1 to 0; SFENCE.VMA fixes rd. */
void test_reserved_rd_and_rs1_fields_are_illegal(void) {
    const struct { const char *name; uint32_t raw; } v[] = {
        {"ecall rd=1",  0x000000F3}, {"ecall rs1=1",  0x00008073},
        {"ebreak rd=1", 0x001000F3}, {"ebreak rs1=1", 0x00108073},
        {"mret rd=1",   0x302000F3}, {"mret rs1=1",   0x30208073},
        {"sret rd=1",   0x102000F3}, {"sret rs1=1",   0x10208073},
        {"wfi rd=1",    0x105000F3}, {"wfi rs1=1",    0x10508073},
        {"sfence.vma rd=1", 0x120000F3},
    };
    const PrivMode modes[] = {PRV_U, PRV_S, PRV_M};
    for (unsigned i = 0; i < sizeof v / sizeof v[0]; i++) {
        for (unsigned m = 0; m < 3; m++) {
            assert_illegal_in_mode(modes[m], 0, v[i].raw, v[i].name);
        }
    }
}

void test_standard_system_words_still_work(void) {
    CPU cpu; cpu_reset(&cpu);                       // M-mode
    cpu_write_csr(&cpu, CSR_MTVEC, 0x1000);
    run_trap_case(&cpu, 0x00000073);                // ECALL
    TEST_ASSERT_EQUAL_UINT32(11, cpu_read_csr(&cpu, CSR_MCAUSE));
    cpu_reset(&cpu);
    run_trap_case(&cpu, EBREAK_WORD);
    TEST_ASSERT_EQUAL_UINT32(3, cpu_read_csr(&cpu, CSR_MCAUSE));
    cpu_reset(&cpu);
    cpu_write_csr(&cpu, CSR_MEPC, 0x80);
    run_trap_case(&cpu, MRET_WORD);
    TEST_ASSERT_EQUAL_UINT32(0x80, cpu.pc);
}

/* ---- Round trips: trap, handler, return, through cpu_step ------------------------------- */

#define ADDI_X6_X0_7   0x00700313u   /* addi x6, x0, 7 */
#define ADDI_X5_X5_4   0x00428293u   /* addi x5, x5, 4 */
#define CSRR_X5_SEPC   0x141022F3u
#define CSRW_SEPC_X5   0x14129073u
#define CSRR_X5_MEPC   0x341022F3u
#define CSRW_MEPC_X5   0x34129073u

/* ECALL at 0, then `addi x6, x0, 7` at 4. A four-instruction handler at 0x100 reads the return
 * address (xepc), adds 4, writes it back and returns (xret). Six cpu_steps: the ecall, the four
 * handler instructions, and the addi. The ecall traps, so it does not retire: minstret is 5. */
static void run_round_trip(CPU *cpu, uint32_t csr_read, uint32_t csr_write, uint32_t xret,
                           PrivMode mode_in_handler) {
    Memory mem;
    mem_init(&mem, 0x200);
    mem_write32(&mem, 0, ECALL_WORD);
    mem_write32(&mem, 4, ADDI_X6_X0_7);
    mem_write32(&mem, 0x100, csr_read);
    mem_write32(&mem, 0x104, ADDI_X5_X5_4);
    mem_write32(&mem, 0x108, csr_write);
    mem_write32(&mem, 0x10C, xret);

    cpu_step(cpu, &mem);                              // the ecall
    TEST_ASSERT_EQUAL_INT(mode_in_handler, cpu->privilege_mode);
    TEST_ASSERT_EQUAL_UINT32(0x100, cpu->pc);
    for (int i = 0; i < 5; i++) {
        cpu_step(cpu, &mem);                          // csrr, addi, csrw, xret, addi
    }
    mem_free(&mem);

    TEST_ASSERT_EQUAL_UINT32(7, cpu_read_reg(cpu, 6));          // the code after the ecall ran
    TEST_ASSERT_EQUAL_UINT32(8, cpu->pc);
    TEST_ASSERT_EQUAL_UINT64(6, cpu->mcycle);
    TEST_ASSERT_EQUAL_UINT64(5, cpu->minstret);
}

void test_round_trip_u_to_s_and_back(void) {
    CPU cpu; test_cpu_reset_in_mode(&cpu, PRV_U);
    cpu_write_csr(&cpu, CSR_MEDELEG, 0x100);          // ECALL from U goes to S
    cpu_write_csr(&cpu, CSR_STVEC, 0x100);
    cpu_write_csr(&cpu, CSR_MTVEC, 0x1000);
    run_round_trip(&cpu, CSRR_X5_SEPC, CSRW_SEPC_X5, SRET_WORD, PRV_S);
    TEST_ASSERT_EQUAL_INT(PRV_U, cpu.privilege_mode);
    TEST_ASSERT_EQUAL_UINT32(0x20, cpu_read_csr(&cpu, CSR_SSTATUS));   // SPIE = 1, SPP = U
}

void test_round_trip_s_to_s(void) {
    CPU cpu; test_cpu_reset_in_mode(&cpu, PRV_S);
    cpu_write_csr(&cpu, CSR_MEDELEG, 0x200);          // ECALL from S stays in S
    cpu_write_csr(&cpu, CSR_STVEC, 0x100);
    cpu_write_csr(&cpu, CSR_MTVEC, 0x1000);
    run_round_trip(&cpu, CSRR_X5_SEPC, CSRW_SEPC_X5, SRET_WORD, PRV_S);
    TEST_ASSERT_EQUAL_INT(PRV_S, cpu.privilege_mode);
    TEST_ASSERT_EQUAL_UINT32(0x20, cpu_read_csr(&cpu, CSR_SSTATUS));   // SRET cleared SPP again
}

void test_round_trip_u_to_m_and_back(void) {
    CPU cpu; test_cpu_reset_in_mode(&cpu, PRV_U);
    cpu_write_csr(&cpu, CSR_MTVEC, 0x100);            // nothing delegated
    run_round_trip(&cpu, CSRR_X5_MEPC, CSRW_MEPC_X5, MRET_WORD, PRV_M);
    TEST_ASSERT_EQUAL_INT(PRV_U, cpu.privilege_mode);
    TEST_ASSERT_EQUAL_UINT32(0x80, cpu_read_csr(&cpu, CSR_MSTATUS));   // MPIE = 1, MPP = U
}

/* Read-only CSR addresses (csr[11:10] == 0b11), here 0xF14 (mhartid) */
void test_csrrw_read_only_csr_traps(void) {
    assert_csr_access_illegal(0xF1409073, 0xF14, 7); // CSRRW x0, mhartid, x1
}

void test_csrrw_rs1_x0_read_only_csr_traps(void) {
    assert_csr_access_illegal(0xF1401173, 0xF14, 0); // CSRRW x2, mhartid, x0: writes zero
}

void test_csrrs_read_only_csr_traps(void) {
    assert_csr_access_illegal(0xF140A173, 0xF14, 7); // CSRRS x2, mhartid, x1
}

void test_csrrs_read_only_csr_traps_when_rs1_holds_zero(void) {
    assert_csr_access_illegal(0xF140A173, 0xF14, 0); // CSRRS x2, mhartid, x1 with x1 = 0: rs1 != x0
}

void test_csrrc_read_only_csr_traps(void) {
    assert_csr_access_illegal(0xF140B173, 0xF14, 7); // CSRRC x2, mhartid, x1
}

void test_csrrwi_read_only_csr_traps(void) {
    assert_csr_access_illegal(0xF1405173, 0xF14, 0); // CSRRWI x2, mhartid, 0: always writes
}

void test_csrrsi_read_only_csr_traps(void) {
    assert_csr_access_illegal(0xF140E173, 0xF14, 0); // CSRRSI x2, mhartid, 1
}

void test_csrrci_read_only_csr_traps(void) {
    assert_csr_access_illegal(0xF140F173, 0xF14, 0); // CSRRCI x2, mhartid, 1
}

void test_csrrw_top_of_csr_space_traps(void) {
    assert_csr_access_illegal(0xFFF09073, 0xFFF, 7); // CSRRW x0, 0xFFF, x1
}

/* rs1 = x0 / uimm = 0 never writes, so reading a read-only CSR is legal */
void test_csrr_read_only_csr_does_not_trap(void) {
    assert_csr_read_ok(0xF1402173, 0xF14, 0); // CSRRS x2, mhartid, x0 (csrr)
}

void test_csrrsi_uimm0_read_only_csr_does_not_trap(void) {
    assert_csr_read_ok(0xF1406173, 0xF14, 0); // CSRRSI x2, mhartid, 0
}

void test_csrrci_uimm0_read_only_csr_does_not_trap(void) {
    assert_csr_read_ok(0xF1407173, 0xF14, 0); // CSRRCI x2, mhartid, 0
}

/* 0xBFF is read/write by address, but no CSR lives there, so the access is reserved */
void test_csrrw_nonexistent_csr_below_read_only_range_traps(void) {
    assert_csr_access_illegal(0xBFF09073, 0xBFF, 7); // CSRRW x0, 0xBFF, x1
}

/* rs1 = x0 / uimm = 0 must not write the CSR back at all. mtvec's raw storage is set to
 * 0x1003 directly (bypassing the WARL accessor), so a write-back would show up as 0x1000. */
static void assert_csr_no_write_back(uint32_t encoding) {
    CPU cpu; cpu_reset(&cpu);
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

void test_csr_just_below_debug_range_nonexistent_traps(void) {
    assert_csr_access_illegal(0x7AF02173, 0x7AF, 0); // CSRRS x2, 0x7AF, x0: no CSR there
}

void test_csr_just_above_debug_range_nonexistent_traps(void) {
    assert_csr_access_illegal(0x7C002173, 0x7C0, 0); // CSRRS x2, 0x7C0, x0: no CSR there
}

/* Only implemented CSRs exist: any other address is reserved and traps, reads included */
void test_csr_read_nonexistent_traps(void) {
    assert_csr_access_illegal(0x10A02173, 0x10A, 0); // CSRRS x2, 0x10A (senvcfg), x0: optional, absent
}

void test_csr_write_nonexistent_traps(void) {
    assert_csr_access_illegal(0x10A09073, 0x10A, 7); // CSRRW x0, 0x10A, x1
}

void test_csr_read_tcontrol_traps(void) {
    assert_csr_access_illegal(0x7A502173, 0x7A5, 0); // CSRRS x2, tcontrol, x0: optional, absent
}

void test_csr_read_every_implemented_csr_does_not_trap(void) {
    const uint16_t implemented[] = {
        CSR_MSTATUS, CSR_MISA, CSR_MIE, CSR_MTVEC, CSR_MSCRATCH, CSR_MEPC, CSR_MCAUSE,
        CSR_MTVAL, CSR_MIP, CSR_TSELECT, CSR_TDATA1, CSR_TDATA2, CSR_MVENDORID,
        CSR_MARCHID, CSR_MIMPID, CSR_MHARTID,
        CSR_MCYCLE, CSR_MINSTRET, CSR_MCYCLEH, CSR_MINSTRETH,
        CSR_CYCLE, CSR_INSTRET, CSR_CYCLEH, CSR_INSTRETH,
        CSR_MCOUNTEREN, CSR_SCOUNTEREN,
        CSR_SSTATUS, CSR_SIE, CSR_STVEC, CSR_SSCRATCH, CSR_SEPC, CSR_SCAUSE, CSR_STVAL,
        CSR_SIP, CSR_SATP,
        CSR_MEDELEG, CSR_MIDELEG, CSR_MEDELEGH, CSR_MSTATUSH,
    };
    for (size_t i = 0; i < sizeof(implemented) / sizeof(implemented[0]); i++) {
        CPU cpu; cpu_reset(&cpu);
        Memory mem;
        mem_init(&mem, 16);
        cpu_write_csr(&cpu, CSR_MCAUSE, 0x77); // marker: no trap must overwrite it
        mem_write32(&mem, 0, ((uint32_t)implemented[i] << 20) | 0x2173); // CSRRS x2, csr, x0

        uint32_t raw = mem_read32(&mem, cpu.pc);
        DecodedInstruction instr = decode_instruction(raw);
        execute(&cpu, &mem, &instr);

        TEST_ASSERT_EQUAL_UINT32_MESSAGE(4, cpu.pc, "implemented CSR trapped");
        TEST_ASSERT_EQUAL_UINT32(0x77, cpu_read_csr(&cpu, CSR_MCAUSE));
        mem_free(&mem);
    }
}

void test_csrr_misa_reports_rv32ius(void) {
    assert_csr_read_ok(0x30102173, CSR_MISA, MISA_VALUE); // CSRRS x2, misa, x0
}

void test_csrr_tdata1_reads_zero(void) {
    assert_csr_read_ok(0x7A102173, CSR_TDATA1, 0); // CSRRS x2, tdata1, x0
}

/* The probe in riscv-tests breakpoint.S: write tdata1, read it back, and take a mismatch to
 * mean the trigger type is unsupported. Hardwired-zero tdata1 must not echo the write. */
void test_tdata1_write_does_not_read_back(void) {
    CPU cpu; cpu_reset(&cpu);
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_reg(&cpu, 1, 0x20001040); // type 2 (mcontrol), M-mode, execute
    mem_write32(&mem, 0, 0x7A109073);   // CSRRW x0, tdata1, x1
    mem_write32(&mem, 4, 0x7A102173);   // CSRRS x2, tdata1, x0
    for (int i = 0; i < 2; i++) {
        uint32_t raw = mem_read32(&mem, cpu.pc);
        DecodedInstruction instr = decode_instruction(raw);
        execute(&cpu, &mem, &instr);
    }

    TEST_ASSERT_EQUAL_UINT32(8, cpu.pc);
    TEST_ASSERT_EQUAL_UINT32(0, cpu_read_reg(&cpu, 2));
    mem_free(&mem);
}

/* cpu_step: fetch + decode + execute. A fetch outside memory is an instruction access fault
 * (cause 1, mtval = the faulting pc), not an abort. */
static void assert_fetch_fault(Memory *mem, uint32_t start_pc) {
    CPU cpu; cpu_reset(&cpu);
    cpu_write_csr(&cpu, CSR_MTVEC, 0x1000);
    cpu_write_reg(&cpu, 1, 0xDEADBEEF);
    cpu.pc = start_pc;

    cpu_step(&cpu, mem);

    TEST_ASSERT_EQUAL_UINT32(0x1000, cpu.pc);
    TEST_ASSERT_EQUAL_UINT32(start_pc, cpu_read_csr(&cpu, CSR_MEPC));
    TEST_ASSERT_EQUAL_UINT32(1, cpu_read_csr(&cpu, CSR_MCAUSE));
    TEST_ASSERT_EQUAL_UINT32(start_pc, cpu_read_csr(&cpu, CSR_MTVAL));
    TEST_ASSERT_EQUAL_UINT32(0xDEADBEEF, cpu_read_reg(&cpu, 1));
}

void test_cpu_step_executes_fetched_instruction(void) {
    CPU cpu; cpu_reset(&cpu);
    Memory mem;
    mem_init(&mem, 16);
    cpu_write_csr(&cpu, CSR_MCAUSE, 0x77); // marker: no trap must overwrite it
    mem_write32(&mem, 0, 0x00500093); // ADDI x1, x0, 5

    cpu_step(&cpu, &mem);

    TEST_ASSERT_EQUAL_UINT32(5, cpu_read_reg(&cpu, 1));
    TEST_ASSERT_EQUAL_UINT32(4, cpu.pc);
    TEST_ASSERT_EQUAL_UINT32(0x77, cpu_read_csr(&cpu, CSR_MCAUSE));
    mem_free(&mem);
}

void test_cpu_step_two_steps_in_sequence(void) {
    CPU cpu; cpu_reset(&cpu);
    Memory mem;
    mem_init(&mem, 16);
    mem_write32(&mem, 0, 0x00500093); // ADDI x1, x0, 5
    mem_write32(&mem, 4, 0x00A08093); // ADDI x1, x1, 10

    cpu_step(&cpu, &mem);
    cpu_step(&cpu, &mem);

    TEST_ASSERT_EQUAL_UINT32(15, cpu_read_reg(&cpu, 1));
    TEST_ASSERT_EQUAL_UINT32(8, cpu.pc);
    mem_free(&mem);
}

void test_cpu_step_fetches_last_whole_word(void) {
    CPU cpu; cpu_reset(&cpu);
    Memory mem;
    mem_init(&mem, 16);
    cpu_write_csr(&cpu, CSR_MCAUSE, 0x77);
    mem_write32(&mem, 12, 0x00500093); // ADDI x1, x0, 5
    cpu.pc = 12;

    cpu_step(&cpu, &mem);

    TEST_ASSERT_EQUAL_UINT32(5, cpu_read_reg(&cpu, 1));
    TEST_ASSERT_EQUAL_UINT32(16, cpu.pc);
    TEST_ASSERT_EQUAL_UINT32(0x77, cpu_read_csr(&cpu, CSR_MCAUSE));
    mem_free(&mem);
}

void test_cpu_step_based_memory_fetch_works(void) {
    CPU cpu; cpu_reset(&cpu);
    Memory mem;
    mem_init_at(&mem, 0x80000000u, 16);
    mem_write32(&mem, 0x80000000u, 0x00500093); // ADDI x1, x0, 5
    cpu.pc = 0x80000000u;

    cpu_step(&cpu, &mem);

    TEST_ASSERT_EQUAL_UINT32(5, cpu_read_reg(&cpu, 1));
    TEST_ASSERT_EQUAL_UINT32(0x80000004u, cpu.pc);
    mem_free(&mem);
}

void test_cpu_step_fetch_past_end_faults(void) {
    Memory mem;
    mem_init(&mem, 16);
    assert_fetch_fault(&mem, 0x1000);
    mem_free(&mem);
}

void test_cpu_step_fetch_at_size_faults(void) {
    Memory mem;
    mem_init(&mem, 16);
    assert_fetch_fault(&mem, 16);
    mem_free(&mem);
}

void test_cpu_step_fetch_straddling_end_faults(void) {
    Memory mem;
    mem_init(&mem, 14);                // size not a multiple of 4: the word at 12 straddles the end
    assert_fetch_fault(&mem, 12);      // pc must be 4-aligned, since mepc drops the low two bits
    mem_free(&mem);
}

void test_cpu_step_fetch_below_base_faults(void) {
    Memory mem;
    mem_init_at(&mem, 0x80000000u, 16);
    assert_fetch_fault(&mem, 0x7FFFFFFCu);
    mem_free(&mem);
}

void test_cpu_step_fetch_at_zero_with_based_memory_faults(void) {
    Memory mem;
    mem_init_at(&mem, 0x80000000u, 16);
    assert_fetch_fault(&mem, 0);
    mem_free(&mem);
}

void test_cpu_step_fetch_at_top_of_address_space_faults(void) {
    Memory mem;
    mem_init(&mem, 16);
    assert_fetch_fault(&mem, 0xFFFFFFFCu); // must not wrap around into low memory
    mem_free(&mem);
}

void test_cpu_step_fetch_fault_updates_trap_state(void) {
    CPU cpu; cpu_reset(&cpu);
    Memory mem;
    mem_init(&mem, 16);
    cpu_write_csr(&cpu, CSR_MTVEC, 0x1000);
    cpu_write_csr(&cpu, CSR_MSTATUS, MSTATUS_MIE);
    cpu.pc = 0x1000;

    cpu_step(&cpu, &mem);

    uint32_t mstatus = cpu_read_csr(&cpu, CSR_MSTATUS);
    TEST_ASSERT_EQUAL_UINT32(0, mstatus & MSTATUS_MIE);
    TEST_ASSERT_EQUAL_UINT32(MSTATUS_MPIE, mstatus & MSTATUS_MPIE);
    TEST_ASSERT_EQUAL_UINT32(MSTATUS_MPP, mstatus & MSTATUS_MPP);
    mem_free(&mem);
}

void test_cpu_step_fetch_fault_changes_no_registers_or_memory(void) {
    CPU cpu; cpu_reset(&cpu);
    Memory mem;
    mem_init(&mem, 16);
    for (size_t i = 0; i < mem.size; i++) {
        mem.buffer[i] = 0xAA;
    }
    for (uint8_t r = 1; r < 32; r++) {
        cpu_write_reg(&cpu, r, 0x100u + r);
    }
    cpu_write_csr(&cpu, CSR_MTVEC, 0x1000);
    cpu.pc = 0x1000;

    cpu_step(&cpu, &mem);

    for (uint8_t r = 1; r < 32; r++) {
        TEST_ASSERT_EQUAL_UINT32(0x100u + r, cpu_read_reg(&cpu, r));
    }
    for (size_t i = 0; i < mem.size; i++) {
        TEST_ASSERT_EQUAL_UINT8(0xAA, mem.buffer[i]);
    }
    mem_free(&mem);
}

/* FENCE.I (Zifencei, funct3 = 1): a no-op that ignores funct12, rs1 and rd. Every register
 * holds a sentinel, so a wrongly written rd shows up. */
static void assert_fence_i_is_noop(uint32_t encoding) {
    CPU cpu; cpu_reset(&cpu);
    Memory mem;
    mem_init(&mem, 16);

    cpu_write_csr(&cpu, CSR_MCAUSE, 0x77); // marker: no trap must overwrite it
    for (uint8_t r = 1; r < 32; r++) {
        cpu_write_reg(&cpu, r, 0xA000u + r);
    }
    mem_write32(&mem, 0, encoding);

    uint32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(4, cpu.pc);
    TEST_ASSERT_EQUAL_UINT32(0x77, cpu_read_csr(&cpu, CSR_MCAUSE));
    for (uint8_t r = 1; r < 32; r++) {
        TEST_ASSERT_EQUAL_UINT32(0xA000u + r, cpu_read_reg(&cpu, r));
    }
    mem_free(&mem);
}

void test_fence_i(void) {
    assert_fence_i_is_noop(0x0000100F); // FENCE.I
}

void test_fence_i_ignores_rs1(void) {
    assert_fence_i_is_noop(0x0001100F); // rs1 = x2
}

void test_fence_i_ignores_rd(void) {
    assert_fence_i_is_noop(0x0000108F); // rd = x1: must not be written
}

void test_fence_i_ignores_funct12(void) {
    assert_fence_i_is_noop(0xFF00100F); // funct12 = 0xFF0
}

void test_fence_i_ignores_all_reserved_fields(void) {
    assert_fence_i_is_noop(0xFFFF9F8F); // funct12 = 0xFFF, rs1 = x31, rd = x31
}

/* Only funct3 = 0 (FENCE) and 1 (FENCE.I) exist in MISC-MEM */
void test_misc_mem_funct3_2_is_illegal(void) {
    assert_illegal_instruction(0x0000200F);
}

void test_misc_mem_funct3_7_is_illegal(void) {
    assert_illegal_instruction(0x0000700F);
}

/* The pattern FENCE.I exists for: store a new instruction, FENCE.I, run it. */
void test_fence_i_makes_stored_instruction_visible(void) {
    CPU cpu; cpu_reset(&cpu);
    Memory mem;
    mem_init(&mem, 32);

    cpu_write_reg(&cpu, 2, 0x00700093);  // ADDI x1, x0, 7
    mem_write32(&mem, 0, 0x00202623);    // SW x2, 12(x0): overwrite the word at 12
    mem_write32(&mem, 4, 0x0000100F);    // FENCE.I
    mem_write32(&mem, 8, 0x00000013);    // NOP
    mem_write32(&mem, 12, 0x00500093);   // ADDI x1, x0, 5 (about to be replaced)

    for (int i = 0; i < 4; i++) {
        cpu_step(&cpu, &mem);
    }

    TEST_ASSERT_EQUAL_UINT32(16, cpu.pc);
    TEST_ASSERT_EQUAL_UINT32(7, cpu_read_reg(&cpu, 1)); // ran the new instruction, not the old
    mem_free(&mem);
}

/* mstatus.MPP is WARL and holds only implemented modes; U, S and M are implemented. */
void test_mstatus_mpp_probe_reads_back_s(void) {
    /* The sequence riscv-tests illegal.S uses to detect S-mode: clear MPP, set MPP = S,
     * read it back. S is implemented, so it reads back as S. */
    CPU cpu; cpu_reset(&cpu);
    Memory mem;
    mem_init(&mem, 16);
    mem_write32(&mem, 0, 0x3000B073); // CSRRC x0, mstatus, x1
    mem_write32(&mem, 4, 0x3000A073); // CSRRS x0, mstatus, x1
    mem_write32(&mem, 8, 0x30002173); // CSRRS x2, mstatus, x0

    cpu_write_reg(&cpu, 1, MSTATUS_MPP);  // mask: clear both MPP bits
    cpu_step(&cpu, &mem);
    cpu_write_reg(&cpu, 1, 0x800);        // MPP = 01 (S)
    cpu_step(&cpu, &mem);
    cpu_step(&cpu, &mem);

    TEST_ASSERT_EQUAL_UINT32(0x800, cpu_read_reg(&cpu, 2) & MSTATUS_MPP); // 01 (S)
    mem_free(&mem);
}

void test_mstatus_csrrw_can_set_mpp_u(void) {
    CPU cpu; cpu_reset(&cpu);
    Memory mem;
    mem_init(&mem, 16);
    mem_write32(&mem, 0, 0x30009073); // CSRRW x0, mstatus, x1
    mem_write32(&mem, 4, 0x30002173); // CSRRS x2, mstatus, x0

    cpu_write_reg(&cpu, 1, MSTATUS_MIE); // MIE set, MPP bits written as 00 (U)
    cpu_step(&cpu, &mem);
    cpu_step(&cpu, &mem);

    TEST_ASSERT_EQUAL_UINT32(MSTATUS_MIE, cpu_read_reg(&cpu, 2)); // MPP = U sticks
    mem_free(&mem);
}

void test_mret_sets_mpp_to_u(void) {
    CPU cpu; cpu_reset(&cpu);
    Memory mem;
    mem_init(&mem, 16);
    cpu_write_csr(&cpu, CSR_MSTATUS, MSTATUS_MPP); // MPP = M
    mem_write32(&mem, 0, 0x30200073);              // MRET

    cpu_step(&cpu, &mem);

    TEST_ASSERT_EQUAL_UINT32(0, cpu_read_csr(&cpu, CSR_MSTATUS) & MSTATUS_MPP); // least-privileged mode
    mem_free(&mem);
}

/* ---- Counters (mcycle/minstret and their user-level shadows) ---------------------------
 * cpu_step adds one to mcycle every step and one to minstret only when the instruction
 * retires: a trap (ECALL and EBREAK included) does not retire, and a write to a counter
 * suppresses that counter's own increment. */
static void load_nops(Memory *mem, int count) {
    for (int i = 0; i < count; i++) {
        mem_write32(mem, (uint32_t)(4 * i), 0x00000013); // NOP
    }
}

/* Run one CSR read with execute() and return rd (x2). */
static uint32_t read_via_csrr(CPU *cpu, uint32_t encoding) {
    Memory mem;
    mem_init(&mem, 16);
    mem_write32(&mem, 0, encoding);
    cpu->pc = 0;
    cpu_write_reg(cpu, 2, 0xDEADBEEF);
    DecodedInstruction instr = decode_instruction(mem_read32(&mem, 0));
    execute(cpu, &mem, &instr);
    mem_free(&mem);
    return cpu_read_reg(cpu, 2);
}

void test_counters_advance_one_per_instruction(void) {
    CPU cpu; cpu_reset(&cpu);
    Memory mem;
    mem_init(&mem, 64);
    load_nops(&mem, 3);

    for (int i = 0; i < 3; i++) {
        cpu_step(&cpu, &mem);
    }

    TEST_ASSERT_EQUAL_UINT64(3, cpu.mcycle);
    TEST_ASSERT_EQUAL_UINT64(3, cpu.minstret);
    mem_free(&mem);
}

void test_counter_read_returns_count_before_itself(void) {
    CPU cpu; cpu_reset(&cpu);
    Memory mem;
    mem_init(&mem, 64);
    load_nops(&mem, 3);
    mem_write32(&mem, 12, 0xB0202173); // CSRRS x2, minstret, x0

    for (int i = 0; i < 4; i++) {
        cpu_step(&cpu, &mem);
    }

    TEST_ASSERT_EQUAL_UINT32(3, cpu_read_reg(&cpu, 2)); // the three NOPs, not itself
    TEST_ASSERT_EQUAL_UINT64(4, cpu.minstret);          // it retires after reading
    mem_free(&mem);
}

void test_counter_shadows_read_the_machine_counters(void) {
    CPU cpu; cpu_reset(&cpu);
    cpu.mcycle = 0x0000000100000005ull;
    cpu.minstret = 0x0000000200000009ull;

    TEST_ASSERT_EQUAL_UINT32(5, read_via_csrr(&cpu, 0xC0002173));    // cycle
    TEST_ASSERT_EQUAL_UINT32(1, read_via_csrr(&cpu, 0xC8002173));   // cycleh
    TEST_ASSERT_EQUAL_UINT32(9, read_via_csrr(&cpu, 0xC0202173));  // instret
    TEST_ASSERT_EQUAL_UINT32(2, read_via_csrr(&cpu, 0xC8202173)); // instreth
}

void test_csrrw_writes_a_machine_counter(void) {
    CPU cpu; cpu_reset(&cpu);
    Memory mem;
    mem_init(&mem, 16);
    mem_write32(&mem, 0, 0xB0009073); // CSRRW x0, mcycle, x1
    cpu_write_reg(&cpu, 1, 0x1234);

    DecodedInstruction instr = decode_instruction(mem_read32(&mem, 0));
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT64(0x1234, cpu.mcycle);
    mem_free(&mem);
}

void test_illegal_instruction_does_not_retire(void) {
    CPU cpu; cpu_reset(&cpu);
    Memory mem;
    mem_init(&mem, 16);
    cpu_write_csr(&cpu, CSR_MTVEC, 0x1000);
    mem_write32(&mem, 0, 0x00000000); // all zeros: illegal

    cpu_step(&cpu, &mem);

    TEST_ASSERT_EQUAL_UINT64(1, cpu.mcycle);   // it still took a cycle
    TEST_ASSERT_EQUAL_UINT64(0, cpu.minstret); // but did not retire
    mem_free(&mem);
}

void test_ecall_does_not_retire(void) {
    CPU cpu; cpu_reset(&cpu);
    Memory mem;
    mem_init(&mem, 16);
    cpu_write_csr(&cpu, CSR_MTVEC, 0x1000);
    mem_write32(&mem, 0, 0x00000073); // ECALL

    cpu_step(&cpu, &mem);

    TEST_ASSERT_EQUAL_UINT64(1, cpu.mcycle);
    TEST_ASSERT_EQUAL_UINT64(0, cpu.minstret);
    mem_free(&mem);
}

void test_ebreak_does_not_retire(void) {
    CPU cpu; cpu_reset(&cpu);
    Memory mem;
    mem_init(&mem, 16);
    cpu_write_csr(&cpu, CSR_MTVEC, 0x1000);
    mem_write32(&mem, 0, 0x00100073); // EBREAK

    cpu_step(&cpu, &mem);

    TEST_ASSERT_EQUAL_UINT64(1, cpu.mcycle);
    TEST_ASSERT_EQUAL_UINT64(0, cpu.minstret);
    mem_free(&mem);
}

void test_fetch_fault_does_not_retire(void) {
    CPU cpu; cpu_reset(&cpu);
    Memory mem;
    mem_init(&mem, 16);
    cpu_write_csr(&cpu, CSR_MTVEC, 0x1000);
    cpu.pc = 0x1000; // outside memory

    cpu_step(&cpu, &mem);

    TEST_ASSERT_EQUAL_UINT64(1, cpu.mcycle);
    TEST_ASSERT_EQUAL_UINT64(0, cpu.minstret);
    mem_free(&mem);
}

void test_trapping_csr_instruction_does_not_retire(void) {
    CPU cpu; cpu_reset(&cpu);
    Memory mem;
    mem_init(&mem, 16);
    cpu_write_csr(&cpu, CSR_MTVEC, 0x1000);
    mem_write32(&mem, 0, 0xC0009073); // CSRRW x0, cycle, x1: read-only, traps

    cpu_step(&cpu, &mem);

    TEST_ASSERT_EQUAL_UINT64(0, cpu.minstret);
    mem_free(&mem);
}

void test_mret_retires(void) {
    CPU cpu; cpu_reset(&cpu);
    Memory mem;
    mem_init(&mem, 16);
    mem_write32(&mem, 0, 0x30200073); // MRET

    cpu_step(&cpu, &mem);

    TEST_ASSERT_EQUAL_UINT64(1, cpu.minstret);
    mem_free(&mem);
}

void test_minstret_write_suppresses_its_own_increment(void) {
    CPU cpu; cpu_reset(&cpu);
    Memory mem;
    mem_init(&mem, 16);
    cpu.minstret = 5;
    mem_write32(&mem, 0, 0xB0205073); // CSRRWI x0, minstret, 0
    mem_write32(&mem, 4, 0xB0202173); // CSRRS x2, minstret, x0

    cpu_step(&cpu, &mem);
    cpu_step(&cpu, &mem);

    TEST_ASSERT_EQUAL_UINT32(0, cpu_read_reg(&cpu, 2)); // the written value, not written + 1
    TEST_ASSERT_EQUAL_UINT64(1, cpu.minstret);          // the read retired normally
    mem_free(&mem);
}

void test_mcycle_write_suppresses_its_own_increment(void) {
    CPU cpu; cpu_reset(&cpu);
    Memory mem;
    mem_init(&mem, 16);
    cpu.mcycle = 5;
    mem_write32(&mem, 0, 0xB0005073);  // CSRRWI x0, mcycle, 0
    mem_write32(&mem, 4, 0xB0002173);  // CSRRS x2, mcycle, x0

    cpu_step(&cpu, &mem);
    cpu_step(&cpu, &mem);

    TEST_ASSERT_EQUAL_UINT32(0, cpu_read_reg(&cpu, 2));
    TEST_ASSERT_EQUAL_UINT64(1, cpu.mcycle);
    mem_free(&mem);
}

void test_minstret_wraps_at_64_bits(void) {
    CPU cpu; cpu_reset(&cpu);
    Memory mem;
    mem_init(&mem, 16);
    cpu.minstret = 0xFFFFFFFFFFFFFFFFull;
    load_nops(&mem, 1);

    cpu_step(&cpu, &mem);

    TEST_ASSERT_EQUAL_UINT64(0, cpu.minstret);
    mem_free(&mem);
}

/* The carry out of the low half must reach the high half (a 32-bit counter would drop it). */
void test_minstret_carries_into_the_high_half(void) {
    CPU cpu; cpu_reset(&cpu);
    Memory mem;
    mem_init(&mem, 16);
    cpu.minstret = 0x00000000FFFFFFFFull;
    load_nops(&mem, 1);

    cpu_step(&cpu, &mem);

    TEST_ASSERT_EQUAL_UINT64(0x0000000100000000ull, cpu.minstret);
    TEST_ASSERT_EQUAL_UINT32(0, cpu_read_csr(&cpu, CSR_MINSTRET));
    TEST_ASSERT_EQUAL_UINT32(1, cpu_read_csr(&cpu, CSR_MINSTRETH));
    mem_free(&mem);
}

void test_mcycle_carries_into_the_high_half(void) {
    CPU cpu; cpu_reset(&cpu);
    Memory mem;
    mem_init(&mem, 16);
    cpu.mcycle = 0x00000000FFFFFFFFull;
    load_nops(&mem, 1);

    cpu_step(&cpu, &mem);

    TEST_ASSERT_EQUAL_UINT64(0x0000000100000000ull, cpu.mcycle);
    TEST_ASSERT_EQUAL_UINT32(1, cpu_read_csr(&cpu, CSR_MCYCLEH));
    mem_free(&mem);
}

/* The sequence in riscv-tests instret_overflow.S: write all ones to both halves, then one
 * retired instruction wraps the 64-bit counter to zero. */
void test_instret_overflow_sequence(void) {
    CPU cpu; cpu_reset(&cpu);
    Memory mem;
    mem_init(&mem, 64);
    mem_write32(&mem, 0,  0xFFF00293);                 // ADDI x5, x0, -1
    mem_write32(&mem, 4,  0xB0229073);         // CSRRW x0, minstret, x5
    mem_write32(&mem, 8,  0xB8229073);        // CSRRW x0, minstreth, x5
    mem_write32(&mem, 12, 0x00000013);                 // NOP
    mem_write32(&mem, 16, 0xB0202173);         // CSRRS x2, minstret, x0
    mem_write32(&mem, 20, 0xB8202173);        // CSRRS x2, minstreth, x0 (overwrites x2)

    for (int i = 0; i < 5; i++) {
        cpu_step(&cpu, &mem);
    }
    TEST_ASSERT_EQUAL_UINT32(0, cpu_read_reg(&cpu, 2)); // minstret wrapped to 0

    cpu_step(&cpu, &mem);
    TEST_ASSERT_EQUAL_UINT32(0, cpu_read_reg(&cpu, 2)); // and so did minstreth
    mem_free(&mem);
}

/* The user-level shadows are read-only addresses: writing them is an illegal instruction */
void test_counter_shadow_write_traps(void) {
    assert_csr_access_illegal(0xC0009073, CSR_CYCLE, 7);       // CSRRW x0, cycle, x1
}

void test_counter_high_shadow_write_traps(void) {
    assert_csr_access_illegal(0xC8209073, CSR_INSTRETH, 7); // CSRRW x0, instreth, x1
}

void test_mret_returns_to_mepc(void) {
    CPU cpu; cpu_reset(&cpu);
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
    CPU cpu; cpu_reset(&cpu);
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

/* Run the single instruction `raw` at address 0 of a fresh machine with mtvec = 0x100. */
static void run_one(CPU *cpu, uint32_t raw) {
    Memory mem;
    mem_init(&mem, 16);
    mem_write32(&mem, 0, raw);
    cpu_write_csr(cpu, CSR_MTVEC, 0x100);
    DecodedInstruction instr = decode_instruction(raw);
    execute(cpu, &mem, &instr);
    mem_free(&mem);
}

/* ECALL: (mode it runs in) -> (mcause): U -> 8, S -> 9, M -> 11 */
void test_ecall_cause_depends_on_mode(void) {
    const struct { PrivMode mode; uint32_t cause; } v[] = {
        {PRV_U, 8}, {PRV_S, 9}, {PRV_M, 11},
    };
    for (unsigned i = 0; i < sizeof v / sizeof v[0]; i++) {
        CPU cpu; test_cpu_reset_in_mode(&cpu, v[i].mode);
        run_one(&cpu, 0x00000073);
        TEST_ASSERT_EQUAL_UINT32(v[i].cause, cpu_read_csr(&cpu, CSR_MCAUSE));
    }
}

void test_trap_from_u_records_mpp_u_and_enters_m(void) {
    CPU cpu; test_cpu_reset_in_mode(&cpu, PRV_U);
    cpu_write_csr(&cpu, CSR_MSTATUS, MSTATUS_MIE);               // 0x8
    run_one(&cpu, 0x00000073);                                   // ECALL
    TEST_ASSERT_EQUAL_INT(PRV_M, cpu.privilege_mode);
    TEST_ASSERT_EQUAL_UINT32(0x100, cpu.pc);
    TEST_ASSERT_EQUAL_UINT32(0, cpu_read_csr(&cpu, CSR_MEPC));
    TEST_ASSERT_EQUAL_UINT32(0x00000080, cpu_read_csr(&cpu, CSR_MSTATUS)); // MPIE, MIE = 0, MPP = U
}

void test_trap_from_s_records_mpp_s(void) {
    CPU cpu; test_cpu_reset_in_mode(&cpu, PRV_S);
    run_one(&cpu, 0x00000073);                                   // ECALL
    TEST_ASSERT_EQUAL_INT(PRV_M, cpu.privilege_mode);
    TEST_ASSERT_EQUAL_UINT32(0x00000800, cpu_read_csr(&cpu, CSR_MSTATUS)); // MPP = S
}

void test_trap_from_m_records_mpp_m(void) {
    CPU cpu; cpu_reset(&cpu);
    cpu_write_csr(&cpu, CSR_MSTATUS, MSTATUS_MIE);               // 0x8
    run_one(&cpu, 0x00000073);                                   // ECALL
    TEST_ASSERT_EQUAL_INT(PRV_M, cpu.privilege_mode);
    TEST_ASSERT_EQUAL_UINT32(0x00001880, cpu_read_csr(&cpu, CSR_MSTATUS)); // MPIE | MPP = M
}

void test_mret_to_u_switches_mode_and_clears_mprv(void) {
    CPU cpu; cpu_reset(&cpu);
    cpu_write_csr(&cpu, CSR_MEPC, 0x40);
    cpu_write_csr(&cpu, CSR_MSTATUS, 0x00020080);                // MPRV | MPIE, MPP = U
    run_one(&cpu, 0x30200073);                                   // MRET
    TEST_ASSERT_EQUAL_INT(PRV_U, cpu.privilege_mode);
    TEST_ASSERT_EQUAL_UINT32(0x40, cpu.pc);
    TEST_ASSERT_EQUAL_UINT32(0x00000088, cpu_read_csr(&cpu, CSR_MSTATUS)); // MIE, MPIE, MPP = U, MPRV = 0
}

void test_mret_to_s_switches_mode_and_clears_mprv(void) {
    CPU cpu; cpu_reset(&cpu);
    cpu_write_csr(&cpu, CSR_MEPC, 0x40);
    cpu_write_csr(&cpu, CSR_MSTATUS, 0x00020880);                // MPRV | MPP = S | MPIE
    run_one(&cpu, 0x30200073);                                   // MRET
    TEST_ASSERT_EQUAL_INT(PRV_S, cpu.privilege_mode);
    TEST_ASSERT_EQUAL_UINT32(0x00000088, cpu_read_csr(&cpu, CSR_MSTATUS));
}

void test_mret_to_m_keeps_mprv(void) {
    CPU cpu; cpu_reset(&cpu);
    cpu_write_csr(&cpu, CSR_MEPC, 0x40);
    cpu_write_csr(&cpu, CSR_MSTATUS, 0x00021880);                // MPRV | MPP = M | MPIE
    run_one(&cpu, 0x30200073);                                   // MRET
    TEST_ASSERT_EQUAL_INT(PRV_M, cpu.privilege_mode);
    TEST_ASSERT_EQUAL_UINT32(0x00020088, cpu_read_csr(&cpu, CSR_MSTATUS)); // MPRV kept, MPP = U
}

void test_mret_from_lower_mode_is_illegal(void) {
    const PrivMode modes[] = {PRV_U, PRV_S};
    for (unsigned i = 0; i < 2; i++) {
        CPU cpu; test_cpu_reset_in_mode(&cpu, modes[i]);
        run_one(&cpu, 0x30200073);                               // MRET
        TEST_ASSERT_EQUAL_INT(PRV_M, cpu.privilege_mode);
        TEST_ASSERT_EQUAL_UINT32(0x100, cpu.pc);
        TEST_ASSERT_EQUAL_UINT32(2, cpu_read_csr(&cpu, CSR_MCAUSE));
        TEST_ASSERT_EQUAL_UINT32(0x30200073, cpu_read_csr(&cpu, CSR_MTVAL));
        TEST_ASSERT_EQUAL_UINT32(0, cpu_read_csr(&cpu, CSR_MEPC));
        TEST_ASSERT_EQUAL_UINT32((uint32_t)modes[i] << 11, cpu_read_csr(&cpu, CSR_MSTATUS) & MSTATUS_MPP);
    }
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
    RUN_TEST(test_csrrw_top_of_csr_space_traps);
    RUN_TEST(test_csrr_read_only_csr_does_not_trap);
    RUN_TEST(test_csrrsi_uimm0_read_only_csr_does_not_trap);
    RUN_TEST(test_csrrci_uimm0_read_only_csr_does_not_trap);
    RUN_TEST(test_csrrw_nonexistent_csr_below_read_only_range_traps);
    RUN_TEST(test_csrrs_rs1_x0_does_not_write);
    RUN_TEST(test_csrrc_rs1_x0_does_not_write);
    RUN_TEST(test_csrrsi_uimm0_does_not_write);
    RUN_TEST(test_csrrci_uimm0_does_not_write);
    RUN_TEST(test_debug_csr_read_traps);
    RUN_TEST(test_debug_csr_write_traps);
    RUN_TEST(test_debug_csr_range_upper_bound_traps);
    RUN_TEST(test_csr_just_below_debug_range_nonexistent_traps);
    RUN_TEST(test_csr_just_above_debug_range_nonexistent_traps);
    RUN_TEST(test_csr_read_nonexistent_traps);
    RUN_TEST(test_csr_write_nonexistent_traps);
    RUN_TEST(test_csr_read_tcontrol_traps);
    RUN_TEST(test_csr_read_every_implemented_csr_does_not_trap);
    RUN_TEST(test_csr_read_privilege_by_mode);
    RUN_TEST(test_csr_write_forms_follow_privilege);
    RUN_TEST(test_counter_gating);
    RUN_TEST(test_counter_enable_does_not_gate_machine_counters);
    RUN_TEST(test_satp_access_does_not_trap_and_stays_zero);
    RUN_TEST(test_csrrs_sstatus_sets_sie_in_mstatus);
    RUN_TEST(test_csrrc_sstatus_clears_only_s_bits);
    RUN_TEST(test_csrrw_sepc_drops_low_bits);
    RUN_TEST(test_trap_routing);
    RUN_TEST(test_delegated_trap_writes_s_state_and_leaves_m_state);
    RUN_TEST(test_delegated_trap_spp_and_interrupt_enable_stack);
    RUN_TEST(test_delegated_trap_stval_holds_the_instruction_or_address);
    RUN_TEST(test_delegated_trap_does_not_retire);
    RUN_TEST(test_delegation_csrs_through_instructions);
    RUN_TEST(test_sret_table);
    RUN_TEST(test_sret_returns_to_sepc_without_its_low_bits);
    RUN_TEST(test_sret_leaves_the_m_registers_alone);
    RUN_TEST(test_sret_retires);
    RUN_TEST(test_wfi_by_mode_and_tw);
    RUN_TEST(test_sfence_vma_by_mode);
    RUN_TEST(test_unknown_system_encodings_are_illegal);
    RUN_TEST(test_reserved_rd_and_rs1_fields_are_illegal);
    RUN_TEST(test_standard_system_words_still_work);
    RUN_TEST(test_round_trip_u_to_s_and_back);
    RUN_TEST(test_round_trip_s_to_s);
    RUN_TEST(test_round_trip_u_to_m_and_back);
    RUN_TEST(test_csrr_misa_reports_rv32ius);
    RUN_TEST(test_csrr_tdata1_reads_zero);
    RUN_TEST(test_tdata1_write_does_not_read_back);
    RUN_TEST(test_cpu_step_executes_fetched_instruction);
    RUN_TEST(test_cpu_step_two_steps_in_sequence);
    RUN_TEST(test_cpu_step_fetches_last_whole_word);
    RUN_TEST(test_cpu_step_based_memory_fetch_works);
    RUN_TEST(test_cpu_step_fetch_past_end_faults);
    RUN_TEST(test_cpu_step_fetch_at_size_faults);
    RUN_TEST(test_cpu_step_fetch_straddling_end_faults);
    RUN_TEST(test_cpu_step_fetch_below_base_faults);
    RUN_TEST(test_cpu_step_fetch_at_zero_with_based_memory_faults);
    RUN_TEST(test_cpu_step_fetch_at_top_of_address_space_faults);
    RUN_TEST(test_cpu_step_fetch_fault_updates_trap_state);
    RUN_TEST(test_cpu_step_fetch_fault_changes_no_registers_or_memory);
    RUN_TEST(test_fence_i);
    RUN_TEST(test_fence_i_ignores_rs1);
    RUN_TEST(test_fence_i_ignores_rd);
    RUN_TEST(test_fence_i_ignores_funct12);
    RUN_TEST(test_fence_i_ignores_all_reserved_fields);
    RUN_TEST(test_misc_mem_funct3_2_is_illegal);
    RUN_TEST(test_misc_mem_funct3_7_is_illegal);
    RUN_TEST(test_fence_i_makes_stored_instruction_visible);
    RUN_TEST(test_mstatus_mpp_probe_reads_back_s);
    RUN_TEST(test_mstatus_csrrw_can_set_mpp_u);
    RUN_TEST(test_mret_sets_mpp_to_u);
    RUN_TEST(test_counters_advance_one_per_instruction);
    RUN_TEST(test_counter_read_returns_count_before_itself);
    RUN_TEST(test_counter_shadows_read_the_machine_counters);
    RUN_TEST(test_csrrw_writes_a_machine_counter);
    RUN_TEST(test_illegal_instruction_does_not_retire);
    RUN_TEST(test_ecall_does_not_retire);
    RUN_TEST(test_ebreak_does_not_retire);
    RUN_TEST(test_fetch_fault_does_not_retire);
    RUN_TEST(test_trapping_csr_instruction_does_not_retire);
    RUN_TEST(test_mret_retires);
    RUN_TEST(test_minstret_write_suppresses_its_own_increment);
    RUN_TEST(test_mcycle_write_suppresses_its_own_increment);
    RUN_TEST(test_minstret_wraps_at_64_bits);
    RUN_TEST(test_minstret_carries_into_the_high_half);
    RUN_TEST(test_mcycle_carries_into_the_high_half);
    RUN_TEST(test_instret_overflow_sequence);
    RUN_TEST(test_counter_shadow_write_traps);
    RUN_TEST(test_counter_high_shadow_write_traps);
    RUN_TEST(test_mret_returns_to_mepc);
    RUN_TEST(test_mret_restores_mie_from_mpie);
    RUN_TEST(test_ecall_cause_depends_on_mode);
    RUN_TEST(test_trap_from_u_records_mpp_u_and_enters_m);
    RUN_TEST(test_trap_from_s_records_mpp_s);
    RUN_TEST(test_trap_from_m_records_mpp_m);
    RUN_TEST(test_mret_to_u_switches_mode_and_clears_mprv);
    RUN_TEST(test_mret_to_s_switches_mode_and_clears_mprv);
    RUN_TEST(test_mret_to_m_keeps_mprv);
    RUN_TEST(test_mret_from_lower_mode_is_illegal);
	return UNITY_END();
}
