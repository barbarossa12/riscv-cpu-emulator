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


void test_mem_try_read8_write8_roundtrip(void) {
    Memory test_mem;
    mem_init(&test_mem, 16);

    TEST_ASSERT_TRUE(mem_try_write8(&test_mem, 0, 0x42));
    uint8_t out = 0;
    TEST_ASSERT_TRUE(mem_try_read8(&test_mem, 0, &out));
    TEST_ASSERT_EQUAL_UINT8(0x42, out);

    mem_free(&test_mem);
}

void test_mem_try_boundary_last_valid(void) {
    Memory test_mem8;
    mem_init(&test_mem8, 16);
    TEST_ASSERT_TRUE(mem_try_write8(&test_mem8, 15, 0xAA));
    uint8_t out8 = 0;
    TEST_ASSERT_TRUE(mem_try_read8(&test_mem8, 15, &out8));
    TEST_ASSERT_EQUAL_UINT8(0xAA, out8);
    mem_free(&test_mem8);

    Memory test_mem16;
    mem_init(&test_mem16, 16);
    TEST_ASSERT_TRUE(mem_try_write16(&test_mem16, 14, 0xAABB));
    uint16_t out16 = 0;
    TEST_ASSERT_TRUE(mem_try_read16(&test_mem16, 14, &out16));
    TEST_ASSERT_EQUAL_UINT16(0xAABB, out16);
    mem_free(&test_mem16);

    Memory test_mem32;
    mem_init(&test_mem32, 16);
    TEST_ASSERT_TRUE(mem_try_write32(&test_mem32, 12, 0xAABBCCDD));
    uint32_t out32 = 0;
    TEST_ASSERT_TRUE(mem_try_read32(&test_mem32, 12, &out32));
    TEST_ASSERT_EQUAL_UINT32(0xAABBCCDD, out32);
    mem_free(&test_mem32);
}

void test_mem_try_read8_oob(void) {
    Memory test_mem;
    mem_init(&test_mem, 16);

    uint8_t out = 0xAA; // sentinel
    TEST_ASSERT_FALSE(mem_try_read8(&test_mem, 16, &out));
    TEST_ASSERT_EQUAL_UINT8(0xAA, out); // untouched on failure

    mem_free(&test_mem);
}

void test_mem_try_read16_oob(void) {
    Memory test_mem;
    mem_init(&test_mem, 16);

    uint16_t out = 0xAAAA; // sentinel
    TEST_ASSERT_FALSE(mem_try_read16(&test_mem, 15, &out)); // straddles the boundary
    TEST_ASSERT_EQUAL_UINT16(0xAAAA, out);

    mem_free(&test_mem);
}

void test_mem_try_read32_oob(void) {
    Memory test_mem;
    mem_init(&test_mem, 16);

    uint32_t out = 0xAAAAAAAA; // sentinel
    TEST_ASSERT_FALSE(mem_try_read32(&test_mem, 13, &out)); // straddles the boundary
    TEST_ASSERT_EQUAL_UINT32(0xAAAAAAAA, out);

    mem_free(&test_mem);
}

void test_mem_try_write8_oob(void) {
    Memory test_mem;
    mem_init(&test_mem, 16);

    TEST_ASSERT_FALSE(mem_try_write8(&test_mem, 16, 0x99));

    mem_free(&test_mem);
}

void test_mem_try_write16_oob(void) {
    Memory test_mem;
    mem_init(&test_mem, 16);

    TEST_ASSERT_FALSE(mem_try_write16(&test_mem, 15, 0x99)); // straddles the boundary

    mem_free(&test_mem);
}

void test_mem_try_write32_oob(void) {
    Memory test_mem;
    mem_init(&test_mem, 16);

    TEST_ASSERT_FALSE(mem_try_write32(&test_mem, 13, 0x99)); // straddles the boundary

    mem_free(&test_mem);
}

/* addr + n wraps around 2^32 back into the buffer: must still be rejected */
void test_mem_try_read16_wraparound(void) {
    Memory test_mem;
    mem_init(&test_mem, 16);

    uint16_t out = 0xAAAA; // sentinel
    TEST_ASSERT_FALSE(mem_try_read16(&test_mem, 0xFFFFFFFE, &out)); // addr + 2 wraps to 0
    TEST_ASSERT_FALSE(mem_try_read16(&test_mem, 0xFFFFFFFF, &out)); // addr + 2 wraps to 1
    TEST_ASSERT_EQUAL_UINT16(0xAAAA, out);

    mem_free(&test_mem);
}

