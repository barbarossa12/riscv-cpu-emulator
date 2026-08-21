#include "unity.h"
#include "mem.h"

void setUp() {} // Unity calles this before each test function
void tearDown() {} // Unity calls this after eacht test function


void test_write_then_read_8bit_roundtrip(void) {
    Memory mem;
    mem_init(&mem, 16);
    mem_write8(&mem, 0, 0x42);
    uint8_t result = mem_read8(&mem, 0);
    TEST_ASSERT_EQUAL_UINT8(0x42, result);
    mem_free(&mem);
}

int main() {
	UNITY_BEGIN();
	RUN_TEST(test_write_then_read_8bit_roundtrip);
	return UNITY_END();
}
