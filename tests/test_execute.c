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

    int32_t raw = mem_read32(&mem, cpu.pc);
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

    int32_t raw = mem_read32(&mem, cpu.pc);
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

    int32_t raw = mem_read32(&mem, cpu.pc);
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

    int32_t raw = mem_read32(&mem, cpu.pc);
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

    int32_t raw = mem_read32(&mem, cpu.pc);
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

    int32_t raw = mem_read32(&mem, cpu.pc);
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);

    TEST_ASSERT_EQUAL_UINT32(0, cpu_read_reg(&cpu, 2));
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
	return UNITY_END();
}





