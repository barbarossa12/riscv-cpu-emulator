#include "unity.h"
#include "../src/plic.h"

#define RAM_BASE 0x80000000u

static Plic plic;
static Memory mem;

void setUp(void) {
    mem_init_at(&mem, RAM_BASE, 0x1000);
    plic_reset(&plic);
    mem_add_device(&mem, plic_device(&plic));
}

void tearDown(void) {
    mem_free(&mem);
}

static bool rd(uint32_t offset, uint32_t *v) {
    return mem_bus_read(&mem, PLIC_BASE + offset, 4, v);
}

static bool wr(uint32_t offset, uint32_t v) {
    return mem_bus_write(&mem, PLIC_BASE + offset, 4, v);
}

static uint32_t reg(uint32_t offset) {
    uint32_t v = 0xDEADBEEF;
    TEST_ASSERT_TRUE(rd(offset, &v));
    return v;
}

static void set(uint32_t offset, uint32_t v) {
    TEST_ASSERT_TRUE(wr(offset, v));
}

static void assert_faults(uint32_t offset) {
    uint32_t v = 0;
    TEST_ASSERT_FALSE_MESSAGE(rd(offset, &v), "read should fault");
    TEST_ASSERT_FALSE_MESSAGE(wr(offset, 1), "write should fault");
}

// Reset
void test_reset_everything_reads_zero(void) {
    for (uint32_t s = 1; s <= PLIC_NSOURCES; s++) TEST_ASSERT_EQUAL_HEX32(0, reg(4 * s));
    TEST_ASSERT_EQUAL_HEX32(0, reg(PLIC_PENDING));
    for (uint32_t c = 0; c < PLIC_NCONTEXTS; c++) {
        TEST_ASSERT_EQUAL_HEX32(0, reg(PLIC_ENABLE + 0x80 * c));
        TEST_ASSERT_EQUAL_HEX32(0, reg(PLIC_CONTEXT + 0x1000 * c));
        TEST_ASSERT_EQUAL_HEX32(0, reg(PLIC_CONTEXT + 0x1000 * c + 4));
    }
}

void test_reset_clears_written_state(void) {
    set(4, 5);
    set(PLIC_ENABLE, 0xFFFFFFFF);
    set(PLIC_CONTEXT + 0x1000, 3);
    plic_reset(&plic);
    TEST_ASSERT_EQUAL_HEX32(0, reg(4));
    TEST_ASSERT_EQUAL_HEX32(0, reg(PLIC_ENABLE));
    TEST_ASSERT_EQUAL_HEX32(0, reg(PLIC_CONTEXT + 0x1000));
}

// Priority
void test_priority_is_masked_to_three_bits(void) {
    set(4, 0xFFFFFFFF);
    TEST_ASSERT_EQUAL_HEX32(7, reg(4));
    set(4, 9);
    TEST_ASSERT_EQUAL_HEX32(1, reg(4));
}

void test_priority_registers_are_independent(void) {
    for (uint32_t s = 1; s <= PLIC_NSOURCES; s++) set(4 * s, s & 7);
    for (uint32_t s = 1; s <= PLIC_NSOURCES; s++) TEST_ASSERT_EQUAL_HEX32(s & 7, reg(4 * s));
}

void test_priority_source_31_works(void) {
    set(4 * 31, 6);
    TEST_ASSERT_EQUAL_HEX32(6, reg(0x7C));
}

void test_priority_source_0_and_32_fault(void) {
    assert_faults(0x0);
    assert_faults(0x80);
}

// Pending
void test_pending_write_is_ignored(void) {
    set(PLIC_PENDING, 0xFFFFFFFF);
    TEST_ASSERT_EQUAL_HEX32(0, reg(PLIC_PENDING));
}

// Enable
void test_enable_bit_0_is_hardwired_zero(void) {
    set(PLIC_ENABLE, 0xFFFFFFFF);
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFE, reg(PLIC_ENABLE));
}

void test_enable_contexts_are_independent(void) {
    set(PLIC_ENABLE, 0x400);
    TEST_ASSERT_EQUAL_HEX32(0x400, reg(PLIC_ENABLE));
    TEST_ASSERT_EQUAL_HEX32(0, reg(PLIC_ENABLE + 0x80));
    set(PLIC_ENABLE + 0x80, 0x1000);
    TEST_ASSERT_EQUAL_HEX32(0x400, reg(PLIC_ENABLE));
    TEST_ASSERT_EQUAL_HEX32(0x1000, reg(PLIC_ENABLE + 0x80));
}

