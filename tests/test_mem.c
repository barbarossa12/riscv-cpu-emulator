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
    mem_write32(&test_mem, 0, 0xAABBCCDD);
    uint32_t result = mem_read32(&test_mem, 0);

    TEST_ASSERT_EQUAL_UINT32(0xAABBCCDD, result);
    TEST_ASSERT_EQUAL_UINT8(0xDD, test_mem.buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0xCC, test_mem.buffer[1]);
    TEST_ASSERT_EQUAL_UINT8(0xBB, test_mem.buffer[2]);
    TEST_ASSERT_EQUAL_UINT8(0xAA, test_mem.buffer[3]);
    mem_free(&test_mem);
}


void test_mem_write16_byte_order(void) {
    Memory test_mem;
    mem_init(&test_mem, 128);
    mem_write16(&test_mem, 0, 0xAABB);
    uint16_t result = mem_read16(&test_mem, 0);

    // testing the readback
    TEST_ASSERT_EQUAL_UINT16(0xAABB, result);

    // testing the byte ordering
    TEST_ASSERT_EQUAL_UINT8(0xBB, test_mem.buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0xAA, test_mem.buffer[1]);
    mem_free(&test_mem);
}

void test_mem_boundary_last_valid_address(void) {

    // test for 8 bit read and write
    Memory test_mem8;
    mem_init(&test_mem8, 16);

    mem_write8(&test_mem8, 15, 0xAA);
    uint8_t result_8bit = mem_read8(&test_mem8, 15);

    //test the readback value
    TEST_ASSERT_EQUAL_UINT8(0xAA, result_8bit);

    // test for 16 bit read and write
    Memory test_mem16;
    mem_init(&test_mem16, 16);

    mem_write16(&test_mem16, 14, 0xAABB);
    uint16_t result_16bit = mem_read16(&test_mem16, 14);
    TEST_ASSERT_EQUAL_UINT16(0xAABB, result_16bit);

    // test for the 32bit read and write
    Memory test_mem32;
    mem_init(&test_mem32, 16);

    mem_write32(&test_mem32, 12, 0xAABBCCDD);
    uint32_t result_32bit = mem_read32(&test_mem32, 12);
    TEST_ASSERT_EQUAL_UINT32(0xAABBCCDD, result_32bit);


    mem_free(&test_mem8);
    mem_free(&test_mem16);
    mem_free(&test_mem32);

}


int main(void) {
	UNITY_BEGIN();
	RUN_TEST(test_write_then_read_8bit_roundtrip);
	RUN_TEST(test_mem_write32_byte_order);
    RUN_TEST(test_mem_write16_byte_order);
    RUN_TEST(test_mem_boundary_last_valid_address);

	return UNITY_END();
}
