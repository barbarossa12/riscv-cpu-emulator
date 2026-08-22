#include "unity.h"
#include "../src/cpu.h"

void setUp(void) {} // Unity calles this before each test function
void tearDown(void) {} // Unity calls this after eacht test function

void test_cpu_write_read_roundtrip_x5(void) {
	CPU test_cpu;
	cpu_write_reg(&test_cpu, 5, 0x64);
	uint32_t val = cpu_read_reg(&test_cpu, 5);
	TEST_ASSERT_EQUAL_UINT32(val, 0x64);
}

void test_cpu_write_read_roundtrip_x31(void) {
	CPU test_cpu;
	cpu_write_reg(&test_cpu, 31, 0xDEADBEEF);
	uint32_t val = cpu_read_reg(&test_cpu, 31);
	TEST_ASSERT_EQUAL_UINT32(val, 0xDEADBEEF);
}

int main(void) {
	UNITY_BEGIN();
	RUN_TEST(test_cpu_write_read_roundtrip_x5);
	RUN_TEST(test_cpu_write_read_roundtrip_x31);
	return UNITY_END();
}

