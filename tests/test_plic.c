#include <string.h>
#include "unity.h"
#include "../src/plic.h"

#define RAM_BASE 0x80000000u

static Plic plic;
static Memory mem;

static bool level[PLIC_NSOURCES + 1];

void setUp(void) {
    mem_init_at(&mem, RAM_BASE, 0x1000);
    memset(&plic, 0, sizeof plic);   // plic_reset keeps the source lines, so start from nothing
    memset(level, 0, sizeof level);
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

// ---- Gateway, claim and complete (8.2) ----
// A test source: its level is a plain flag
static bool test_line(void *ctx) { return *(bool *)ctx; }

static void connect(uint32_t id, uint32_t prio) {
    plic_set_source(&plic, id, test_line, &level[id]);
    set(4 * id, prio);
}

static void enable_for(uint32_t context, uint32_t id) {
    uint32_t off = PLIC_ENABLE + 0x80 * context;
    set(off, reg(off) | (1u << id));
}

static uint32_t claim(uint32_t context) { return reg(PLIC_CONTEXT + 0x1000 * context + 4); }
static void complete(uint32_t context, uint32_t id) { set(PLIC_CONTEXT + 0x1000 * context + 4, id); }
static void threshold(uint32_t context, uint32_t v) { set(PLIC_CONTEXT + 0x1000 * context, v); }

void test_unconnected_claim_reads_zero_and_complete_is_ignored(void) {
    set(PLIC_CONTEXT + 4, 10);
    TEST_ASSERT_EQUAL_HEX32(0, reg(PLIC_CONTEXT + 4));
    set(PLIC_CONTEXT + 0x1004, 10);
    TEST_ASSERT_EQUAL_HEX32(0, reg(PLIC_CONTEXT + 0x1004));
    TEST_ASSERT_EQUAL_HEX32(0, reg(PLIC_CONTEXT));   // the threshold next to it is untouched
}

void test_a_high_line_raises_eip_on_the_enabled_context_only(void) {
    connect(10, 3);
    enable_for(0, 10);
    TEST_ASSERT_FALSE(plic_context_pending(&plic, 0));      // line low
    level[10] = true;
    TEST_ASSERT_TRUE(plic_context_pending(&plic, 0));
    TEST_ASSERT_FALSE(plic_context_pending(&plic, 1));      // not enabled there
    TEST_ASSERT_EQUAL_HEX32(1u << 10, reg(PLIC_PENDING));
}

void test_a_pending_read_samples_the_gateway_itself(void) {
    connect(10, 3);
    level[10] = true;                                       // nothing has sampled the line yet
    TEST_ASSERT_EQUAL_HEX32(1u << 10, reg(PLIC_PENDING));
}

void test_claim_returns_the_id_clears_pending_and_blocks_the_gateway(void) {
    connect(10, 3);
    enable_for(0, 10);
    level[10] = true;
    TEST_ASSERT_EQUAL_UINT32(10, claim(0));
    TEST_ASSERT_EQUAL_HEX32(0, reg(PLIC_PENDING));
    TEST_ASSERT_EQUAL_UINT32(0, claim(0));                  // nothing more while it is in flight
    TEST_ASSERT_FALSE(plic_context_pending(&plic, 0));      // the line is still high, but no new request
    TEST_ASSERT_EQUAL_HEX32(0, reg(PLIC_PENDING));
}

void test_complete_with_the_line_still_high_forwards_a_new_request(void) {
    connect(10, 3);
    enable_for(0, 10);
    level[10] = true;
    TEST_ASSERT_EQUAL_UINT32(10, claim(0));
    complete(0, 10);
    TEST_ASSERT_TRUE(plic_context_pending(&plic, 0));
    TEST_ASSERT_EQUAL_UINT32(10, claim(0));
}

void test_complete_with_the_line_low_does_not_raise_again(void) {
    connect(10, 3);
    enable_for(0, 10);
    level[10] = true;
    TEST_ASSERT_EQUAL_UINT32(10, claim(0));
    level[10] = false;
    complete(0, 10);
    TEST_ASSERT_FALSE(plic_context_pending(&plic, 0));
    TEST_ASSERT_EQUAL_HEX32(0, reg(PLIC_PENDING));
    level[10] = true;                                       // a new assertion is a new request
    TEST_ASSERT_TRUE(plic_context_pending(&plic, 0));
}

void test_a_request_cannot_be_retracted(void) {
    connect(10, 3);
    enable_for(0, 10);
    level[10] = true;
    TEST_ASSERT_TRUE(plic_context_pending(&plic, 0));
    level[10] = false;                                      // the line drops before the claim
    TEST_ASSERT_TRUE(plic_context_pending(&plic, 0));
    TEST_ASSERT_EQUAL_UINT32(10, claim(0));                 // a spurious claim still returns the id
    complete(0, 10);
    TEST_ASSERT_FALSE(plic_context_pending(&plic, 0));
}

void test_priority_zero_never_interrupts_and_is_never_claimed(void) {
    connect(10, 0);
    enable_for(0, 10);
    level[10] = true;
    TEST_ASSERT_FALSE(plic_context_pending(&plic, 0));
    TEST_ASSERT_EQUAL_UINT32(0, claim(0));
    TEST_ASSERT_EQUAL_HEX32(1u << 10, reg(PLIC_PENDING));  // still pending, so it fires once the priority is raised
    set(4 * 10, 1);
    TEST_ASSERT_TRUE(plic_context_pending(&plic, 0));
}

void test_eip_needs_priority_above_the_threshold(void) {
    connect(10, 4);
    enable_for(0, 10);
    level[10] = true;
    threshold(0, 3);
    TEST_ASSERT_TRUE(plic_context_pending(&plic, 0));
    threshold(0, 4);                                        // equal: masked
    TEST_ASSERT_FALSE(plic_context_pending(&plic, 0));
    threshold(0, 7);
    TEST_ASSERT_FALSE(plic_context_pending(&plic, 0));
}

void test_the_threshold_does_not_stop_a_claim(void) {
    connect(10, 2);
    enable_for(0, 10);
    threshold(0, 7);
    level[10] = true;
    TEST_ASSERT_FALSE(plic_context_pending(&plic, 0));
    TEST_ASSERT_EQUAL_UINT32(10, claim(0));
}

void test_the_threshold_is_per_context(void) {
    connect(10, 2);
    enable_for(0, 10);
    enable_for(1, 10);
    threshold(0, 7);
    level[10] = true;
    TEST_ASSERT_FALSE(plic_context_pending(&plic, 0));
    TEST_ASSERT_TRUE(plic_context_pending(&plic, 1));
}

void test_a_source_not_enabled_for_a_context_is_not_claimed_there(void) {
    connect(10, 3);
    enable_for(1, 10);
    level[10] = true;
    plic_context_pending(&plic, 0);                         // latches the request
    TEST_ASSERT_EQUAL_UINT32(0, claim(0));
    TEST_ASSERT_EQUAL_HEX32(1u << 10, reg(PLIC_PENDING));  // untouched
    TEST_ASSERT_EQUAL_UINT32(10, claim(1));
}

void test_claim_order_is_priority_then_lowest_id(void) {
    connect(3, 2);
    connect(5, 6);
    connect(7, 6);
    enable_for(0, 3);
    enable_for(0, 5);
    enable_for(0, 7);
    level[3] = level[5] = level[7] = true;
    TEST_ASSERT_EQUAL_UINT32(5, claim(0));
    TEST_ASSERT_EQUAL_UINT32(7, claim(0));
    TEST_ASSERT_EQUAL_UINT32(3, claim(0));
    TEST_ASSERT_EQUAL_UINT32(0, claim(0));
}

void test_eip_follows_the_best_source_priority(void) {
    connect(3, 1);
    connect(5, 6);
    enable_for(0, 3);
    enable_for(0, 5);
    level[3] = true;
    threshold(0, 2);
    TEST_ASSERT_FALSE(plic_context_pending(&plic, 0));      // only the priority-1 source is pending
    level[5] = true;
    TEST_ASSERT_TRUE(plic_context_pending(&plic, 0));
}

void test_complete_of_an_id_not_enabled_for_the_context_is_ignored(void) {
    connect(10, 3);
    enable_for(0, 10);
    level[10] = true;
    TEST_ASSERT_EQUAL_UINT32(10, claim(0));
    complete(1, 10);                                        // context 1 has not enabled it
    TEST_ASSERT_FALSE(plic_context_pending(&plic, 0));      // still in flight
    complete(0, 10);
    TEST_ASSERT_TRUE(plic_context_pending(&plic, 0));
}

void test_complete_of_invalid_ids_is_ignored(void) {
    connect(10, 3);
    enable_for(0, 10);
    set(PLIC_ENABLE, 0xFFFFFFFE);                           // every source enabled
    level[10] = true;
    TEST_ASSERT_EQUAL_UINT32(10, claim(0));
    complete(0, 0);
    complete(0, 32);
    complete(0, 0xFFFFFFFF);
    TEST_ASSERT_FALSE(plic_context_pending(&plic, 0));      // 10 is still in flight
}

void test_complete_need_not_match_the_claimed_id(void) {
    connect(10, 3);
    connect(11, 3);
    set(PLIC_ENABLE, 0xFFFFFFFE);
    level[10] = true;
    level[11] = true;
    TEST_ASSERT_EQUAL_UINT32(10, claim(0));
    TEST_ASSERT_EQUAL_UINT32(11, claim(0));
    complete(0, 11);                                        // only 11 is re-armed
    TEST_ASSERT_TRUE(plic_context_pending(&plic, 0));
    TEST_ASSERT_EQUAL_UINT32(11, claim(0));
    TEST_ASSERT_EQUAL_UINT32(0, claim(0));                  // 10 is still in flight
}

void test_reset_keeps_the_sources_and_clears_the_state(void) {
    connect(10, 3);
    enable_for(0, 10);
    level[10] = true;
    TEST_ASSERT_EQUAL_UINT32(10, claim(0));                 // in flight
    plic_reset(&plic);
    TEST_ASSERT_EQUAL_HEX32(0, reg(4 * 10));
    TEST_ASSERT_EQUAL_HEX32(0, reg(PLIC_ENABLE));
    set(4 * 10, 3);
    enable_for(0, 10);
    TEST_ASSERT_TRUE(plic_context_pending(&plic, 0));       // the line is still connected and not in flight
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
    RUN_TEST(test_unconnected_claim_reads_zero_and_complete_is_ignored);
    RUN_TEST(test_a_high_line_raises_eip_on_the_enabled_context_only);
    RUN_TEST(test_a_pending_read_samples_the_gateway_itself);
    RUN_TEST(test_claim_returns_the_id_clears_pending_and_blocks_the_gateway);
    RUN_TEST(test_complete_with_the_line_still_high_forwards_a_new_request);
    RUN_TEST(test_complete_with_the_line_low_does_not_raise_again);
    RUN_TEST(test_a_request_cannot_be_retracted);
    RUN_TEST(test_priority_zero_never_interrupts_and_is_never_claimed);
    RUN_TEST(test_eip_needs_priority_above_the_threshold);
    RUN_TEST(test_the_threshold_does_not_stop_a_claim);
    RUN_TEST(test_the_threshold_is_per_context);
    RUN_TEST(test_a_source_not_enabled_for_a_context_is_not_claimed_there);
    RUN_TEST(test_claim_order_is_priority_then_lowest_id);
    RUN_TEST(test_eip_follows_the_best_source_priority);
    RUN_TEST(test_complete_of_an_id_not_enabled_for_the_context_is_ignored);
    RUN_TEST(test_complete_of_invalid_ids_is_ignored);
    RUN_TEST(test_complete_need_not_match_the_claimed_id);
    RUN_TEST(test_reset_keeps_the_sources_and_clears_the_state);
    RUN_TEST(test_only_32_bit_accesses);
    RUN_TEST(test_misaligned_access_faults);
    RUN_TEST(test_gaps_fault);
    return UNITY_END();
}
