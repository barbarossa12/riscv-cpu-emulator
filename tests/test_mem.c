#include "unity.h"
#include "mem.h"

void setUp(void) {} // Unity calles this before each test function
void tearDown(void) {} // Unity calls this after eacht test function


void test_write_then_read_8bit_roundtrip(void) {
    Memory test_mem;
    mem_init(&test_mem, 16);
    mem_write8(&test_mem, 0, 0x42);
    uint8_t result = mem_read8(&test_mem, 0);
    TEST_ASSERT_EQUAL_UINT8(0x42, result);
    mem_free(&test_mem);
}

void test_mem_write32_byte_order(void) {
    Memory test_mem;
    mem_init(&test_mem, 128);
    mem_write32(&test_mem, 0, (uint32_t)0xAABBCCDD);
    uint32_t result = mem_read32(&test_mem, 0);

    TEST_ASSERT_EQUAL_UINT32((uint32_t)0xAABBCCDD, result);
    TEST_ASSERT_EQUAL_UINT8(0xDD, test_mem.buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0xCC, test_mem.buffer[1]);
    TEST_ASSERT_EQUAL_UINT8(0xBB, test_mem.buffer[2]);
    TEST_ASSERT_EQUAL_UINT8(0xAA, test_mem.buffer[3]);
}

int main(void) {
	UNITY_BEGIN();
	RUN_TEST(test_write_then_read_8bit_roundtrip);
	RUN_TEST(test_mem_write32_byte_order);
	return UNITY_END();
}
