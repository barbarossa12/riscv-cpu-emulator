#include "unity.h"
#include "../src/cpu.h"

void setUp(void) {} // Unity calles this before each test function
void tearDown(void) {} // Unity calls this after eacht test function

void test_cpu_write_read_roundtrip_x5(void) {
	CPU test_cpu;
	cpu_write_reg(&test_cpu, 5, 0x64);
	uint32_t val = cpu_read_reg(&test_cpu, 5);
	TEST_ASSERT_EQUAL_UINT32(0x64, val);
}

void test_cpu_write_read_roundtrip_x31(void) {
	CPU test_cpu;
	cpu_write_reg(&test_cpu, 31, 0xDEADBEEF);
	uint32_t val = cpu_read_reg(&test_cpu, 31);
	TEST_ASSERT_EQUAL_UINT32(0xDEADBEEF, val);
}

void test_cpu_x0_read_ignores_underlying_value(void) {
	CPU test_cpu;
	test_cpu.regs[0] = 0xDEADBEEF;
	uint32_t val = cpu_read_reg(&test_cpu, 0);
	TEST_ASSERT_EQUAL_UINT32(0, val);
}

void test_cpu_x0_write_is_discarded(void) {
	CPU test_cpu = {0};
	cpu_write_reg(&test_cpu, 0, 0x12345);
	uint32_t val = cpu_read_reg(&test_cpu, 0);
	TEST_ASSERT_EQUAL_UINT32(0, val);
}

void test_cpu_write_to_x0_does_not_affect_other_registers(void) {
	CPU test_cpu = {0};
	cpu_write_reg(&test_cpu, 1, 0x42);
	cpu_write_reg(&test_cpu, 0, 0xDEADBEEF);
	uint32_t val = cpu_read_reg(&test_cpu, 1);
	TEST_ASSERT_EQUAL_UINT32(0x42, val);
}

int main(void) {
	UNITY_BEGIN();
	RUN_TEST(test_cpu_write_read_roundtrip_x5);
	RUN_TEST(test_cpu_write_read_roundtrip_x31);
	RUN_TEST(test_cpu_x0_read_ignores_underlying_value);
	RUN_TEST(test_cpu_x0_write_is_discarded);
	RUN_TEST(test_cpu_write_to_x0_does_not_affect_other_registers);
	return UNITY_END();
}