void test_mem_try_read32_wraparound(void) {
    Memory test_mem;
    mem_init(&test_mem, 16);

    uint32_t out = 0xAAAAAAAA; // sentinel
    TEST_ASSERT_FALSE(mem_try_read32(&test_mem, 0xFFFFFFFC, &out)); // addr + 4 wraps to 0
    TEST_ASSERT_FALSE(mem_try_read32(&test_mem, 0xFFFFFFFD, &out)); // wraps to 1
    TEST_ASSERT_FALSE(mem_try_read32(&test_mem, 0xFFFFFFFE, &out)); // wraps to 2
    TEST_ASSERT_FALSE(mem_try_read32(&test_mem, 0xFFFFFFFF, &out)); // wraps to 3
    TEST_ASSERT_EQUAL_UINT32(0xAAAAAAAA, out);

    mem_free(&test_mem);
}

void test_mem_try_write16_wraparound(void) {
    Memory test_mem;
    mem_init(&test_mem, 16);
    mem_write16(&test_mem, 0, 0x1234);

    TEST_ASSERT_FALSE(mem_try_write16(&test_mem, 0xFFFFFFFE, 0x99)); // addr + 2 wraps to 0
    TEST_ASSERT_FALSE(mem_try_write16(&test_mem, 0xFFFFFFFF, 0x99)); // wraps to 1
    TEST_ASSERT_EQUAL_UINT16(0x1234, mem_read16(&test_mem, 0)); // nothing written at the bottom

    mem_free(&test_mem);
}

void test_mem_try_write32_wraparound(void) {
    Memory test_mem;
    mem_init(&test_mem, 16);
    mem_write32(&test_mem, 0, 0x12345678);

    TEST_ASSERT_FALSE(mem_try_write32(&test_mem, 0xFFFFFFFC, 0x99)); // addr + 4 wraps to 0
    TEST_ASSERT_FALSE(mem_try_write32(&test_mem, 0xFFFFFFFD, 0x99)); // wraps to 1
    TEST_ASSERT_FALSE(mem_try_write32(&test_mem, 0xFFFFFFFE, 0x99)); // wraps to 2
    TEST_ASSERT_FALSE(mem_try_write32(&test_mem, 0xFFFFFFFF, 0x99)); // wraps to 3
    TEST_ASSERT_EQUAL_UINT32(0x12345678, mem_read32(&test_mem, 0)); // nothing written at the bottom

    mem_free(&test_mem);
}

void test_mem_try_8bit_top_of_address_space(void) {
    Memory test_mem;
    mem_init(&test_mem, 16);

    uint8_t out = 0xAA; // sentinel
    TEST_ASSERT_FALSE(mem_try_read8(&test_mem, 0xFFFFFFFF, &out));
    TEST_ASSERT_EQUAL_UINT8(0xAA, out);
    TEST_ASSERT_FALSE(mem_try_write8(&test_mem, 0xFFFFFFFF, 0x99));

    mem_free(&test_mem);
}

int main(void) {
	UNITY_BEGIN();
	RUN_TEST(test_write_then_read_8bit_roundtrip);
	RUN_TEST(test_mem_write32_byte_order);
    RUN_TEST(test_mem_write16_byte_order);
    RUN_TEST(test_mem_boundary_last_valid_address);
    RUN_TEST(test_mem_try_read8_write8_roundtrip);
    RUN_TEST(test_mem_try_boundary_last_valid);
    RUN_TEST(test_mem_try_read8_oob);
    RUN_TEST(test_mem_try_read16_oob);
    RUN_TEST(test_mem_try_read32_oob);
    RUN_TEST(test_mem_try_write8_oob);
    RUN_TEST(test_mem_try_write16_oob);
    RUN_TEST(test_mem_try_write32_oob);
    RUN_TEST(test_mem_try_read16_wraparound);
    RUN_TEST(test_mem_try_read32_wraparound);
    RUN_TEST(test_mem_try_write16_wraparound);
    RUN_TEST(test_mem_try_write32_wraparound);
    RUN_TEST(test_mem_try_8bit_top_of_address_space);

	return UNITY_END();
}
