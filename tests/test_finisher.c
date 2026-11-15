#include "unity.h"
#include "cpu_test_util.h"
#include "../src/execute.h"
#include "../src/finisher.h"

#define RAM_BASE 0x80000000u
#define MTVEC    (RAM_BASE + 0x100u)

static Memory mem;
static Finisher fin;
static CPU cpu;

void setUp(void) {
    mem_init_at(&mem, RAM_BASE, 0x1000);
    finisher_reset(&fin);
    mem_add_device(&mem, finisher_device(&fin));
}

void tearDown(void) {
    mem_free(&mem);
}

static bool wr(uint32_t offset, uint32_t v) {
    return mem_bus_write(&mem, FINISHER_BASE + offset, 4, v);
}

static bool rd(uint32_t offset, uint32_t *v) {
    return mem_bus_read(&mem, FINISHER_BASE + offset, 4, v);
}

/* ---- The register ---------------------------------------------------------------------------------------- */

void test_a1_nothing_is_done_after_reset(void) {
    TEST_ASSERT_FALSE(fin.done);
    TEST_ASSERT_EQUAL_UINT32(0, fin.status);
    TEST_ASSERT_EQUAL_UINT32(0, fin.code);
}

void test_a2_a_read_returns_zero_and_changes_nothing(void) {
    uint32_t v = 0xDEADBEEF;
    TEST_ASSERT_TRUE(rd(0, &v));
    TEST_ASSERT_EQUAL_HEX32(0, v);
    TEST_ASSERT_FALSE(fin.done);
}

void test_a3_pass(void) {
    TEST_ASSERT_TRUE(wr(0, FINISHER_PASS));
    TEST_ASSERT_TRUE(fin.done);
    TEST_ASSERT_EQUAL_UINT32(FINISHER_PASS, fin.status);
}

void test_a4_fail_carries_the_code_from_bits_31_to_16(void) {
    TEST_ASSERT_TRUE(wr(0, FINISHER_FAIL | (42u << 16)));
    TEST_ASSERT_TRUE(fin.done);
    TEST_ASSERT_EQUAL_UINT32(FINISHER_FAIL, fin.status);
    TEST_ASSERT_EQUAL_UINT32(42, fin.code);
}

void test_a5_the_largest_code(void) {
    TEST_ASSERT_TRUE(wr(0, FINISHER_FAIL | (0xFFFFu << 16)));
    TEST_ASSERT_EQUAL_UINT32(0xFFFF, fin.code);
}

void test_a6_reset(void) {
    TEST_ASSERT_TRUE(wr(0, FINISHER_RESET));
    TEST_ASSERT_TRUE(fin.done);
    TEST_ASSERT_EQUAL_UINT32(FINISHER_RESET, fin.status);
}

void test_a7_other_values_are_accepted_and_ignored(void) {
    static const uint32_t values[] = { 0, 1, 0x1234, 0x5554, 0x5556, 0x3332, 0x7778, 0xFFFF, 0x10000, 0xFFFF0000u, 0xAAAA5554u };
    for (unsigned i = 0; i < sizeof values / sizeof values[0]; i++) {
        TEST_ASSERT_TRUE(wr(0, values[i]));
        TEST_ASSERT_FALSE(fin.done);
    }
}

void test_a8_the_status_is_the_low_16_bits_only(void) {
    TEST_ASSERT_TRUE(wr(0, 0x00015555u));              // pass in the low half, junk above: a pass, the high half is the code
    TEST_ASSERT_TRUE(fin.done);
    TEST_ASSERT_EQUAL_UINT32(FINISHER_PASS, fin.status);
    TEST_ASSERT_EQUAL_UINT32(1, fin.code);
}

void test_a9_the_first_valid_write_stays(void) {
    TEST_ASSERT_TRUE(wr(0, FINISHER_FAIL | (3u << 16)));
    TEST_ASSERT_TRUE(wr(0, FINISHER_PASS));
    TEST_ASSERT_TRUE(wr(0, FINISHER_RESET));
    TEST_ASSERT_EQUAL_UINT32(FINISHER_FAIL, fin.status);
    TEST_ASSERT_EQUAL_UINT32(3, fin.code);
}

void test_a10_an_invalid_write_does_not_use_up_the_first_one(void) {
    TEST_ASSERT_TRUE(wr(0, 0x1234));
    TEST_ASSERT_TRUE(wr(0, FINISHER_PASS));
    TEST_ASSERT_TRUE(fin.done);
    TEST_ASSERT_EQUAL_UINT32(FINISHER_PASS, fin.status);
}

void test_a11_opensbi_reads_then_writes(void) {
    // regmap_update_bits(offset 0, mask 0xFFFFFFFF, value): read the register, merge, write it back
    uint32_t old = 0xDEADBEEF;
    TEST_ASSERT_TRUE(rd(0, &old));
    uint32_t mask = 0xFFFFFFFFu, value = FINISHER_PASS;
    TEST_ASSERT_TRUE(wr(0, (old & ~mask) | (value & mask)));
    TEST_ASSERT_TRUE(fin.done);
    TEST_ASSERT_EQUAL_UINT32(FINISHER_PASS, fin.status);
}