void test_enable_context_2_and_word_1_fault(void) {
    assert_faults(PLIC_ENABLE + 0x100);
    assert_faults(PLIC_ENABLE + 4);
    assert_faults(PLIC_ENABLE + 0x84);
    assert_faults(PLIC_ENABLE + 0x8);
}

// Threshold
void test_threshold_is_masked_to_three_bits(void) {
    set(PLIC_CONTEXT, 9);
    TEST_ASSERT_EQUAL_HEX32(1, reg(PLIC_CONTEXT));
    set(PLIC_CONTEXT, 0xFFFFFFFF);
    TEST_ASSERT_EQUAL_HEX32(7, reg(PLIC_CONTEXT));
}

void test_threshold_contexts_are_independent(void) {
    set(PLIC_CONTEXT, 2);
    TEST_ASSERT_EQUAL_HEX32(0, reg(PLIC_CONTEXT + 0x1000));
    set(PLIC_CONTEXT + 0x1000, 5);
    TEST_ASSERT_EQUAL_HEX32(2, reg(PLIC_CONTEXT));
    TEST_ASSERT_EQUAL_HEX32(5, reg(PLIC_CONTEXT + 0x1000));
}

void test_context_2_faults(void) {
    assert_faults(PLIC_CONTEXT + 0x2000);
    assert_faults(PLIC_CONTEXT + 0x2004);
}

// Claim and complete (stubs until 8.2)
void test_claim_reads_zero_and_complete_is_ignored(void) {
    set(PLIC_CONTEXT + 4, 10);
    TEST_ASSERT_EQUAL_HEX32(0, reg(PLIC_CONTEXT + 4));
    set(PLIC_CONTEXT + 0x1004, 10);
    TEST_ASSERT_EQUAL_HEX32(0, reg(PLIC_CONTEXT + 0x1004));
    // and the threshold next to it is untouched
    TEST_ASSERT_EQUAL_HEX32(0, reg(PLIC_CONTEXT));
}

// Bus rules
void test_only_32_bit_accesses(void) {
    uint32_t v;
    TEST_ASSERT_FALSE(mem_bus_read(&mem, PLIC_BASE + 4, 1, &v));
    TEST_ASSERT_FALSE(mem_bus_read(&mem, PLIC_BASE + 4, 2, &v));
    TEST_ASSERT_FALSE(mem_bus_write(&mem, PLIC_BASE + 4, 1, 1));
    TEST_ASSERT_FALSE(mem_bus_write(&mem, PLIC_BASE + 4, 2, 1));
}

void test_misaligned_access_faults(void) {
    uint32_t v;
    TEST_ASSERT_FALSE(mem_bus_read(&mem, PLIC_BASE + 6, 4, &v));
    TEST_ASSERT_FALSE(mem_bus_write(&mem, PLIC_BASE + 5, 4, 1));
    TEST_ASSERT_FALSE(mem_bus_read(&mem, PLIC_BASE + PLIC_CONTEXT + 2, 4, &v));
}

void test_gaps_fault(void) {
    assert_faults(0x1004);
    assert_faults(0x1F00);
    assert_faults(0x2008);
    assert_faults(0x3FFFFFC);
    assert_faults(PLIC_CONTEXT + 0x8);
    assert_faults(PLIC_CONTEXT + 0xFFC);
    assert_faults(PLIC_CONTEXT + 0x1008);
    assert_faults(0x200000 - 4);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_reset_everything_reads_zero);
    RUN_TEST(test_reset_clears_written_state);
    RUN_TEST(test_priority_is_masked_to_three_bits);
    RUN_TEST(test_priority_registers_are_independent);
    RUN_TEST(test_priority_source_31_works);
    RUN_TEST(test_priority_source_0_and_32_fault);
    RUN_TEST(test_pending_write_is_ignored);
    RUN_TEST(test_enable_bit_0_is_hardwired_zero);
    RUN_TEST(test_enable_contexts_are_independent);
    RUN_TEST(test_enable_context_2_and_word_1_fault);
    RUN_TEST(test_threshold_is_masked_to_three_bits);
    RUN_TEST(test_threshold_contexts_are_independent);
    RUN_TEST(test_context_2_faults);
    RUN_TEST(test_claim_reads_zero_and_complete_is_ignored);
    RUN_TEST(test_only_32_bit_accesses);
    RUN_TEST(test_misaligned_access_faults);
    RUN_TEST(test_gaps_fault);
    return UNITY_END();
}
