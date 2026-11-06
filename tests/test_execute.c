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
    assert_illegal_instruction(0x042081B3); // funct7=0x02, funct3=0 (funct7=0x01 is MUL since the M extension)
}

void test_illegal_op_funct7_in_sll_slot(void) {
    assert_illegal_instruction(0x042091B3); // funct7=0x02, funct3=1 (funct7=0x01 is MULH since the M extension)
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

void test_csrrw_sepc_drops_bit_zero(void) {
    CPU cpu; test_cpu_reset_in_mode(&cpu, PRV_S);
    cpu_write_reg(&cpu, 1, 0x80000003);
    run_on(&cpu, 0x14109073);                           // CSRRW x0, sepc, x1
    run_on(&cpu, 0x14102173);                           // CSRRS x2, sepc, x0
    TEST_ASSERT_EQUAL_UINT32(0x80000002, cpu_read_reg(&cpu, 2));
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

void test_sret_returns_to_sepc_without_its_bit_zero(void) {
    CPU cpu; test_cpu_reset_in_mode(&cpu, PRV_S);
    cpu_write_csr(&cpu, CSR_MSTATUS, 0x120);
    cpu_write_csr(&cpu, CSR_SEPC, 0x43);
    run_trap_case(&cpu, SRET_WORD);
    TEST_ASSERT_EQUAL_UINT32(0x42, cpu.pc);
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

/* ---- PMP CSRs through instructions ------------------------------------------------------- */

#define CSRRW_PMPCFG0_X1   0x3A009073u   /* CSRRW x0, pmpcfg0, x1 */
#define CSRRS_PMPCFG0_X1   0x3A00A073u   /* CSRRS x0, pmpcfg0, x1 */
#define CSRRC_PMPCFG0_X1   0x3A00B073u   /* CSRRC x0, pmpcfg0, x1 */
#define CSRRW_PMPADDR0_X1  0x3B009073u   /* CSRRW x0, pmpaddr0, x1 */
#define CSRR_X2_PMPADDR0   0x3B002173u   /* CSRRS x2, pmpaddr0, x0 */

/* All 80 PMP CSRs (pmpcfg0-15, pmpaddr0-63) exist and need M-mode. */
void test_pmp_csrs_exist_and_are_m_only(void) {
    for (unsigned csr = 0x3A0; csr <= 0x3EF; csr++) {
        uint32_t enc = ((uint32_t)csr << 20) | 0x2173;       /* CSRRS x2, csr, x0 */
        assert_csr_traps(PRV_M, 0, 0, enc, false, "pmp csr from M");
        assert_csr_traps(PRV_S, 0, 0, enc, true,  "pmp csr from S");
        assert_csr_traps(PRV_U, 0, 0, enc, true,  "pmp csr from U");
    }
}

void test_csrs_next_to_the_pmp_range_do_not_exist(void) {
    assert_csr_traps(PRV_M, 0, 0, (0x39Fu << 20) | 0x2173, true, "0x39F");
    assert_csr_traps(PRV_M, 0, 0, (0x3F0u << 20) | 0x2173, true, "0x3F0");
}

/* The sequence the riscv-tests harness (INIT_PMP) runs. */
void test_pmp_harness_sequence_is_accepted(void) {
    CPU cpu; cpu_reset(&cpu);
    cpu_write_reg(&cpu, 1, 0x7FFFFFFF);
    run_on(&cpu, CSRRW_PMPADDR0_X1);
    TEST_ASSERT_EQUAL_UINT32(4, cpu.pc);                       // no trap
    cpu_write_reg(&cpu, 1, 0x1F);                              // NAPOT | R | W | X
    run_on(&cpu, CSRRW_PMPCFG0_X1);
    TEST_ASSERT_EQUAL_UINT32(4, cpu.pc);
    TEST_ASSERT_EQUAL_UINT32(0x7FFFFFFF, cpu_read_csr(&cpu, CSR_PMPADDR0));
    TEST_ASSERT_EQUAL_UINT32(0x1F, cpu_read_csr(&cpu, CSR_PMPCFG0));
}

/* CSRRS and CSRRC compute a new value from the old one, and a reserved result keeps the old RWX. */
void test_pmpcfg_read_modify_write_applies_the_warl_rule(void) {
    CPU cpu; cpu_reset(&cpu);
    cpu_write_csr(&cpu, CSR_PMPCFG0, 0x01);                    // R
    cpu_write_reg(&cpu, 1, 0x02);
    run_on(&cpu, CSRRS_PMPCFG0_X1);                            // 0x01 | 0x02 = 0x03: RW, legal
    TEST_ASSERT_EQUAL_UINT32(0x03, cpu_read_csr(&cpu, CSR_PMPCFG0));
    cpu_write_reg(&cpu, 1, 0x01);
    run_on(&cpu, CSRRC_PMPCFG0_X1);                            // 0x03 & ~0x01 = 0x02: W only, reserved
    TEST_ASSERT_EQUAL_UINT32(0x03, cpu_read_csr(&cpu, CSR_PMPCFG0));   // RWX keeps its old value
}

void test_pmp_locking_through_instructions(void) {
    CPU cpu; cpu_reset(&cpu);
    cpu_write_reg(&cpu, 1, 0x80);
    run_on(&cpu, CSRRS_PMPCFG0_X1);                            // set L
    TEST_ASSERT_EQUAL_UINT32(0x80, cpu_read_csr(&cpu, CSR_PMPCFG0));
    cpu_write_reg(&cpu, 1, 0x1F);
    run_on(&cpu, CSRRW_PMPCFG0_X1);                            // locked: ignored
    TEST_ASSERT_EQUAL_UINT32(0x80, cpu_read_csr(&cpu, CSR_PMPCFG0));
    cpu_write_reg(&cpu, 1, 0x1234);
    run_on(&cpu, CSRRW_PMPADDR0_X1);                           // locked: ignored
    TEST_ASSERT_EQUAL_UINT32(0, cpu_read_csr(&cpu, CSR_PMPADDR0));
}

void test_pmpaddr_all_ones_reads_back(void) {
    CPU cpu; cpu_reset(&cpu);
    cpu_write_reg(&cpu, 1, 0xFFFFFFFF);
    run_on(&cpu, CSRRW_PMPADDR0_X1);
    run_on(&cpu, CSRR_X2_PMPADDR0);
    TEST_ASSERT_EQUAL_UINT32(0xFFFFFFFF, cpu_read_reg(&cpu, 2));
}

/* ---- PMP enforcement on fetch, load and store -------------------------------------------- */

#define LB_X1_0_X2   0x00010083u
#define LH_X1_0_X2   0x00011083u
#define LBU_X1_0_X2  0x00014083u
#define LHU_X1_0_X2  0x00015083u
#define SB_X3_0_X2   0x00310023u
#define SH_X3_0_X2   0x00311023u
#define SW_X3_0_X2   0x00312023u
#define ADDI_X6_7    0x00700313u
#define PMP_DATA     0x1000u   /* the data region is NAPOT [0x1000, 0x2000) */

/* A machine with an EMPTY PMP (cpu_reset, not the allow-all helper), 16 KiB of memory at 0, a marker
 * word at 0x1000, mtvec = 0x3000, stvec = 0x2000 and a marker in mcause. */
static void pmp_rig(CPU *cpu, Memory *mem, PrivMode mode) {
    cpu_reset(cpu);
    cpu->privilege_mode = mode;
    cpu_write_csr(cpu, CSR_MTVEC, 0x3000);
    cpu_write_csr(cpu, CSR_STVEC, 0x2000);
    cpu_write_csr(cpu, CSR_MCAUSE, 0x77);
    mem_init(mem, 0x4000);
    mem_write32(mem, PMP_DATA, 0xAABBCCDD);
}

/* Entry `i` covers [0x1000, 0x2000) (pmpaddr 0x5FF, NAPOT) with the given config byte. */
static void pmp_data_region(CPU *cpu, unsigned i, uint8_t cfg) {
    cpu->pmpcfg[i] = cfg;
    cpu->pmpaddr[i] = 0x5FF;
}

/* Execute one load or store at pc 0x40 with x2 = addr, x3 = a store value, x1 = a marker. */
static void pmp_run_data(CPU *cpu, Memory *mem, uint32_t raw, uint32_t addr) {
    cpu_write_reg(cpu, 1, 0x11111111);
    cpu_write_reg(cpu, 2, addr);
    cpu_write_reg(cpu, 3, 0x55667788);
    cpu->pc = 0x40;
    DecodedInstruction instr = decode_instruction(raw);
    execute(cpu, mem, &instr);
}

/* The access trapped in M with the given cause and tval, from mode `from`, and nothing else changed. */
static void pmp_assert_fault(CPU *cpu, uint32_t cause, uint32_t tval, uint32_t epc, PrivMode from, const char *what) {
    TEST_ASSERT_EQUAL_INT_MESSAGE(PRV_M, cpu->privilege_mode, what);
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(0x3000, cpu->pc, what);
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(cause, cpu_read_csr(cpu, CSR_MCAUSE), what);
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(tval, cpu_read_csr(cpu, CSR_MTVAL), what);
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(epc, cpu_read_csr(cpu, CSR_MEPC), what);
    TEST_ASSERT_EQUAL_UINT32_MESSAGE((uint32_t)from << 11, cpu_read_csr(cpu, CSR_MSTATUS) & MSTATUS_MPP, what);
}

static void pmp_assert_no_trap(CPU *cpu, const char *what) {
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(0x44, cpu->pc, what);
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(0x77, cpu_read_csr(cpu, CSR_MCAUSE), what);
}

/* Loads and stores from U and from S against one region. fault = the cause, or 0 for allowed. */
void test_pmp_enforced_on_loads_and_stores_from_s_and_u(void) {
    const struct { const char *name; uint8_t cfg; uint32_t raw, addr; uint32_t fault; } v[] = {
        {"R: LW ok",                 0x19, LW_X1_0_X2,  0x1000, 0},
        {"R: SW denied",             0x19, SW_X3_0_X2,  0x1000, 7},
        {"RW: SW ok",                0x1B, SW_X3_0_X2,  0x1000, 0},
        {"none: LW denied",          0x18, LW_X1_0_X2,  0x1000, 5},
        {"R: LW at the top",         0x19, LW_X1_0_X2,  0x1FFC, 0},
        {"R: LW partial overlap",    0x19, LW_X1_0_X2,  0x1FFD, 5},
        {"R: LW misaligned inside",  0x19, LW_X1_0_X2,  0x1001, 0},
        {"R: LW outside, no match",  0x19, LW_X1_0_X2,  0x2000, 5},
        {"RW: LB outside",           0x1B, LB_X1_0_X2,  0x2000, 5},
        {"RW: LH outside",           0x1B, LH_X1_0_X2,  0x2000, 5},
        {"RW: LBU outside",          0x1B, LBU_X1_0_X2, 0x2000, 5},
        {"RW: LHU outside",          0x1B, LHU_X1_0_X2, 0x2000, 5},
        {"RW: SB outside",           0x1B, SB_X3_0_X2,  0x2000, 7},
        {"RW: SH outside",           0x1B, SH_X3_0_X2,  0x2000, 7},
        {"RW: SW outside",           0x1B, SW_X3_0_X2,  0x2000, 7},
    };
    const PrivMode modes[] = {PRV_U, PRV_S};
    for (unsigned m = 0; m < 2; m++) {
        for (unsigned i = 0; i < sizeof v / sizeof v[0]; i++) {
            CPU cpu; Memory mem;
            pmp_rig(&cpu, &mem, modes[m]);
            pmp_data_region(&cpu, 0, v[i].cfg);
            pmp_run_data(&cpu, &mem, v[i].raw, v[i].addr);
            if (v[i].fault == 0) {
                pmp_assert_no_trap(&cpu, v[i].name);
            } else {
                pmp_assert_fault(&cpu, v[i].fault, v[i].addr, 0x40, modes[m], v[i].name);
                TEST_ASSERT_EQUAL_UINT32_MESSAGE(0x11111111, cpu_read_reg(&cpu, 1), v[i].name);     /* rd untouched */
                uint32_t word0 = 0, word_hi = 0xFFFFFFFF;
                mem_try_read32(&mem, 0x1000, &word0);
                mem_try_read32(&mem, 0x2000, &word_hi);
                TEST_ASSERT_EQUAL_UINT32_MESSAGE(0xAABBCCDD, word0, v[i].name);                      /* memory untouched */
                TEST_ASSERT_EQUAL_UINT32_MESSAGE(0, word_hi, v[i].name);
            }
            mem_free(&mem);
        }
    }
}

void test_pmp_allowed_accesses_do_their_work(void) {
    CPU cpu; Memory mem;
    pmp_rig(&cpu, &mem, PRV_U);
    pmp_data_region(&cpu, 0, 0x1B);                              /* RW */
    pmp_run_data(&cpu, &mem, LW_X1_0_X2, 0x1000);
    TEST_ASSERT_EQUAL_UINT32(0xAABBCCDD, cpu_read_reg(&cpu, 1));
    pmp_run_data(&cpu, &mem, SW_X3_0_X2, 0x1004);
    uint32_t stored = 0;
    mem_try_read32(&mem, 0x1004, &stored);
    TEST_ASSERT_EQUAL_UINT32(0x55667788, stored);
    mem_free(&mem);
}

void test_pmp_no_entries_denies_u_and_s_but_the_helper_allows(void) {
    CPU cpu; Memory mem;
    pmp_rig(&cpu, &mem, PRV_U);                                  /* empty PMP */
    pmp_run_data(&cpu, &mem, LW_X1_0_X2, 0x1000);
    pmp_assert_fault(&cpu, 5, 0x1000, 0x40, PRV_U, "no entries: U denied");
    mem_free(&mem);

    pmp_rig(&cpu, &mem, PRV_S);
    pmp_run_data(&cpu, &mem, SW_X3_0_X2, 0x1000);
    pmp_assert_fault(&cpu, 7, 0x1000, 0x40, PRV_S, "no entries: S denied");
    mem_free(&mem);

    mem_init(&mem, 0x4000);                                      /* the allow-all helper */
    test_cpu_reset_in_mode(&cpu, PRV_U);
    cpu_write_csr(&cpu, CSR_MCAUSE, 0x77);
    pmp_run_data(&cpu, &mem, LW_X1_0_X2, 0x1000);
    pmp_assert_no_trap(&cpu, "helper: U allowed");
    mem_free(&mem);
}

/* cpu_step in U with ADDI x6, x0, 7 at `pc`, an X-only or R-only region over [0x1000, 0x2000). */
void test_pmp_enforced_on_fetch(void) {
    const struct { const char *name; uint8_t cfg; uint32_t pc; bool ok; } v[] = {
        {"X: fetch at the base",     0x1C, 0x1000, true },
        {"X: fetch at the last word", 0x1C, 0x1FFC, true },
        {"R only: no execute",       0x19, 0x1000, false},
        {"X: outside, no match",     0x1C, 0x2000, false},
    };
    for (unsigned i = 0; i < sizeof v / sizeof v[0]; i++) {
        CPU cpu; Memory mem;
        pmp_rig(&cpu, &mem, PRV_U);
        pmp_data_region(&cpu, 0, v[i].cfg);
        mem_write32(&mem, v[i].pc, ADDI_X6_7);
        cpu.pc = v[i].pc;
        cpu_step(&cpu, &mem);
        if (v[i].ok) {
            TEST_ASSERT_EQUAL_UINT32_MESSAGE(7, cpu_read_reg(&cpu, 6), v[i].name);
            TEST_ASSERT_EQUAL_UINT32_MESSAGE(v[i].pc + 4, cpu.pc, v[i].name);
            TEST_ASSERT_EQUAL_UINT64_MESSAGE(1, cpu.minstret, v[i].name);
        } else {
            pmp_assert_fault(&cpu, 1, v[i].pc, v[i].pc, PRV_U, v[i].name);
            TEST_ASSERT_EQUAL_UINT32_MESSAGE(0, cpu_read_reg(&cpu, 6), v[i].name);
            TEST_ASSERT_EQUAL_UINT64_MESSAGE(0, cpu.minstret, v[i].name);
        }
        mem_free(&mem);
    }
}

/* M-mode ignores a region with L clear, and is held to it with L set. */
void test_pmp_m_mode_and_the_l_bit(void) {
    CPU cpu; Memory mem;
    pmp_rig(&cpu, &mem, PRV_M);
    pmp_data_region(&cpu, 0, 0x18);                              /* no permissions, L clear */
    pmp_run_data(&cpu, &mem, LW_X1_0_X2, 0x1000);
    pmp_assert_no_trap(&cpu, "M, L clear, no permissions: LW ok");
    mem_free(&mem);

    pmp_rig(&cpu, &mem, PRV_M);
    pmp_data_region(&cpu, 0, 0x99);                              /* R only, L set */
    pmp_run_data(&cpu, &mem, LW_X1_0_X2, 0x1000);
    pmp_assert_no_trap(&cpu, "M, L set, R: LW ok");
    mem_free(&mem);

    pmp_rig(&cpu, &mem, PRV_M);
    pmp_data_region(&cpu, 0, 0x99);
    pmp_run_data(&cpu, &mem, SW_X3_0_X2, 0x1000);
    pmp_assert_fault(&cpu, 7, 0x1000, 0x40, PRV_M, "M, L set, R: SW denied");
    mem_free(&mem);

    pmp_rig(&cpu, &mem, PRV_M);
    pmp_data_region(&cpu, 0, 0x99);                              /* no execute */
    mem_write32(&mem, 0x1000, ADDI_X6_7);
    cpu.pc = 0x1000;
    cpu_step(&cpu, &mem);
    pmp_assert_fault(&cpu, 1, 0x1000, 0x1000, PRV_M, "M fetch, L set, no X");
    mem_free(&mem);
}

/* M-mode, region R only. MPRV = 1 checks loads and stores as MPP; fetches ignore MPRV. */
void test_pmp_mprv(void) {
    const struct { const char *name; uint32_t mstatus; bool region; uint32_t raw; uint32_t fault; } v[] = {
        {"MPRV, MPP=U: LW ok",         0x20000, true,  LW_X1_0_X2, 0},
        {"MPRV, MPP=U: SW denied",     0x20000, true,  SW_X3_0_X2, 7},
        {"MPRV, MPP=S: SW denied",     0x20800, true,  SW_X3_0_X2, 7},
        {"MPRV, MPP=M: SW ok",         0x21800, true,  SW_X3_0_X2, 0},
        {"MPRV clear: SW ok",          0x00000, true,  SW_X3_0_X2, 0},
        {"MPRV, MPP=U, no entries: LW denied", 0x20000, false, LW_X1_0_X2, 5},
        {"MPRV, MPP=M, no entries: LW ok",     0x21800, false, LW_X1_0_X2, 0},
    };
    for (unsigned i = 0; i < sizeof v / sizeof v[0]; i++) {
        CPU cpu; Memory mem;
        pmp_rig(&cpu, &mem, PRV_M);
        if (v[i].region) {
            pmp_data_region(&cpu, 0, 0x19);
        }
        cpu_write_csr(&cpu, CSR_MSTATUS, v[i].mstatus);
        pmp_run_data(&cpu, &mem, v[i].raw, 0x1000);
        if (v[i].fault == 0) {
            pmp_assert_no_trap(&cpu, v[i].name);
        } else {
            pmp_assert_fault(&cpu, v[i].fault, 0x1000, 0x40, PRV_M, v[i].name);
        }
        mem_free(&mem);
    }

    CPU cpu; Memory mem;                                         /* a fetch is not affected by MPRV */
    pmp_rig(&cpu, &mem, PRV_M);
    cpu_write_csr(&cpu, CSR_MSTATUS, 0x20000);                   /* MPRV, MPP = U, and no entries */
    mem_write32(&mem, 0x40, ADDI_X6_7);
    cpu.pc = 0x40;
    cpu_step(&cpu, &mem);
    TEST_ASSERT_EQUAL_UINT32(7, cpu_read_reg(&cpu, 6));
    mem_free(&mem);
}

/* A PMP fault can be delegated to S like any access fault. */
void test_pmp_fault_delegated_to_s(void) {
    CPU cpu; Memory mem;
    pmp_rig(&cpu, &mem, PRV_U);
    pmp_data_region(&cpu, 0, 0x18);                              /* no permissions */
    cpu_write_csr(&cpu, CSR_MEDELEG, 0xA2);                      /* bits 1, 5 and 7: access faults */
    pmp_run_data(&cpu, &mem, LW_X1_0_X2, 0x1000);
    TEST_ASSERT_EQUAL_INT(PRV_S, cpu.privilege_mode);
    TEST_ASSERT_EQUAL_UINT32(0x2000, cpu.pc);
    TEST_ASSERT_EQUAL_UINT32(5, cpu_read_csr(&cpu, CSR_SCAUSE));
    TEST_ASSERT_EQUAL_UINT32(0x1000, cpu_read_csr(&cpu, CSR_STVAL));
    TEST_ASSERT_EQUAL_UINT32(0x40, cpu_read_csr(&cpu, CSR_SEPC));
    TEST_ASSERT_EQUAL_UINT32(0x77, cpu_read_csr(&cpu, CSR_MCAUSE));   /* M registers untouched */
    mem_free(&mem);
}

/* The entry the riscv-tests harness sets up lets U do everything. */
void test_pmp_harness_entry_allows_u(void) {
    CPU cpu; Memory mem;
    pmp_rig(&cpu, &mem, PRV_U);
    cpu.pmpcfg[0] = 0x1F;
    cpu.pmpaddr[0] = 0x7FFFFFFF;
    pmp_run_data(&cpu, &mem, LW_X1_0_X2, 0x1000);
    pmp_assert_no_trap(&cpu, "harness entry: LW");
    pmp_run_data(&cpu, &mem, SW_X3_0_X2, 0x1008);
    pmp_assert_no_trap(&cpu, "harness entry: SW");
    mem_write32(&mem, 0x40, ADDI_X6_7);
    cpu.pc = 0x40;
    cpu_step(&cpu, &mem);
    TEST_ASSERT_EQUAL_UINT32(7, cpu_read_reg(&cpu, 6));
    mem_free(&mem);
}

/* ---- M extension: multiply and divide ----------------------------------------------------- */

#define MUL_X1_X2_X3     0x023100B3u
#define MULH_X1_X2_X3    0x023110B3u
#define MULHSU_X1_X2_X3  0x023120B3u
#define MULHU_X1_X2_X3   0x023130B3u
#define DIV_X1_X2_X3     0x023140B3u
#define DIVU_X1_X2_X3    0x023150B3u
#define REM_X1_X2_X3     0x023160B3u
#define REMU_X1_X2_X3    0x023170B3u

/* x1 = op x2, x3 with x2 = a and x3 = b, on a CPU in `mode`. Returns x1 and checks nothing else moved. */
static uint32_t run_muldiv_in(PrivMode mode, uint32_t raw, uint32_t a, uint32_t b) {
    CPU cpu; test_cpu_reset_in_mode(&cpu, mode);
    cpu_write_csr(&cpu, CSR_MCAUSE, 0x77);
    cpu_write_reg(&cpu, 1, 0xDEADBEEF);
    cpu_write_reg(&cpu, 2, a);
    cpu_write_reg(&cpu, 3, b);
    run_on(&cpu, raw);
    TEST_ASSERT_EQUAL_UINT32(4, cpu.pc);                          /* no trap, whatever the operands */
    TEST_ASSERT_EQUAL_UINT32(0x77, cpu_read_csr(&cpu, CSR_MCAUSE));
    TEST_ASSERT_EQUAL_UINT32(a, cpu_read_reg(&cpu, 2));           /* the sources are untouched */
    TEST_ASSERT_EQUAL_UINT32(b, cpu_read_reg(&cpu, 3));
    return cpu_read_reg(&cpu, 1);
}

static uint32_t run_muldiv(uint32_t raw, uint32_t a, uint32_t b) {
    return run_muldiv_in(PRV_M, raw, a, b);
}

void test_m_multiplies(void) {
    const struct { uint32_t a, b, mul, mulh, mulhsu, mulhu; } v[] = {
        {0x00000003, 0x00000004, 0x0000000C, 0x00000000, 0x00000000, 0x00000000},
        {0xFFFFFFFF, 0xFFFFFFFF, 0x00000001, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFE},
        {0x80000000, 0x80000000, 0x00000000, 0x40000000, 0xC0000000, 0x40000000},
        {0x80000000, 0x00000002, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000001},
        {0x7FFFFFFF, 0x7FFFFFFF, 0x00000001, 0x3FFFFFFF, 0x3FFFFFFF, 0x3FFFFFFF},
        {0xFFFFFFFF, 0x00000001, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000},
        {0x80000000, 0xFFFFFFFF, 0x80000000, 0x00000000, 0x80000000, 0x7FFFFFFF},
        {0x7FFFFFFF, 0xFFFFFFFF, 0x80000001, 0xFFFFFFFF, 0x7FFFFFFE, 0x7FFFFFFE},
    };
    for (unsigned i = 0; i < sizeof v / sizeof v[0]; i++) {
        char msg[48];
        snprintf(msg, sizeof msg, "row %u: a=0x%08x b=0x%08x", i, v[i].a, v[i].b);
        TEST_ASSERT_EQUAL_HEX32_MESSAGE(v[i].mul,    run_muldiv(MUL_X1_X2_X3,    v[i].a, v[i].b), msg);
        TEST_ASSERT_EQUAL_HEX32_MESSAGE(v[i].mulh,   run_muldiv(MULH_X1_X2_X3,   v[i].a, v[i].b), msg);
        TEST_ASSERT_EQUAL_HEX32_MESSAGE(v[i].mulhsu, run_muldiv(MULHSU_X1_X2_X3, v[i].a, v[i].b), msg);
        TEST_ASSERT_EQUAL_HEX32_MESSAGE(v[i].mulhu,  run_muldiv(MULHU_X1_X2_X3,  v[i].a, v[i].b), msg);
    }
}

void test_m_divisions(void) {
    const struct { uint32_t a, b, div, divu, rem, remu; } v[] = {
        {0x00000014, 0x00000006, 0x00000003, 0x00000003, 0x00000002, 0x00000002},
        {0xFFFFFFEC, 0x00000006, 0xFFFFFFFD, 0x2AAAAAA7, 0xFFFFFFFE, 0x00000002},   /* -20 / 6 */
        {0x00000014, 0xFFFFFFFA, 0xFFFFFFFD, 0x00000000, 0x00000002, 0x00000014},   /* 20 / -6 */
        {0xFFFFFFEC, 0xFFFFFFFA, 0x00000003, 0x00000000, 0xFFFFFFFE, 0xFFFFFFEC},   /* -20 / -6 */
        {0x00000007, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000007, 0x00000007},   /* divide by zero */
        {0xFFFFFFF9, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFF9, 0xFFFFFFF9},   /* -7 / 0 */
        {0x80000000, 0xFFFFFFFF, 0x80000000, 0x00000000, 0x00000000, 0x80000000},   /* signed overflow */
        {0x80000000, 0x00000001, 0x80000000, 0x80000000, 0x00000000, 0x00000000},
        {0x00000000, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000000},   /* 0 / 0 */
    };
    for (unsigned i = 0; i < sizeof v / sizeof v[0]; i++) {
        char msg[48];
        snprintf(msg, sizeof msg, "row %u: a=0x%08x b=0x%08x", i, v[i].a, v[i].b);
        TEST_ASSERT_EQUAL_HEX32_MESSAGE(v[i].div,  run_muldiv(DIV_X1_X2_X3,  v[i].a, v[i].b), msg);
        TEST_ASSERT_EQUAL_HEX32_MESSAGE(v[i].divu, run_muldiv(DIVU_X1_X2_X3, v[i].a, v[i].b), msg);
        TEST_ASSERT_EQUAL_HEX32_MESSAGE(v[i].rem,  run_muldiv(REM_X1_X2_X3,  v[i].a, v[i].b), msg);
        TEST_ASSERT_EQUAL_HEX32_MESSAGE(v[i].remu, run_muldiv(REMU_X1_X2_X3, v[i].a, v[i].b), msg);
    }
}

void test_m_works_in_every_mode(void) {
    const PrivMode modes[] = {PRV_U, PRV_S, PRV_M};
    for (unsigned m = 0; m < 3; m++) {
        TEST_ASSERT_EQUAL_HEX32(0x0000000C, run_muldiv_in(modes[m], MUL_X1_X2_X3, 3, 4));
        TEST_ASSERT_EQUAL_HEX32(0x00000003, run_muldiv_in(modes[m], DIV_X1_X2_X3, 20, 6));
    }
}

void test_m_register_aliasing_and_x0(void) {
    CPU cpu; cpu_reset(&cpu);
    cpu_write_reg(&cpu, 1, 7);
    run_on(&cpu, 0x021080B3);                                     /* MUL x1, x1, x1 */
    TEST_ASSERT_EQUAL_UINT32(49, cpu_read_reg(&cpu, 1));

    cpu_reset(&cpu);
    cpu_write_reg(&cpu, 2, 20);
    cpu_write_reg(&cpu, 3, 6);
    run_on(&cpu, 0x02314133);                                     /* DIV x2, x2, x3: rd is also a source */
    TEST_ASSERT_EQUAL_UINT32(3, cpu_read_reg(&cpu, 2));

    cpu_reset(&cpu);
    cpu_write_reg(&cpu, 2, 3);
    cpu_write_reg(&cpu, 3, 4);
    run_on(&cpu, 0x02310033);                                     /* MUL x0, x2, x3 */
    TEST_ASSERT_EQUAL_UINT32(0, cpu_read_reg(&cpu, 0));
}

void test_m_instruction_retires(void) {
    CPU cpu; cpu_reset(&cpu);
    cpu_write_reg(&cpu, 2, 3);
    cpu_write_reg(&cpu, 3, 4);
    Memory mem;
    mem_init(&mem, 64);
    mem_write32(&mem, 0, MUL_X1_X2_X3);
    cpu_step(&cpu, &mem);
    TEST_ASSERT_EQUAL_UINT32(12, cpu_read_reg(&cpu, 1));
    TEST_ASSERT_EQUAL_UINT64(1, cpu.minstret);
    mem_free(&mem);
}

void test_m_reserved_funct7_still_traps(void) {
    assert_illegal_instruction(0x4220A1B3);                       /* funct7=0x21, funct3=2 */
    assert_illegal_instruction(0x462091B3);                       /* funct7=0x23, funct3=1 */
}

/* ---- A extension: atomic memory operations ------------------------------------------------ */

#define AMOADD_W    0x003120AFu   /* AMOADD.W x1, x3, (x2) */
#define AMOSWAP_W   0x083120AFu
#define AMOXOR_W    0x203120AFu
#define AMOOR_W     0x403120AFu
#define AMOAND_W    0x603120AFu
#define AMOMIN_W    0x803120AFu
#define AMOMAX_W    0xA03120AFu
#define AMOMINU_W   0xC03120AFu
#define AMOMAXU_W   0xE03120AFu

/* An empty-PMP machine from pmp_rig (M-mode unless `mode` says otherwise), the word at 0x1000 set to
 * `old`, x1 = a marker, x2 = addr, x3 = src, and one instruction executed at pc 0x40. */
static void amo_run(CPU *cpu, Memory *mem, PrivMode mode, uint32_t raw, uint32_t addr, uint32_t src, uint32_t old) {
    pmp_rig(cpu, mem, mode);
    mem_write32(mem, PMP_DATA, old);
    cpu_write_reg(cpu, 1, 0x11111111);
    cpu_write_reg(cpu, 2, addr);
    cpu_write_reg(cpu, 3, src);
    cpu->pc = 0x40;
    DecodedInstruction instr = decode_instruction(raw);
    execute(cpu, mem, &instr);
}

static uint32_t amo_memory(Memory *mem) {
    uint32_t word = 0;
    mem_try_read32(mem, PMP_DATA, &word);
    return word;
}

void test_amo_operations(void) {
    const struct { const char *name; uint32_t raw, old, src, mem_after; } v[] = {
        {"AMOADD",  AMOADD_W,  0x00000005, 0x00000003, 0x00000008},
        {"AMOADD wraps", AMOADD_W, 0xFFFFFFFF, 0x00000001, 0x00000000},
        {"AMOADD to the sign bit", AMOADD_W, 0x7FFFFFFF, 0x00000001, 0x80000000},
        {"AMOSWAP", AMOSWAP_W, 0x11111111, 0x22222222, 0x22222222},
        {"AMOXOR",  AMOXOR_W,  0xF0F0F0F0, 0xFFFF0000, 0x0F0FF0F0},
        {"AMOOR",   AMOOR_W,   0xF0F0F0F0, 0x0F000F00, 0xFFF0FFF0},
        {"AMOAND",  AMOAND_W,  0xF0F0F0F0, 0xFFFF0000, 0xF0F00000},
        {"AMOMIN -1 vs 1",       AMOMIN_W,  0xFFFFFFFF, 0x00000001, 0xFFFFFFFF},
        {"AMOMIN 0x80000000",    AMOMIN_W,  0x80000000, 0x7FFFFFFF, 0x80000000},
        {"AMOMIN 5 vs 9",        AMOMIN_W,  0x00000005, 0x00000009, 0x00000005},
        {"AMOMIN 9 vs 5",        AMOMIN_W,  0x00000009, 0x00000005, 0x00000005},
        {"AMOMAX -1 vs 1",       AMOMAX_W,  0xFFFFFFFF, 0x00000001, 0x00000001},
        {"AMOMAX 0x80000000",    AMOMAX_W,  0x80000000, 0x7FFFFFFF, 0x7FFFFFFF},
        {"AMOMAX 5 vs 9",        AMOMAX_W,  0x00000005, 0x00000009, 0x00000009},
        {"AMOMAX 9 vs 5",        AMOMAX_W,  0x00000009, 0x00000005, 0x00000009},
        {"AMOMINU 0xFFFFFFFF",   AMOMINU_W, 0xFFFFFFFF, 0x00000001, 0x00000001},
        {"AMOMINU 0x80000000",   AMOMINU_W, 0x80000000, 0x7FFFFFFF, 0x7FFFFFFF},
        {"AMOMINU 5 vs 9",       AMOMINU_W, 0x00000005, 0x00000009, 0x00000005},
        {"AMOMINU 9 vs 5",       AMOMINU_W, 0x00000009, 0x00000005, 0x00000005},
        {"AMOMAXU 0xFFFFFFFF",   AMOMAXU_W, 0xFFFFFFFF, 0x00000001, 0xFFFFFFFF},
        {"AMOMAXU 0x80000000",   AMOMAXU_W, 0x80000000, 0x7FFFFFFF, 0x80000000},
        {"AMOMAXU 5 vs 9",       AMOMAXU_W, 0x00000005, 0x00000009, 0x00000009},
        {"AMOMAXU 9 vs 5",       AMOMAXU_W, 0x00000009, 0x00000005, 0x00000009},
    };
    for (unsigned i = 0; i < sizeof v / sizeof v[0]; i++) {
        CPU cpu; Memory mem;
        amo_run(&cpu, &mem, PRV_M, v[i].raw, PMP_DATA, v[i].src, v[i].old);
        pmp_assert_no_trap(&cpu, v[i].name);
        TEST_ASSERT_EQUAL_HEX32_MESSAGE(v[i].old, cpu_read_reg(&cpu, 1), v[i].name);        /* rd = the old value */
        TEST_ASSERT_EQUAL_HEX32_MESSAGE(v[i].mem_after, amo_memory(&mem), v[i].name);
        TEST_ASSERT_EQUAL_HEX32_MESSAGE(PMP_DATA, cpu_read_reg(&cpu, 2), v[i].name);        /* sources untouched */
        TEST_ASSERT_EQUAL_HEX32_MESSAGE(v[i].src, cpu_read_reg(&cpu, 3), v[i].name);
        mem_free(&mem);
    }
}

void test_amo_aq_and_rl_are_ignored(void) {
    CPU cpu; Memory mem;
    amo_run(&cpu, &mem, PRV_M, 0x063120AF, PMP_DATA, 3, 5);       /* AMOADD.W.aq.rl */
    pmp_assert_no_trap(&cpu, "aq.rl");
    TEST_ASSERT_EQUAL_HEX32(8, amo_memory(&mem));
    TEST_ASSERT_EQUAL_HEX32(5, cpu_read_reg(&cpu, 1));
    mem_free(&mem);
}

void test_amo_with_rd_x0_still_updates_memory(void) {
    CPU cpu; Memory mem;
    amo_run(&cpu, &mem, PRV_M, 0x0031202F, PMP_DATA, 3, 5);       /* AMOADD.W x0, x3, (x2) */
    pmp_assert_no_trap(&cpu, "rd = x0");
    TEST_ASSERT_EQUAL_HEX32(8, amo_memory(&mem));
    TEST_ASSERT_EQUAL_HEX32(0, cpu_read_reg(&cpu, 0));
    mem_free(&mem);
}

/* rd may be the same register as rs1 or rs2: both sources are read before rd is written. */
void test_amo_rd_may_alias_a_source(void) {
    CPU cpu; Memory mem;
    amo_run(&cpu, &mem, PRV_M, 0x003121AF, PMP_DATA, 3, 5);       /* AMOADD.W x3, x3, (x2): rd == rs2 */
    pmp_assert_no_trap(&cpu, "rd == rs2");
    TEST_ASSERT_EQUAL_HEX32(5, cpu_read_reg(&cpu, 3));            /* the old value */
    TEST_ASSERT_EQUAL_HEX32(8, amo_memory(&mem));                 /* old + the ORIGINAL x3 */
    mem_free(&mem);

    amo_run(&cpu, &mem, PRV_M, 0x0031212F, PMP_DATA, 3, 5);       /* AMOADD.W x2, x3, (x2): rd == rs1 */
    pmp_assert_no_trap(&cpu, "rd == rs1");
    TEST_ASSERT_EQUAL_HEX32(5, cpu_read_reg(&cpu, 2));
    TEST_ASSERT_EQUAL_HEX32(8, amo_memory(&mem));                 /* the address was read before rd changed */
    mem_free(&mem);
}

void test_amo_misaligned_address_raises_address_misaligned(void) {
    for (uint32_t addr = 0x1001; addr <= 0x1003; addr++) {
        CPU cpu; Memory mem;
        amo_run(&cpu, &mem, PRV_M, AMOADD_W, addr, 3, 5);
        pmp_assert_fault(&cpu, 6, addr, 0x40, PRV_M, "misaligned AMO");
        TEST_ASSERT_EQUAL_HEX32(0x11111111, cpu_read_reg(&cpu, 1));    /* rd untouched */
        TEST_ASSERT_EQUAL_HEX32(5, amo_memory(&mem));                  /* memory untouched */
        mem_free(&mem);
    }
}

void test_amo_outside_memory_is_a_store_access_fault(void) {
    const uint32_t addrs[] = {0x4000, 0xFFFFFFFC};
    for (unsigned i = 0; i < 2; i++) {
        CPU cpu; Memory mem;
        amo_run(&cpu, &mem, PRV_M, AMOADD_W, addrs[i], 3, 5);
        pmp_assert_fault(&cpu, 7, addrs[i], 0x40, PRV_M, "AMO outside memory");
        TEST_ASSERT_EQUAL_HEX32(0x11111111, cpu_read_reg(&cpu, 1));
        mem_free(&mem);
    }
}

/* AMOSWAP over a region [0x1000, 0x2000): old = 0x00AB, src = 0x55667788. */
void test_amo_pmp(void) {
    const struct { const char *name; PrivMode mode; uint8_t cfg; bool region; uint32_t mstatus; bool ok; } v[] = {
        {"U, read-only region",          PRV_U, 0x19, true,  0,       false},
        {"U, read-write region",         PRV_U, 0x1B, true,  0,       true },
        {"U, no entries",                PRV_U, 0,    false, 0,       false},
        {"S, read-write region",         PRV_S, 0x1B, true,  0,       true },
        {"M, L clear, read-only region", PRV_M, 0x19, true,  0,       true },
        {"M, L set, read-only region",   PRV_M, 0x99, true,  0,       false},
        {"M, MPRV with MPP=U, read-only", PRV_M, 0x19, true, 0x20000, false},
        {"M, MPRV with MPP=U, read-write", PRV_M, 0x1B, true, 0x20000, true },
    };
    for (unsigned i = 0; i < sizeof v / sizeof v[0]; i++) {
        CPU cpu; Memory mem;
        pmp_rig(&cpu, &mem, v[i].mode);
        if (v[i].region) {
            pmp_data_region(&cpu, 0, v[i].cfg);
        }
        cpu_write_csr(&cpu, CSR_MSTATUS, v[i].mstatus);
        mem_write32(&mem, PMP_DATA, 0xAB);
        cpu_write_reg(&cpu, 1, 0x11111111);
        cpu_write_reg(&cpu, 2, PMP_DATA);
        cpu_write_reg(&cpu, 3, 0x55667788);
        cpu.pc = 0x40;
        DecodedInstruction instr = decode_instruction(AMOSWAP_W);
        execute(&cpu, &mem, &instr);
        if (v[i].ok) {
            pmp_assert_no_trap(&cpu, v[i].name);
            TEST_ASSERT_EQUAL_HEX32_MESSAGE(0xAB, cpu_read_reg(&cpu, 1), v[i].name);
            TEST_ASSERT_EQUAL_HEX32_MESSAGE(0x55667788, amo_memory(&mem), v[i].name);
        } else {
            pmp_assert_fault(&cpu, 7, PMP_DATA, 0x40, v[i].mode, v[i].name);
            TEST_ASSERT_EQUAL_HEX32_MESSAGE(0x11111111, cpu_read_reg(&cpu, 1), v[i].name);
            TEST_ASSERT_EQUAL_HEX32_MESSAGE(0xAB, amo_memory(&mem), v[i].name);        /* memory untouched */
        }
        mem_free(&mem);
    }
}

void test_amo_pmp_fault_can_be_delegated_to_s(void) {
    CPU cpu; Memory mem;
    pmp_rig(&cpu, &mem, PRV_U);
    pmp_data_region(&cpu, 0, 0x19);                               /* read-only */
    cpu_write_csr(&cpu, CSR_MEDELEG, 0x80);                       /* bit 7: store/AMO access fault */
    mem_write32(&mem, PMP_DATA, 0xAB);
    cpu_write_reg(&cpu, 2, PMP_DATA);
    cpu.pc = 0x40;
    DecodedInstruction instr = decode_instruction(AMOSWAP_W);
    execute(&cpu, &mem, &instr);
    TEST_ASSERT_EQUAL_INT(PRV_S, cpu.privilege_mode);
    TEST_ASSERT_EQUAL_UINT32(7, cpu_read_csr(&cpu, CSR_SCAUSE));
    TEST_ASSERT_EQUAL_UINT32(PMP_DATA, cpu_read_csr(&cpu, CSR_STVAL));
    TEST_ASSERT_EQUAL_UINT32(0x40, cpu_read_csr(&cpu, CSR_SEPC));
    mem_free(&mem);
}

/* funct3 other than 010, and unused funct5 values. */
void test_amo_reserved_encodings_are_illegal(void) {
    const struct { const char *name; uint32_t raw; } v[] = {
        {"funct3 = 3 (the doubleword form)", 0x003130AF},
        {"funct5 = 0x05",                    0x283120AF},
        {"funct5 = 0x06",                    0x303120AF},
        {"funct5 = 0x1F",                    0xF83120AF},
    };
    const PrivMode modes[] = {PRV_U, PRV_S, PRV_M};
    for (unsigned i = 0; i < sizeof v / sizeof v[0]; i++) {
        for (unsigned m = 0; m < 3; m++) {
            assert_illegal_in_mode(modes[m], 0, v[i].raw, v[i].name);
        }
    }
}

void test_amo_retires_and_a_faulting_amo_does_not(void) {
    CPU cpu; Memory mem;
    pmp_rig(&cpu, &mem, PRV_M);
    mem_write32(&mem, PMP_DATA, 5);
    mem_write32(&mem, 0x40, AMOADD_W);
    cpu_write_reg(&cpu, 2, PMP_DATA);
    cpu_write_reg(&cpu, 3, 3);
    cpu.pc = 0x40;
    cpu_step(&cpu, &mem);
    TEST_ASSERT_EQUAL_UINT64(1, cpu.minstret);
    TEST_ASSERT_EQUAL_HEX32(8, amo_memory(&mem));
    mem_free(&mem);

    pmp_rig(&cpu, &mem, PRV_M);
    mem_write32(&mem, 0x40, AMOADD_W);
    cpu_write_reg(&cpu, 2, PMP_DATA + 1);                         /* misaligned */
    cpu.pc = 0x40;
    cpu_step(&cpu, &mem);
    TEST_ASSERT_EQUAL_UINT64(0, cpu.minstret);
    mem_free(&mem);
}

/* ---- A extension: load-reserved and store-conditional ------------------------------------- */

#define LR_W   0x100120AFu   /* LR.W x1, (x2) */
#define SC_W   0x183120AFu   /* SC.W x1, x3, (x2) */
#define LRSC_OTHER 0x1004u   /* a second word in the same region */

/* One instruction at pc 0x40 on a machine that is already set up: x1 = a marker, x2 = addr, x3 = src. */
static void lrsc_exec(CPU *cpu, Memory *mem, uint32_t raw, uint32_t addr, uint32_t src) {
    cpu_write_reg(cpu, 1, 0x11111111);
    cpu_write_reg(cpu, 2, addr);
    cpu_write_reg(cpu, 3, src);
    cpu->pc = 0x40;
    DecodedInstruction instr = decode_instruction(raw);
    execute(cpu, mem, &instr);
}

/* An empty-PMP M-mode machine with 0x1000 = 0xDEADBEEF and 0x1004 = 0xCAFEF00D. */
static void lrsc_rig(CPU *cpu, Memory *mem) {
    pmp_rig(cpu, mem, PRV_M);
    mem_write32(mem, PMP_DATA, 0xDEADBEEF);
    mem_write32(mem, LRSC_OTHER, 0xCAFEF00D);
}

static uint32_t lrsc_word(Memory *mem, uint32_t addr) {
    uint32_t word = 0;
    mem_try_read32(mem, addr, &word);
    return word;
}

void test_lr_loads_the_word_and_registers_a_reservation(void) {
    CPU cpu; Memory mem;
    lrsc_rig(&cpu, &mem);
    lrsc_exec(&cpu, &mem, LR_W, PMP_DATA, 0);
    pmp_assert_no_trap(&cpu, "LR");
    TEST_ASSERT_EQUAL_HEX32(0xDEADBEEF, cpu_read_reg(&cpu, 1));
    TEST_ASSERT_TRUE(cpu.lr_valid);
    TEST_ASSERT_EQUAL_HEX32(PMP_DATA, cpu.lr_addr);
    mem_free(&mem);
}

void test_lr_with_rd_x0_still_registers_a_reservation(void) {
    CPU cpu; Memory mem;
    lrsc_rig(&cpu, &mem);
    lrsc_exec(&cpu, &mem, 0x1000202F, PMP_DATA, 0);               /* LR.W x0, (x2) */
    pmp_assert_no_trap(&cpu, "LR rd = x0");
    TEST_ASSERT_TRUE(cpu.lr_valid);
    mem_free(&mem);
}

void test_sc_after_lr_to_the_same_address_succeeds(void) {
    CPU cpu; Memory mem;
    lrsc_rig(&cpu, &mem);
    lrsc_exec(&cpu, &mem, LR_W, PMP_DATA, 0);
    lrsc_exec(&cpu, &mem, SC_W, PMP_DATA, 0x1234);
    pmp_assert_no_trap(&cpu, "SC");
    TEST_ASSERT_EQUAL_HEX32(0, cpu_read_reg(&cpu, 1));
    TEST_ASSERT_EQUAL_HEX32(0x1234, lrsc_word(&mem, PMP_DATA));
    TEST_ASSERT_FALSE(cpu.lr_valid);
    mem_free(&mem);
}

void test_sc_without_a_reservation_fails(void) {
    CPU cpu; Memory mem;
    lrsc_rig(&cpu, &mem);
    lrsc_exec(&cpu, &mem, SC_W, PMP_DATA, 0x1234);
    pmp_assert_no_trap(&cpu, "SC");
    TEST_ASSERT_EQUAL_HEX32(1, cpu_read_reg(&cpu, 1));
    TEST_ASSERT_EQUAL_HEX32(0xDEADBEEF, lrsc_word(&mem, PMP_DATA));
    mem_free(&mem);
}

void test_sc_to_another_address_fails_and_clears_the_reservation(void) {
    CPU cpu; Memory mem;
    lrsc_rig(&cpu, &mem);
    lrsc_exec(&cpu, &mem, LR_W, PMP_DATA, 0);
    lrsc_exec(&cpu, &mem, SC_W, LRSC_OTHER, 0x1234);
    TEST_ASSERT_EQUAL_HEX32(1, cpu_read_reg(&cpu, 1));
    TEST_ASSERT_EQUAL_HEX32(0xCAFEF00D, lrsc_word(&mem, LRSC_OTHER));
    TEST_ASSERT_FALSE(cpu.lr_valid);
    lrsc_exec(&cpu, &mem, SC_W, PMP_DATA, 0x1234);                /* the failed SC cleared it */
    TEST_ASSERT_EQUAL_HEX32(1, cpu_read_reg(&cpu, 1));
    TEST_ASSERT_EQUAL_HEX32(0xDEADBEEF, lrsc_word(&mem, PMP_DATA));
    mem_free(&mem);
}

void test_a_successful_sc_uses_up_the_reservation(void) {
    CPU cpu; Memory mem;
    lrsc_rig(&cpu, &mem);
    lrsc_exec(&cpu, &mem, LR_W, PMP_DATA, 0);
    lrsc_exec(&cpu, &mem, SC_W, PMP_DATA, 0x1111);
    lrsc_exec(&cpu, &mem, SC_W, PMP_DATA, 0x2222);
    TEST_ASSERT_EQUAL_HEX32(1, cpu_read_reg(&cpu, 1));
    TEST_ASSERT_EQUAL_HEX32(0x1111, lrsc_word(&mem, PMP_DATA));
    mem_free(&mem);
}

void test_a_later_lr_replaces_the_reservation(void) {
    CPU cpu; Memory mem;
    lrsc_rig(&cpu, &mem);
    lrsc_exec(&cpu, &mem, LR_W, PMP_DATA, 0);
    lrsc_exec(&cpu, &mem, LR_W, LRSC_OTHER, 0);
    lrsc_exec(&cpu, &mem, SC_W, PMP_DATA, 0x1234);                /* the first address is no longer reserved */
    TEST_ASSERT_EQUAL_HEX32(1, cpu_read_reg(&cpu, 1));
    TEST_ASSERT_EQUAL_HEX32(0xDEADBEEF, lrsc_word(&mem, PMP_DATA));

    lrsc_exec(&cpu, &mem, LR_W, PMP_DATA, 0);
    lrsc_exec(&cpu, &mem, LR_W, LRSC_OTHER, 0);
    lrsc_exec(&cpu, &mem, SC_W, LRSC_OTHER, 0x5678);
    TEST_ASSERT_EQUAL_HEX32(0, cpu_read_reg(&cpu, 1));
    TEST_ASSERT_EQUAL_HEX32(0x5678, lrsc_word(&mem, LRSC_OTHER));
    mem_free(&mem);
}

void test_lr_sc_aq_and_rl_are_ignored(void) {
    CPU cpu; Memory mem;
    lrsc_rig(&cpu, &mem);
    lrsc_exec(&cpu, &mem, 0x140120AF, PMP_DATA, 0);               /* LR.W.aq */
    TEST_ASSERT_EQUAL_HEX32(0xDEADBEEF, cpu_read_reg(&cpu, 1));
    lrsc_exec(&cpu, &mem, 0x1E3120AF, PMP_DATA, 0x1234);          /* SC.W.aq.rl */
    TEST_ASSERT_EQUAL_HEX32(0, cpu_read_reg(&cpu, 1));
    TEST_ASSERT_EQUAL_HEX32(0x1234, lrsc_word(&mem, PMP_DATA));
    mem_free(&mem);
}

void test_sc_rd_may_be_rs2(void) {
    CPU cpu; Memory mem;
    lrsc_rig(&cpu, &mem);
    lrsc_exec(&cpu, &mem, LR_W, PMP_DATA, 0);
    lrsc_exec(&cpu, &mem, 0x183121AF, PMP_DATA, 0x1234);          /* SC.W x3, x3, (x2) */
    TEST_ASSERT_EQUAL_HEX32(0, cpu_read_reg(&cpu, 3));            /* the status replaces rs2 ... */
    TEST_ASSERT_EQUAL_HEX32(0x1234, lrsc_word(&mem, PMP_DATA));   /* ... after the store used the original */
    mem_free(&mem);
}

void test_lr_sc_misaligned_address_raises_address_misaligned(void) {
    for (uint32_t addr = PMP_DATA + 1; addr <= PMP_DATA + 3; addr++) {
        CPU cpu; Memory mem;
        lrsc_rig(&cpu, &mem);
        lrsc_exec(&cpu, &mem, LR_W, addr, 0);
        pmp_assert_fault(&cpu, 4, addr, 0x40, PRV_M, "misaligned LR");
        TEST_ASSERT_FALSE(cpu.lr_valid);
        TEST_ASSERT_EQUAL_HEX32(0x11111111, cpu_read_reg(&cpu, 1));
        mem_free(&mem);

        lrsc_rig(&cpu, &mem);
        lrsc_exec(&cpu, &mem, SC_W, addr, 0x1234);
        pmp_assert_fault(&cpu, 6, addr, 0x40, PRV_M, "misaligned SC");
        TEST_ASSERT_EQUAL_HEX32(0x11111111, cpu_read_reg(&cpu, 1));
        mem_free(&mem);
    }
}

void test_lr_sc_outside_memory_is_an_access_fault(void) {
    const uint32_t addrs[] = {0x4000, 0xFFFFFFFC};
    for (unsigned i = 0; i < 2; i++) {
        CPU cpu; Memory mem;
        lrsc_rig(&cpu, &mem);
        lrsc_exec(&cpu, &mem, LR_W, addrs[i], 0);
        pmp_assert_fault(&cpu, 5, addrs[i], 0x40, PRV_M, "LR outside memory");
        TEST_ASSERT_FALSE(cpu.lr_valid);
        mem_free(&mem);

        lrsc_rig(&cpu, &mem);                                     /* no reservation: the SC would fail, but still faults */
        lrsc_exec(&cpu, &mem, SC_W, addrs[i], 0x1234);
        pmp_assert_fault(&cpu, 7, addrs[i], 0x40, PRV_M, "SC outside memory");
        mem_free(&mem);
    }
}

/* LR needs read permission, SC needs write permission, and an SC that would fail needs it too. */
void test_lr_sc_pmp(void) {
    const struct { const char *name; PrivMode mode; uint8_t cfg; uint32_t raw; bool ok; uint32_t fault; } v[] = {
        {"LR, U, read-only",     PRV_U, 0x19, LR_W, true,  0},
        {"LR, U, execute-only",  PRV_U, 0x1C, LR_W, false, 5},
        {"LR, U, no entries",    PRV_U, 0,    LR_W, false, 5},
        {"LR, S, read-only",     PRV_S, 0x19, LR_W, true,  0},
        {"SC, U, read-write",    PRV_U, 0x1B, SC_W, true,  0},
        {"SC, U, read-only",     PRV_U, 0x19, SC_W, false, 7},
        {"SC, S, read-only",     PRV_S, 0x19, SC_W, false, 7},
        {"SC, U, no entries",    PRV_U, 0,    SC_W, false, 7},
    };
    for (unsigned i = 0; i < sizeof v / sizeof v[0]; i++) {
        CPU cpu; Memory mem;
        pmp_rig(&cpu, &mem, v[i].mode);
        if (v[i].cfg) {
            pmp_data_region(&cpu, 0, v[i].cfg);
        }
        mem_write32(&mem, PMP_DATA, 0xDEADBEEF);
        lrsc_exec(&cpu, &mem, v[i].raw, PMP_DATA, 0x1234);
        if (v[i].ok) {
            pmp_assert_no_trap(&cpu, v[i].name);
        } else {
            pmp_assert_fault(&cpu, v[i].fault, PMP_DATA, 0x40, v[i].mode, v[i].name);
            TEST_ASSERT_EQUAL_HEX32_MESSAGE(0x11111111, cpu_read_reg(&cpu, 1), v[i].name);
            TEST_ASSERT_EQUAL_HEX32_MESSAGE(0xDEADBEEF, lrsc_word(&mem, PMP_DATA), v[i].name);
        }
        mem_free(&mem);
    }
}

void test_lr_with_nonzero_rs2_and_other_funct3_are_illegal(void) {
    const struct { const char *name; uint32_t raw; } v[] = {
        {"LR.W with rs2 = x3",         0x103120AF},
        {"LR.W with rs2 = x1",         0x101120AF},
        {"LR with funct3 = 3",         0x100130AF},
        {"SC with funct3 = 3",         0x183130AF},
    };
    const PrivMode modes[] = {PRV_U, PRV_S, PRV_M};
    for (unsigned i = 0; i < sizeof v / sizeof v[0]; i++) {
        for (unsigned m = 0; m < 3; m++) {
            assert_illegal_in_mode(modes[m], 0, v[i].raw, v[i].name);
        }
    }
}

void test_a_faulting_lr_or_sc_keeps_the_reservation(void) {
    CPU cpu; Memory mem;
    lrsc_rig(&cpu, &mem);
    lrsc_exec(&cpu, &mem, LR_W, PMP_DATA, 0);
    lrsc_exec(&cpu, &mem, SC_W, PMP_DATA + 1, 0x1234);            /* misaligned SC traps */
    TEST_ASSERT_TRUE(cpu.lr_valid);
    lrsc_exec(&cpu, &mem, LR_W, 0x4000, 0);                       /* LR outside memory traps */
    TEST_ASSERT_TRUE(cpu.lr_valid);
    lrsc_exec(&cpu, &mem, SC_W, 0x4000, 0x1234);                  /* SC access fault traps */
    TEST_ASSERT_TRUE(cpu.lr_valid);
    TEST_ASSERT_EQUAL_HEX32(PMP_DATA, cpu.lr_addr);
    lrsc_exec(&cpu, &mem, SC_W, PMP_DATA, 0x1234);
    TEST_ASSERT_EQUAL_HEX32(0, cpu_read_reg(&cpu, 1));
    mem_free(&mem);
}

/* The manual allows xRET to clear the reservation but does not require it. We do not. */
void test_the_reservation_survives_a_trap_and_mret(void) {
    CPU cpu; Memory mem;
    lrsc_rig(&cpu, &mem);
    lrsc_exec(&cpu, &mem, LR_W, PMP_DATA, 0);
    lrsc_exec(&cpu, &mem, 0x00000073, 0, 0);                      /* ECALL: trap to mtvec */
    TEST_ASSERT_EQUAL_UINT32(0x3000, cpu.pc);
    TEST_ASSERT_TRUE(cpu.lr_valid);
    cpu_write_reg(&cpu, 2, PMP_DATA);
    cpu_write_reg(&cpu, 3, 0x1234);
    DecodedInstruction mret = decode_instruction(0x30200073);
    execute(&cpu, &mem, &mret);
    TEST_ASSERT_TRUE(cpu.lr_valid);
    cpu.pc = 0x40;
    DecodedInstruction sc = decode_instruction(SC_W);
    execute(&cpu, &mem, &sc);
    TEST_ASSERT_EQUAL_HEX32(0, cpu_read_reg(&cpu, 1));
    mem_free(&mem);
}

void test_lr_sc_retire_and_a_faulting_one_does_not(void) {
    CPU cpu; Memory mem;
    lrsc_rig(&cpu, &mem);
    mem_write32(&mem, 0x40, LR_W);
    cpu_write_reg(&cpu, 2, PMP_DATA);
    cpu.pc = 0x40;
    cpu_step(&cpu, &mem);
    TEST_ASSERT_EQUAL_UINT64(1, cpu.minstret);

    lrsc_rig(&cpu, &mem);
    mem_write32(&mem, 0x40, SC_W);
    cpu_write_reg(&cpu, 2, PMP_DATA + 1);                         /* misaligned */
    cpu.pc = 0x40;
    cpu_step(&cpu, &mem);
    TEST_ASSERT_EQUAL_UINT64(0, cpu.minstret);
    mem_free(&mem);
}

/* ---- Instruction length ------------------------------------------------------------------- */

/* Compressed instructions arrive in step 3 with length 2, so check now that every instruction
 * (x1 = 1, x2 = 0x1000, x3 = 3 and the branches all not taken) advances the pc, and links, by the instruction's length rather than a fixed 4. */
static void length_run(CPU *cpu, Memory *mem, uint32_t raw, uint8_t length) {
    cpu_reset(cpu);
    mem_init(mem, 0x4000);
    cpu_write_csr(cpu, CSR_MCAUSE, 0x77);
    cpu_write_reg(cpu, 1, 1);
    cpu_write_reg(cpu, 2, 0x1000);
    cpu_write_reg(cpu, 3, 3);
    cpu->pc = 0x40;
    DecodedInstruction instr = decode_instruction(raw);
    instr.length = length;
    execute(cpu, mem, &instr);
}

void test_every_kind_of_instruction_advances_by_its_length(void) {
    const struct { const char *name; uint32_t raw; } v[] = {
        {"ADDI", 0x00518213},
        {"SLTI", 0x0051A213},
        {"SLTIU", 0x0051B213},
        {"XORI", 0x0051C213},
        {"ORI", 0x0051E213},
        {"ANDI", 0x0051F213},
        {"SLLI", 0x00219213},
        {"SRLI", 0x0011D213},
        {"SRAI", 0x4011D213},
        {"ADD", 0x00308233},
        {"SUB", 0x40308233},
        {"SLL", 0x00309233},
        {"SLT", 0x0030A233},
        {"SLTU", 0x0030B233},
        {"XOR", 0x0030C233},
        {"SRL", 0x0030D233},
        {"SRA", 0x4030D233},
        {"OR", 0x0030E233},
        {"AND", 0x0030F233},
        {"MUL", 0x02308233},
        {"MULH", 0x02309233},
        {"MULHSU", 0x0230A233},
        {"MULHU", 0x0230B233},
        {"DIV", 0x0230C233},
        {"DIVU", 0x0230D233},
        {"REM", 0x0230E233},
        {"REMU", 0x0230F233},
        {"LB", 0x00010203},
        {"LH", 0x00011203},
        {"LW", 0x00012203},
        {"LBU", 0x00014203},
        {"LHU", 0x00015203},
        {"SB", 0x00310023},
        {"SH", 0x00311023},
        {"SW", 0x00312023},
        {"BEQ not taken", 0x00208463},
        {"BNE not taken", 0x00109463},
        {"BLT not taken", 0x00114463},
        {"BGE not taken", 0x0020D463},
        {"BLTU not taken", 0x00116463},
        {"BGEU not taken", 0x0020F463},
        {"LUI", 0x000010B7},
        {"AUIPC", 0x00001097},
        {"FENCE", 0x0FF0000F},
        {"FENCE.I", 0x0000100F},
        {"CSRRW", 0x34019273},
        {"CSRRS", 0x3401A273},
        {"CSRRC", 0x3401B273},
        {"CSRRWI", 0x3401D273},
        {"CSRRSI", 0x3401E273},
        {"CSRRCI", 0x3401F273},
        {"WFI", 0x10500073},
        {"SFENCE.VMA", 0x12000073},
        {"AMOADD.W", 0x0031222F},
        {"AMOSWAP.W", 0x0831222F},
        {"LR.W", 0x1001222F},
        {"SC.W", 0x1831222F},
        {"AMOXOR.W", 0x2031222F},
        {"AMOOR.W", 0x4031222F},
        {"AMOAND.W", 0x6031222F},
        {"AMOMIN.W", 0x8031222F},
        {"AMOMAX.W", 0xA031222F},
        {"AMOMINU.W", 0xC031222F},
        {"AMOMAXU.W", 0xE031222F},
    };
    const uint8_t lengths[] = {2, 4};
    for (unsigned i = 0; i < sizeof v / sizeof v[0]; i++) {
        for (unsigned l = 0; l < 2; l++) {
            CPU cpu; Memory mem;
            length_run(&cpu, &mem, v[i].raw, lengths[l]);
            TEST_ASSERT_EQUAL_HEX32_MESSAGE(0x77, cpu_read_csr(&cpu, CSR_MCAUSE), v[i].name);   /* no trap */
            TEST_ASSERT_EQUAL_HEX32_MESSAGE(0x40 + lengths[l], cpu.pc, v[i].name);
            mem_free(&mem);
        }
    }
}

void test_jal_and_jalr_link_the_address_after_the_instruction(void) {
    CPU cpu; Memory mem;
    length_run(&cpu, &mem, 0x008000EF, 2);                        /* JAL x1, +8 */
    TEST_ASSERT_EQUAL_HEX32(0x42, cpu_read_reg(&cpu, 1));
    TEST_ASSERT_EQUAL_HEX32(0x48, cpu.pc);
    mem_free(&mem);

    length_run(&cpu, &mem, 0x008000EF, 4);
    TEST_ASSERT_EQUAL_HEX32(0x44, cpu_read_reg(&cpu, 1));
    mem_free(&mem);

    length_run(&cpu, &mem, 0x000100E7, 2);                        /* JALR x1, 0(x2) with x2 = 0x1000 */
    TEST_ASSERT_EQUAL_HEX32(0x42, cpu_read_reg(&cpu, 1));
    TEST_ASSERT_EQUAL_HEX32(0x1000, cpu.pc);
    mem_free(&mem);

    length_run(&cpu, &mem, 0x000100E7, 4);
    TEST_ASSERT_EQUAL_HEX32(0x44, cpu_read_reg(&cpu, 1));
    mem_free(&mem);
}

/* ---- Compressed instructions through cpu_step ---------------------------------------------- */

/* Encodings from riscv64-unknown-elf-as (-march=rv32imac). */
#define C_NOP        0x0001u
#define C_LI_A0_3    0x450Du   /* c.li x10, 3 */
#define C_ADDI_A0_5  0x0515u   /* c.addi x10, 5 */
#define C_ADD_A0_A1  0x952Eu   /* c.add x10, x11 */
#define C_JAL_8      0x2021u   /* c.jal .+8 */
#define C_JALR_T0    0x9282u   /* c.jalr x5 */
#define C_EBREAK     0x9002u
#define ADDI_A1_A0_4 0x00450593u /* addi x11, x10, 4 */

void test_cpu_step_executes_a_compressed_instruction(void) {
    CPU cpu; Memory mem;
    pmp_rig(&cpu, &mem, PRV_M);
    mem_write16(&mem, 0, C_ADDI_A0_5);
    cpu.pc = 0;
    cpu_step(&cpu, &mem);
    TEST_ASSERT_EQUAL_UINT32(5, cpu_read_reg(&cpu, 10));
    TEST_ASSERT_EQUAL_UINT32(2, cpu.pc);
    TEST_ASSERT_EQUAL_UINT64(1, cpu.minstret);
    TEST_ASSERT_EQUAL_UINT32(0x77, cpu_read_csr(&cpu, CSR_MCAUSE));
    mem_free(&mem);
}

void test_cpu_step_runs_mixed_16_and_32_bit_instructions(void) {
    CPU cpu; Memory mem;
    pmp_rig(&cpu, &mem, PRV_M);
    mem_write16(&mem, 0, C_LI_A0_3);          /* x10 = 3 */
    mem_write32(&mem, 2, ADDI_A1_A0_4);       /* x11 = 7: a 32-bit instruction at a 2-aligned address */
    mem_write16(&mem, 6, C_ADD_A0_A1);        /* x10 = 10 */
    cpu.pc = 0;
    cpu_step(&cpu, &mem);
    TEST_ASSERT_EQUAL_UINT32(2, cpu.pc);
    cpu_step(&cpu, &mem);
    TEST_ASSERT_EQUAL_UINT32(6, cpu.pc);
    cpu_step(&cpu, &mem);
    TEST_ASSERT_EQUAL_UINT32(8, cpu.pc);
    TEST_ASSERT_EQUAL_UINT32(10, cpu_read_reg(&cpu, 10));
    TEST_ASSERT_EQUAL_UINT32(7, cpu_read_reg(&cpu, 11));
    TEST_ASSERT_EQUAL_UINT64(3, cpu.minstret);
    TEST_ASSERT_EQUAL_UINT32(0x77, cpu_read_csr(&cpu, CSR_MCAUSE));
    mem_free(&mem);
}

void test_cpu_step_fetches_a_32_bit_instruction_across_a_4k_boundary(void) {
    CPU cpu; Memory mem;
    pmp_rig(&cpu, &mem, PRV_M);
    mem_free(&mem);
    mem_init(&mem, 0x2000);
    cpu_write_csr(&cpu, CSR_MCAUSE, 0x77);
    mem_write32(&mem, 0xFFE, ADDI_X6_7);
    cpu.pc = 0xFFE;
    cpu_step(&cpu, &mem);
    TEST_ASSERT_EQUAL_UINT32(7, cpu_read_reg(&cpu, 6));
    TEST_ASSERT_EQUAL_UINT32(0x1002, cpu.pc);
    TEST_ASSERT_EQUAL_UINT32(0x77, cpu_read_csr(&cpu, CSR_MCAUSE));
    mem_free(&mem);
}

void test_c_jal_and_c_jalr_link_pc_plus_2(void) {
    CPU cpu; Memory mem;
    pmp_rig(&cpu, &mem, PRV_M);
    mem_write16(&mem, 0x100, C_JAL_8);
    cpu.pc = 0x100;
    cpu_step(&cpu, &mem);
    TEST_ASSERT_EQUAL_UINT32(0x102, cpu_read_reg(&cpu, 1));
    TEST_ASSERT_EQUAL_UINT32(0x108, cpu.pc);
    mem_free(&mem);

    pmp_rig(&cpu, &mem, PRV_M);
    mem_write16(&mem, 0x100, C_JALR_T0);
    cpu_write_reg(&cpu, 5, 0x200);
    cpu.pc = 0x100;
    cpu_step(&cpu, &mem);
    TEST_ASSERT_EQUAL_UINT32(0x102, cpu_read_reg(&cpu, 1));
    TEST_ASSERT_EQUAL_UINT32(0x200, cpu.pc);
    mem_free(&mem);
}

void test_c_ebreak_traps_with_mepc_at_a_2_aligned_pc(void) {
    CPU cpu; Memory mem;
    pmp_rig(&cpu, &mem, PRV_M);
    mem_write16(&mem, 0x102, C_EBREAK);
    cpu.pc = 0x102;
    cpu_step(&cpu, &mem);
    pmp_assert_fault(&cpu, 3, 0, 0x102, PRV_M, "c.ebreak");
    TEST_ASSERT_EQUAL_UINT64(0, cpu.minstret);
    mem_free(&mem);
}

void test_illegal_compressed_instruction_reports_its_16_bits(void) {
    const struct { const char *name; uint16_t half; uint32_t pc; } v[] = {
        {"all zero",            0x0000, 0x100},
        {"C.ADDI16SP, nzimm 0", 0x6101, 0x102},
        {"C.FLD (no D)",        0x2000, 0x106},
        {"C.SLLI, shamt 32",    0x1502, 0x1FE},
    };
    for (unsigned i = 0; i < sizeof v / sizeof v[0]; i++) {
        CPU cpu; Memory mem;
        pmp_rig(&cpu, &mem, PRV_M);
        mem_write16(&mem, v[i].pc, v[i].half);
        cpu_write_reg(&cpu, 1, 0x11111111);
        cpu.pc = v[i].pc;
        cpu_step(&cpu, &mem);
        pmp_assert_fault(&cpu, 2, v[i].half, v[i].pc, PRV_M, v[i].name);
        TEST_ASSERT_EQUAL_HEX32_MESSAGE(0x11111111, cpu_read_reg(&cpu, 1), v[i].name);
        TEST_ASSERT_EQUAL_UINT64_MESSAGE(0, cpu.minstret, v[i].name);
        mem_free(&mem);
    }
}

/* PMP checks each 2-byte fetch on its own. Region: X only, [0x1000, 0x2000), in U-mode. */
void test_pmp_fetch_is_checked_per_halfword(void) {
    const struct { const char *name; uint32_t pc; uint32_t raw; uint32_t tval; bool ok; uint32_t next_pc; } v[] = {
        {"32-bit, inside",                    0x1002, ADDI_X6_7,    0,      true,  0x1006},
        {"32-bit, second half outside",       0x1FFE, ADDI_X6_7,    0x2000, false, 0},
        {"32-bit, first half outside",        0x0FFE, ADDI_X6_7,    0x0FFE, false, 0},
        {"compressed, last halfword inside",  0x1FFE, C_LI_A0_3,    0,      true,  0x2000},
        {"compressed, just outside",          0x2000, C_LI_A0_3,    0x2000, false, 0},
    };
    for (unsigned i = 0; i < sizeof v / sizeof v[0]; i++) {
        CPU cpu; Memory mem;
        pmp_rig(&cpu, &mem, PRV_U);
        pmp_data_region(&cpu, 0, 0x1C);
        if (v[i].raw > 0xFFFF) {
            mem_write32(&mem, v[i].pc, v[i].raw);
        } else {
            mem_write16(&mem, v[i].pc, (uint16_t)v[i].raw);
        }
        cpu.pc = v[i].pc;
        cpu_step(&cpu, &mem);
        if (v[i].ok) {
            TEST_ASSERT_EQUAL_UINT32_MESSAGE(v[i].next_pc, cpu.pc, v[i].name);
            TEST_ASSERT_EQUAL_UINT32_MESSAGE(0x77, cpu_read_csr(&cpu, CSR_MCAUSE), v[i].name);
            TEST_ASSERT_EQUAL_UINT64_MESSAGE(1, cpu.minstret, v[i].name);
        } else {
            pmp_assert_fault(&cpu, 1, v[i].tval, v[i].pc, PRV_U, v[i].name);
            TEST_ASSERT_EQUAL_UINT64_MESSAGE(0, cpu.minstret, v[i].name);
        }
        mem_free(&mem);
    }
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

void test_csrr_misa_reports_the_implemented_extensions(void) {
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
static void assert_fetch_fault_with_tval(Memory *mem, uint32_t start_pc, uint32_t tval) {
    CPU cpu; cpu_reset(&cpu);
    cpu_write_csr(&cpu, CSR_MTVEC, 0x1000);
    cpu_write_reg(&cpu, 1, 0xDEADBEEF);
    cpu.pc = start_pc;

    cpu_step(&cpu, mem);

    TEST_ASSERT_EQUAL_UINT32(0x1000, cpu.pc);
    TEST_ASSERT_EQUAL_UINT32(start_pc, cpu_read_csr(&cpu, CSR_MEPC));       /* the start of the instruction */
    TEST_ASSERT_EQUAL_UINT32(1, cpu_read_csr(&cpu, CSR_MCAUSE));
    TEST_ASSERT_EQUAL_UINT32(tval, cpu_read_csr(&cpu, CSR_MTVAL));          /* the part that faulted */
    TEST_ASSERT_EQUAL_UINT32(0xDEADBEEF, cpu_read_reg(&cpu, 1));
    TEST_ASSERT_EQUAL_UINT64(0, cpu.minstret);
}

static void assert_fetch_fault(Memory *mem, uint32_t start_pc) {
    assert_fetch_fault_with_tval(mem, start_pc, start_pc);
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

void test_cpu_step_fetch_second_half_past_the_end_faults(void) {
    Memory mem;
    mem_init(&mem, 14);                // the 32-bit instruction at 12 has its second half at 14, past the end
    mem_write16(&mem, 12, 0x0013);     // bits [1:0] = 11: a 32-bit instruction
    assert_fetch_fault_with_tval(&mem, 12, 14);   // mepc = the start, mtval = the half that faulted
    mem_free(&mem);

    mem_init(&mem, 16);
    mem_write16(&mem, 14, 0x0013);     // at a 2-aligned pc: the second half is at 16
    assert_fetch_fault_with_tval(&mem, 14, 16);
    mem_free(&mem);
}

void test_cpu_step_compressed_instruction_fits_in_the_last_halfword(void) {
    CPU cpu; cpu_reset(&cpu);
    Memory mem;
    mem_init(&mem, 14);
    cpu_write_csr(&cpu, CSR_MCAUSE, 0x77);
    mem_write16(&mem, 12, 0x0001);     // C.NOP
    cpu.pc = 12;
    cpu_step(&cpu, &mem);
    TEST_ASSERT_EQUAL_UINT32(14, cpu.pc);
    TEST_ASSERT_EQUAL_UINT32(0x77, cpu_read_csr(&cpu, CSR_MCAUSE));
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
    RUN_TEST(test_csrrw_sepc_drops_bit_zero);
    RUN_TEST(test_trap_routing);
    RUN_TEST(test_delegated_trap_writes_s_state_and_leaves_m_state);
    RUN_TEST(test_delegated_trap_spp_and_interrupt_enable_stack);
    RUN_TEST(test_delegated_trap_stval_holds_the_instruction_or_address);
    RUN_TEST(test_delegated_trap_does_not_retire);
    RUN_TEST(test_delegation_csrs_through_instructions);
    RUN_TEST(test_sret_table);
    RUN_TEST(test_sret_returns_to_sepc_without_its_bit_zero);
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
    RUN_TEST(test_pmp_csrs_exist_and_are_m_only);
    RUN_TEST(test_csrs_next_to_the_pmp_range_do_not_exist);
    RUN_TEST(test_pmp_harness_sequence_is_accepted);
    RUN_TEST(test_pmpcfg_read_modify_write_applies_the_warl_rule);
    RUN_TEST(test_pmp_locking_through_instructions);
    RUN_TEST(test_pmpaddr_all_ones_reads_back);
    RUN_TEST(test_pmp_enforced_on_loads_and_stores_from_s_and_u);
    RUN_TEST(test_pmp_allowed_accesses_do_their_work);
    RUN_TEST(test_pmp_no_entries_denies_u_and_s_but_the_helper_allows);
    RUN_TEST(test_pmp_enforced_on_fetch);
    RUN_TEST(test_pmp_m_mode_and_the_l_bit);
    RUN_TEST(test_pmp_mprv);
    RUN_TEST(test_pmp_fault_delegated_to_s);
    RUN_TEST(test_pmp_harness_entry_allows_u);
    RUN_TEST(test_m_multiplies);
    RUN_TEST(test_m_divisions);
    RUN_TEST(test_m_works_in_every_mode);
    RUN_TEST(test_m_register_aliasing_and_x0);
    RUN_TEST(test_m_instruction_retires);
    RUN_TEST(test_m_reserved_funct7_still_traps);
    RUN_TEST(test_amo_operations);
    RUN_TEST(test_amo_aq_and_rl_are_ignored);
    RUN_TEST(test_amo_with_rd_x0_still_updates_memory);
    RUN_TEST(test_amo_rd_may_alias_a_source);
    RUN_TEST(test_amo_misaligned_address_raises_address_misaligned);
    RUN_TEST(test_amo_outside_memory_is_a_store_access_fault);
    RUN_TEST(test_amo_pmp);
    RUN_TEST(test_amo_pmp_fault_can_be_delegated_to_s);
    RUN_TEST(test_amo_reserved_encodings_are_illegal);
    RUN_TEST(test_amo_retires_and_a_faulting_amo_does_not);
    RUN_TEST(test_lr_loads_the_word_and_registers_a_reservation);
    RUN_TEST(test_lr_with_rd_x0_still_registers_a_reservation);
    RUN_TEST(test_sc_after_lr_to_the_same_address_succeeds);
    RUN_TEST(test_sc_without_a_reservation_fails);
    RUN_TEST(test_sc_to_another_address_fails_and_clears_the_reservation);
    RUN_TEST(test_a_successful_sc_uses_up_the_reservation);
    RUN_TEST(test_a_later_lr_replaces_the_reservation);
    RUN_TEST(test_lr_sc_aq_and_rl_are_ignored);
    RUN_TEST(test_sc_rd_may_be_rs2);
    RUN_TEST(test_lr_sc_misaligned_address_raises_address_misaligned);
    RUN_TEST(test_lr_sc_outside_memory_is_an_access_fault);
    RUN_TEST(test_lr_sc_pmp);
    RUN_TEST(test_lr_with_nonzero_rs2_and_other_funct3_are_illegal);
    RUN_TEST(test_a_faulting_lr_or_sc_keeps_the_reservation);
    RUN_TEST(test_the_reservation_survives_a_trap_and_mret);
    RUN_TEST(test_lr_sc_retire_and_a_faulting_one_does_not);
    RUN_TEST(test_every_kind_of_instruction_advances_by_its_length);
    RUN_TEST(test_jal_and_jalr_link_the_address_after_the_instruction);
    RUN_TEST(test_cpu_step_executes_a_compressed_instruction);
    RUN_TEST(test_cpu_step_runs_mixed_16_and_32_bit_instructions);
    RUN_TEST(test_cpu_step_fetches_a_32_bit_instruction_across_a_4k_boundary);
    RUN_TEST(test_c_jal_and_c_jalr_link_pc_plus_2);
    RUN_TEST(test_c_ebreak_traps_with_mepc_at_a_2_aligned_pc);
    RUN_TEST(test_illegal_compressed_instruction_reports_its_16_bits);
    RUN_TEST(test_pmp_fetch_is_checked_per_halfword);
    RUN_TEST(test_csrr_misa_reports_the_implemented_extensions);
    RUN_TEST(test_csrr_tdata1_reads_zero);
    RUN_TEST(test_tdata1_write_does_not_read_back);
    RUN_TEST(test_cpu_step_executes_fetched_instruction);
    RUN_TEST(test_cpu_step_two_steps_in_sequence);
    RUN_TEST(test_cpu_step_fetches_last_whole_word);
    RUN_TEST(test_cpu_step_based_memory_fetch_works);
    RUN_TEST(test_cpu_step_fetch_past_end_faults);
    RUN_TEST(test_cpu_step_fetch_at_size_faults);
    RUN_TEST(test_cpu_step_fetch_second_half_past_the_end_faults);
    RUN_TEST(test_cpu_step_compressed_instruction_fits_in_the_last_halfword);
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
