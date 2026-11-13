#include "unity.h"
#include "../src/execute.h"
#include "../src/clint.h"

// mie, mip, sie, sip and mideleg with the machine-level interrupts (Milestone 6.3). MSIP, MTIP and MEIP
// are read-only in mip and driven by the platform (machine.adoc:1400-1410); MSIE, MTIE and MEIE are
// writable in mie (:1370). sip and sie only ever show the supervisor bits.

static CPU c;
static Memory mem;

void setUp(void) {
    cpu_reset(&c);                                  // mtime 0, mtimecmp all ones: no timer pending
    mem_init(&mem, 64);
}

void tearDown(void) {
    mem_free(&mem);
}

static void timer_on(void)  { c.clint.mtimecmp = 0; }                  // mtime >= mtimecmp
static void timer_off(void) { c.clint.mtimecmp = UINT64_MAX; }

/* ---- A. mie ------------------------------------------------------------------------------------- */

void test_a1_all_ones(void) {
    cpu_write_csr(&c, CSR_MIE, 0xFFFFFFFF);
    TEST_ASSERT_EQUAL_HEX32(0xAAA, cpu_read_csr(&c, CSR_MIE));
}

void test_a2_each_machine_enable_alone(void) {
    cpu_write_csr(&c, CSR_MIE, 0x008);
    TEST_ASSERT_EQUAL_HEX32(0x008, cpu_read_csr(&c, CSR_MIE));              // MSIE
    cpu_write_csr(&c, CSR_MIE, 0x080);
    TEST_ASSERT_EQUAL_HEX32(0x080, cpu_read_csr(&c, CSR_MIE));              // MTIE
    cpu_write_csr(&c, CSR_MIE, 0x800);
    TEST_ASSERT_EQUAL_HEX32(0x800, cpu_read_csr(&c, CSR_MIE));              // MEIE
}

void test_a3_the_other_bits_read_zero(void) {
    const struct { uint32_t written, read; } v[] = {
        {0x00002000, 0x000},      // bit 13 (LCOFIE, not implemented)
        {0x00005555, 0x000},      // every even bit
        {0x0000AAAA, 0xAAA},      // every odd bit: only the six implemented ones stay
        {0x00000001, 0x000},      // bit 0
        {0x00000002, 0x002},      // SSIE
        {0xFFFF0000, 0x000},      // the platform-use bits 31:16
    };
    for (unsigned i = 0; i < sizeof v / sizeof v[0]; i++) {
        cpu_write_csr(&c, CSR_MIE, v[i].written);
        TEST_ASSERT_EQUAL_HEX32_MESSAGE(v[i].read, cpu_read_csr(&c, CSR_MIE), "mie readback");
    }
}

void test_a4_clearing(void) {
    cpu_write_csr(&c, CSR_MIE, 0xFFFFFFFF);
    cpu_write_csr(&c, CSR_MIE, 0);
    TEST_ASSERT_EQUAL_HEX32(0, cpu_read_csr(&c, CSR_MIE));
}

/* ---- B. mip ------------------------------------------------------------------------------------- */

void test_b1_reset(void) {
    TEST_ASSERT_EQUAL_HEX32(0, cpu_read_csr(&c, CSR_MIP));
}

void test_b2_writes_store_only_the_supervisor_bits(void) {
    cpu_write_csr(&c, CSR_MIP, 0xFFFFFFFF);
    TEST_ASSERT_EQUAL_HEX32(0x222, c.csr[CSR_MIP]);
    TEST_ASSERT_EQUAL_HEX32(0x222, cpu_read_csr(&c, CSR_MIP));
}

void test_b3_mtip_follows_the_timer(void) {
    timer_on();
    TEST_ASSERT_EQUAL_HEX32(0x080, cpu_read_csr(&c, CSR_MIP));              // no write to mip happened
}

void test_b4_and_goes_away_again(void) {
    timer_on();
    TEST_ASSERT_EQUAL_HEX32(0x080, cpu_read_csr(&c, CSR_MIP));
    timer_off();
    TEST_ASSERT_EQUAL_HEX32(0x000, cpu_read_csr(&c, CSR_MIP));
}