void test_a12_reset_clears_the_state(void) {
    wr(0, FINISHER_FAIL | (9u << 16));
    finisher_reset(&fin);
    TEST_ASSERT_FALSE(fin.done);
    TEST_ASSERT_EQUAL_UINT32(0, fin.code);
    TEST_ASSERT_TRUE(wr(0, FINISHER_PASS));
    TEST_ASSERT_EQUAL_UINT32(FINISHER_PASS, fin.status);
}

/* ---- The bus rules --------------------------------------------------------------------------------------- */

void test_b1_only_32_bit_accesses(void) {
    uint32_t v;
    TEST_ASSERT_FALSE(mem_bus_read(&mem, FINISHER_BASE, 1, &v));
    TEST_ASSERT_FALSE(mem_bus_read(&mem, FINISHER_BASE, 2, &v));
    TEST_ASSERT_FALSE(mem_bus_write(&mem, FINISHER_BASE, 1, FINISHER_PASS));
    TEST_ASSERT_FALSE(mem_bus_write(&mem, FINISHER_BASE, 2, FINISHER_PASS));
    TEST_ASSERT_FALSE(fin.done);
}

void test_b2_misaligned_accesses_fault(void) {
    uint32_t v;
    TEST_ASSERT_FALSE(mem_bus_read(&mem, FINISHER_BASE + 2, 4, &v));
    TEST_ASSERT_FALSE(mem_bus_write(&mem, FINISHER_BASE + 1, 4, FINISHER_PASS));
    TEST_ASSERT_FALSE(fin.done);
}

void test_b3_other_offsets_fault(void) {
    uint32_t v;
    static const uint32_t offsets[] = { 4, 8, 0x100, 0xFFC };
    for (unsigned i = 0; i < sizeof offsets / sizeof offsets[0]; i++) {
        TEST_ASSERT_FALSE(rd(offsets[i], &v));
        TEST_ASSERT_FALSE(wr(offsets[i], FINISHER_PASS));
    }
    TEST_ASSERT_FALSE(fin.done);
}

void test_b4_fetching_from_it_faults(void) {
    test_cpu_reset_in_mode(&cpu, PRV_M);
    cpu_write_csr(&cpu, CSR_MTVEC, MTVEC);
    cpu.pc = FINISHER_BASE;
    cpu_step(&cpu, &mem);
    TEST_ASSERT_EQUAL_HEX32(1, cpu_read_csr(&cpu, CSR_MCAUSE));
    TEST_ASSERT_EQUAL_HEX32(FINISHER_BASE, cpu_read_csr(&cpu, CSR_MTVAL));
}

void test_b5_a_store_word_from_the_cpu_finishes(void) {
    test_cpu_reset_in_mode(&cpu, PRV_M);
    mem_write32(&mem, RAM_BASE, 0x00B52023u);          // sw a1, 0(a0)
    cpu_write_reg(&cpu, 10, FINISHER_BASE);
    cpu_write_reg(&cpu, 11, FINISHER_FAIL | (7u << 16));
    cpu.pc = RAM_BASE;
    cpu_step(&cpu, &mem);
    TEST_ASSERT_TRUE(fin.done);
    TEST_ASSERT_EQUAL_UINT32(FINISHER_FAIL, fin.status);
    TEST_ASSERT_EQUAL_UINT32(7, fin.code);
}

void test_b6_a_store_byte_from_the_cpu_traps_and_does_not_finish(void) {
    test_cpu_reset_in_mode(&cpu, PRV_M);
    cpu_write_csr(&cpu, CSR_MTVEC, MTVEC);
    mem_write32(&mem, RAM_BASE, 0x00B50023u);          // sb a1, 0(a0)
    cpu_write_reg(&cpu, 10, FINISHER_BASE);
    cpu_write_reg(&cpu, 11, FINISHER_PASS);
    cpu.pc = RAM_BASE;
    cpu_step(&cpu, &mem);
    TEST_ASSERT_FALSE(fin.done);
    TEST_ASSERT_EQUAL_HEX32(7, cpu_read_csr(&cpu, CSR_MCAUSE));
    TEST_ASSERT_EQUAL_HEX32(FINISHER_BASE, cpu_read_csr(&cpu, CSR_MTVAL));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_a1_nothing_is_done_after_reset);
    RUN_TEST(test_a2_a_read_returns_zero_and_changes_nothing);
    RUN_TEST(test_a3_pass);
    RUN_TEST(test_a4_fail_carries_the_code_from_bits_31_to_16);
    RUN_TEST(test_a5_the_largest_code);
    RUN_TEST(test_a6_reset);
    RUN_TEST(test_a7_other_values_are_accepted_and_ignored);
    RUN_TEST(test_a8_the_status_is_the_low_16_bits_only);
    RUN_TEST(test_a9_the_first_valid_write_stays);
    RUN_TEST(test_a10_an_invalid_write_does_not_use_up_the_first_one);
    RUN_TEST(test_a11_opensbi_reads_then_writes);
    RUN_TEST(test_a12_reset_clears_the_state);
    RUN_TEST(test_b1_only_32_bit_accesses);
    RUN_TEST(test_b2_misaligned_accesses_fault);
    RUN_TEST(test_b3_other_offsets_fault);
    RUN_TEST(test_b4_fetching_from_it_faults);
    RUN_TEST(test_b5_a_store_word_from_the_cpu_finishes);
    RUN_TEST(test_b6_a_store_byte_from_the_cpu_traps_and_does_not_finish);
    return UNITY_END();
}
