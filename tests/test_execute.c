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
	return UNITY_END();
}