void test_b4b_mtip_follows_mtime_passing_mtimecmp(void) {
    c.clint.mtimecmp = 1;
    TEST_ASSERT_EQUAL_HEX32(0, cpu_read_csr(&c, CSR_MIP));
    for (unsigned i = 0; i < CLINT_CYCLES_PER_TICK; i++) {
        clint_tick(&c.clint);
    }
    TEST_ASSERT_EQUAL_HEX32(0x080, cpu_read_csr(&c, CSR_MIP));
}

void test_b5_msip_follows_the_software_register(void) {
    c.clint.msip = 1;
    TEST_ASSERT_EQUAL_HEX32(0x008, cpu_read_csr(&c, CSR_MIP));
    c.clint.msip = 0;
    TEST_ASSERT_EQUAL_HEX32(0x000, cpu_read_csr(&c, CSR_MIP));
}

void test_b6_both_lines(void) {
    timer_on();
    c.clint.msip = 1;
    TEST_ASSERT_EQUAL_HEX32(0x088, cpu_read_csr(&c, CSR_MIP));
}

void test_b7_lines_and_stored_supervisor_bits(void) {
    cpu_write_csr(&c, CSR_MIP, 0x222);
    timer_on();
    c.clint.msip = 1;
    TEST_ASSERT_EQUAL_HEX32(0x2AA, cpu_read_csr(&c, CSR_MIP));
}

void test_b8_meip_stays_zero_without_a_plic(void) {
    timer_on();
    c.clint.msip = 1;
    cpu_write_csr(&c, CSR_MIP, 0xFFFFFFFF);
    TEST_ASSERT_EQUAL_HEX32(0, cpu_read_csr(&c, CSR_MIP) & INT_MEI);
}

void test_b9_a_write_cannot_clear_a_machine_bit(void) {
    timer_on();
    c.clint.msip = 1;
    cpu_write_csr(&c, CSR_MIP, 0);
    TEST_ASSERT_EQUAL_HEX32(0x088, cpu_read_csr(&c, CSR_MIP));
}

/* ---- C. Through the CSR instructions ------------------------------------------------------------- */

#define X1 1
#define X5 5
static uint32_t csr_insn(uint32_t csr, uint32_t funct3, uint32_t rd, uint32_t rs1) {
    return (csr << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | 0x73;
}
#define CSRRW 1u
#define CSRRS 2u
#define CSRRC 3u

static void run(uint32_t raw) {
    DecodedInstruction instr = decode_instruction(raw);
    c.pc = 0;
    execute(&c, &mem, &instr);
    TEST_ASSERT_EQUAL_HEX32(4, c.pc);               // it did not trap
}

void test_c1_csrrs_reads_the_line(void) {
    timer_on();
    run(csr_insn(CSR_MIP, CSRRS, X1, 0));
    TEST_ASSERT_EQUAL_HEX32(0x080, cpu_read_reg(&c, X1));
}

void test_c2_csrrc_cannot_clear_mtip(void) {
    timer_on();
    cpu_write_reg(&c, X5, 0x080);
    run(csr_insn(CSR_MIP, CSRRC, 0, X5));
    TEST_ASSERT_EQUAL_HEX32(0x080, cpu_read_csr(&c, CSR_MIP));
}

void test_c3_csrrw_zero_clears_the_stored_bits_only(void) {
    cpu_write_csr(&c, CSR_MIP, 0x222);
    timer_on();
    c.clint.msip = 1;
    cpu_write_reg(&c, X5, 0);
    run(csr_insn(CSR_MIP, CSRRW, 0, X5));
    TEST_ASSERT_EQUAL_HEX32(0x088, cpu_read_csr(&c, CSR_MIP));
}

void test_c4_csrrs_sets_ssip_and_leaves_mtip(void) {
    timer_on();
    cpu_write_reg(&c, X5, 0x2);
    run(csr_insn(CSR_MIP, CSRRS, 0, X5));
    TEST_ASSERT_EQUAL_HEX32(0x082, cpu_read_csr(&c, CSR_MIP));
    TEST_ASSERT_EQUAL_HEX32(0x002, c.csr[CSR_MIP]);                         // MTIP was not stored
}

void test_c5_csrrw_and_csrr_mie(void) {
    cpu_write_reg(&c, X5, 0x888);
    run(csr_insn(CSR_MIE, CSRRW, 0, X5));
    run(csr_insn(CSR_MIE, CSRRS, X1, 0));
    TEST_ASSERT_EQUAL_HEX32(0x888, cpu_read_reg(&c, X1));
}

/* ---- D. sip, sie and mideleg never show the machine bits ----------------------------------------- */

void test_d1_sie_shows_only_delegated_supervisor_bits(void) {
    cpu_write_csr(&c, CSR_MIE, 0xFFFFFFFF);
    cpu_write_csr(&c, CSR_MIDELEG, 0x222);
    TEST_ASSERT_EQUAL_HEX32(0x222, cpu_read_csr(&c, CSR_SIE));
}

void test_d2_sip_never_shows_a_machine_line(void) {
    timer_on();
    c.clint.msip = 1;
    cpu_write_csr(&c, CSR_MIDELEG, 0x222);
    TEST_ASSERT_EQUAL_HEX32(0, cpu_read_csr(&c, CSR_SIP));
    cpu_write_csr(&c, CSR_MIP, 0x222);
    TEST_ASSERT_EQUAL_HEX32(0x222, cpu_read_csr(&c, CSR_SIP));
}

void test_d3_the_machine_bits_cannot_be_delegated(void) {
    cpu_write_csr(&c, CSR_MIDELEG, 0x888);
    TEST_ASSERT_EQUAL_HEX32(0, cpu_read_csr(&c, CSR_MIDELEG));
}

void test_d4_only_the_supervisor_bits_are_delegatable(void) {
    cpu_write_csr(&c, CSR_MIDELEG, 0xAAA);
    TEST_ASSERT_EQUAL_HEX32(0x222, cpu_read_csr(&c, CSR_MIDELEG));
}

void test_d5_a_sie_write_leaves_the_machine_enables(void) {
    cpu_write_csr(&c, CSR_MIE, 0xAAA);
    cpu_write_csr(&c, CSR_MIDELEG, 0x222);
    cpu_write_csr(&c, CSR_SIE, 0x888);
    TEST_ASSERT_EQUAL_HEX32(0x888, cpu_read_csr(&c, CSR_MIE));              // the S enables were cleared, the M ones kept
}

void test_d6_a_sip_write_leaves_the_machine_lines(void) {
    timer_on();
    c.clint.msip = 1;
    cpu_write_csr(&c, CSR_MIDELEG, 0x222);
    cpu_write_csr(&c, CSR_SIP, 0x888);
    TEST_ASSERT_EQUAL_HEX32(0x088, cpu_read_csr(&c, CSR_MIP));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_a1_all_ones);
    RUN_TEST(test_a2_each_machine_enable_alone);
    RUN_TEST(test_a3_the_other_bits_read_zero);
    RUN_TEST(test_a4_clearing);
    RUN_TEST(test_b1_reset);
    RUN_TEST(test_b2_writes_store_only_the_supervisor_bits);
    RUN_TEST(test_b3_mtip_follows_the_timer);
    RUN_TEST(test_b4_and_goes_away_again);
    RUN_TEST(test_b4b_mtip_follows_mtime_passing_mtimecmp);
    RUN_TEST(test_b5_msip_follows_the_software_register);
    RUN_TEST(test_b6_both_lines);
    RUN_TEST(test_b7_lines_and_stored_supervisor_bits);
    RUN_TEST(test_b8_meip_stays_zero_without_a_plic);
    RUN_TEST(test_b9_a_write_cannot_clear_a_machine_bit);
    RUN_TEST(test_c1_csrrs_reads_the_line);
    RUN_TEST(test_c2_csrrc_cannot_clear_mtip);
    RUN_TEST(test_c3_csrrw_zero_clears_the_stored_bits_only);
    RUN_TEST(test_c4_csrrs_sets_ssip_and_leaves_mtip);
    RUN_TEST(test_c5_csrrw_and_csrr_mie);
    RUN_TEST(test_d1_sie_shows_only_delegated_supervisor_bits);
    RUN_TEST(test_d2_sip_never_shows_a_machine_line);
    RUN_TEST(test_d3_the_machine_bits_cannot_be_delegated);
    RUN_TEST(test_d4_only_the_supervisor_bits_are_delegatable);
    RUN_TEST(test_d5_a_sie_write_leaves_the_machine_enables);
    RUN_TEST(test_d6_a_sip_write_leaves_the_machine_lines);
    return UNITY_END();
}
