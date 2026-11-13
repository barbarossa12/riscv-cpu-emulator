#include "unity.h"
#include "../src/execute.h"
#include "../src/clint.h"

// time and timeh, the read-only shadows of the CLINT's mtime (machine.adoc:1598-1603), and the TM bit that
// gates them from S-mode and U-mode (machine.adoc:1571, supervisor.adoc:510).

#define RAM_BASE 0x80000000u
#define MTVEC    0x80000100u

#define X1 1
#define X5 5
#define CSRRW  1u
#define CSRRS  2u
#define CSRRC  3u
#define CSRRWI 5u
#define CSRRSI 6u

static CPU cpu;
static Memory mem;

void setUp(void) {
    mem_init_at(&mem, RAM_BASE, 0x2000);
    cpu_reset(&cpu);
    mem_add_device(&mem, clint_device(&cpu.clint));
    cpu_write_csr(&cpu, CSR_MTVEC, MTVEC);
}

void tearDown(void) {
    mem_free(&mem);
}

static uint32_t csr_insn(uint32_t csr, uint32_t funct3, uint32_t rd, uint32_t rs1) {
    return (csr << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | 0x73;
}

// Runs one instruction in a mode. True if it trapped as an illegal instruction.
static bool traps(PrivMode mode, uint32_t raw) {
    DecodedInstruction instr = decode_instruction(raw);
    cpu.privilege_mode = mode;
    cpu.pc = 0x1000;
    cpu_write_csr(&cpu, CSR_MCAUSE, 0);
    execute(&cpu, &mem, &instr);
    if (cpu.pc == MTVEC) {
        TEST_ASSERT_EQUAL_UINT32(2, cpu_read_csr(&cpu, CSR_MCAUSE));
        return true;
    }
    TEST_ASSERT_EQUAL_HEX32(0x1004, cpu.pc);
    return false;
}

static void counteren(uint32_t m, uint32_t s) {
    cpu_write_csr(&cpu, CSR_MCOUNTEREN, m);
    cpu_write_csr(&cpu, CSR_SCOUNTEREN, s);
}

/* ---- B. Reading time ------------------------------------------------------------------------------ */

void test_b1_the_two_halves(void) {
    cpu.clint.mtime = 0x0000000500000007ull;
    TEST_ASSERT_EQUAL_HEX32(7, cpu_read_csr(&cpu, CSR_TIME));
    TEST_ASSERT_EQUAL_HEX32(5, cpu_read_csr(&cpu, CSR_TIMEH));
    TEST_ASSERT_FALSE(traps(PRV_M, csr_insn(CSR_TIME, CSRRS, X1, 0)));
    TEST_ASSERT_EQUAL_HEX32(7, cpu_read_reg(&cpu, X1));
    TEST_ASSERT_FALSE(traps(PRV_M, csr_insn(CSR_TIMEH, CSRRS, X1, 0)));
    TEST_ASSERT_EQUAL_HEX32(5, cpu_read_reg(&cpu, X1));
}

void test_b2_a_write_through_the_clint_shows_at_once(void) {
    TEST_ASSERT_TRUE(mem_bus_write(&mem, CLINT_BASE + CLINT_MTIME, 4, 0x1234));
    TEST_ASSERT_TRUE(mem_bus_write(&mem, CLINT_BASE + CLINT_MTIME + 4, 4, 0x9));
    TEST_ASSERT_EQUAL_HEX32(0x1234, cpu_read_csr(&cpu, CSR_TIME));
    TEST_ASSERT_EQUAL_HEX32(0x9, cpu_read_csr(&cpu, CSR_TIMEH));
}

void test_b3_crossing_32_bits(void) {
    cpu.clint.mtime = 0xFFFFFFFFull;
    for (unsigned i = 0; i < CLINT_CYCLES_PER_TICK; i++) {
        clint_tick(&cpu.clint);
    }
    TEST_ASSERT_EQUAL_HEX32(0, cpu_read_csr(&cpu, CSR_TIME));
    TEST_ASSERT_EQUAL_HEX32(1, cpu_read_csr(&cpu, CSR_TIMEH));
}

static void step_nops(unsigned n) {
    for (uint32_t a = RAM_BASE; a < RAM_BASE + 0x1000; a += 4) {
        mem_write32(&mem, a, 0x00000013);
    }
    cpu.pc = RAM_BASE;
    while (n--) {
        cpu_step(&cpu, &mem);
    }
}

void test_b4_time_follows_cpu_steps(void) {
    step_nops(37);
    TEST_ASSERT_EQUAL_UINT64(cpu.clint.mtime, cpu_read_csr(&cpu, CSR_TIME));
    TEST_ASSERT_EQUAL_UINT64(3, cpu_read_csr(&cpu, CSR_TIME));
}

void test_b5_time_and_cycle(void) {
    step_nops(100);
    TEST_ASSERT_EQUAL_UINT32(10, cpu_read_csr(&cpu, CSR_TIME));
    TEST_ASSERT_EQUAL_UINT32(100, cpu_read_csr(&cpu, CSR_CYCLE));
}

/* ---- C. Gating -------------------------------------------------------------------------------------- */

void test_c1_m_mode_is_never_gated(void) {
    counteren(0, 0);
    TEST_ASSERT_FALSE(traps(PRV_M, csr_insn(CSR_TIME, CSRRS, X1, 0)));
}

void test_c2_s_mode_needs_mcounteren_tm(void) {
    counteren(0, COUNTEREN_TM);
    TEST_ASSERT_TRUE(traps(PRV_S, csr_insn(CSR_TIME, CSRRS, X1, 0)));
    counteren(COUNTEREN_CY | COUNTEREN_IR, COUNTEREN_TM);
    TEST_ASSERT_TRUE(traps(PRV_S, csr_insn(CSR_TIME, CSRRS, X1, 0)));
}

void test_c3_s_mode_does_not_need_scounteren(void) {
    counteren(COUNTEREN_TM, 0);
    TEST_ASSERT_FALSE(traps(PRV_S, csr_insn(CSR_TIME, CSRRS, X1, 0)));
}

void test_c4_u_mode_needs_scounteren_too(void) {
    counteren(COUNTEREN_TM, 0);
    TEST_ASSERT_TRUE(traps(PRV_U, csr_insn(CSR_TIME, CSRRS, X1, 0)));
}

void test_c5_u_mode_needs_mcounteren_too(void) {
    counteren(0, COUNTEREN_TM);
    TEST_ASSERT_TRUE(traps(PRV_U, csr_insn(CSR_TIME, CSRRS, X1, 0)));
}

void test_c6_u_mode_with_both(void) {
    counteren(COUNTEREN_TM, COUNTEREN_TM);
    TEST_ASSERT_FALSE(traps(PRV_U, csr_insn(CSR_TIME, CSRRS, X1, 0)));
}

void test_c7_timeh_is_gated_the_same_way(void) {
    counteren(0, 0);
    TEST_ASSERT_TRUE(traps(PRV_S, csr_insn(CSR_TIMEH, CSRRS, X1, 0)));
    TEST_ASSERT_TRUE(traps(PRV_U, csr_insn(CSR_TIMEH, CSRRS, X1, 0)));
    counteren(COUNTEREN_TM, 0);
    TEST_ASSERT_FALSE(traps(PRV_S, csr_insn(CSR_TIMEH, CSRRS, X1, 0)));
    TEST_ASSERT_TRUE(traps(PRV_U, csr_insn(CSR_TIMEH, CSRRS, X1, 0)));
    counteren(COUNTEREN_TM, COUNTEREN_TM);
    TEST_ASSERT_FALSE(traps(PRV_U, csr_insn(CSR_TIMEH, CSRRS, X1, 0)));
}

void test_c8_cycle_and_instret_gating_is_unchanged(void) {
    counteren(COUNTEREN_CY, COUNTEREN_CY);
    TEST_ASSERT_FALSE(traps(PRV_U, csr_insn(CSR_CYCLE, CSRRS, X1, 0)));
    TEST_ASSERT_TRUE(traps(PRV_U, csr_insn(CSR_INSTRET, CSRRS, X1, 0)));
    counteren(COUNTEREN_IR, 0);
    TEST_ASSERT_FALSE(traps(PRV_S, csr_insn(CSR_INSTRET, CSRRS, X1, 0)));
    TEST_ASSERT_TRUE(traps(PRV_U, csr_insn(CSR_INSTRET, CSRRS, X1, 0)));
    TEST_ASSERT_TRUE(traps(PRV_S, csr_insn(CSR_CYCLE, CSRRS, X1, 0)));
}

void test_c9_tm_is_independent_of_cy_and_ir(void) {
    counteren(COUNTEREN_TM, COUNTEREN_TM);
    TEST_ASSERT_FALSE(traps(PRV_S, csr_insn(CSR_TIME, CSRRS, X1, 0)));
    TEST_ASSERT_TRUE(traps(PRV_S, csr_insn(CSR_CYCLE, CSRRS, X1, 0)));
    counteren(COUNTEREN_CY | COUNTEREN_IR, COUNTEREN_CY | COUNTEREN_IR);
    TEST_ASSERT_FALSE(traps(PRV_S, csr_insn(CSR_CYCLE, CSRRS, X1, 0)));
    TEST_ASSERT_TRUE(traps(PRV_S, csr_insn(CSR_TIME, CSRRS, X1, 0)));
}

/* ---- D. Read-only ------------------------------------------------------------------------------------ */

void test_d1_csrrw_is_illegal(void) {
    cpu_write_reg(&cpu, X5, 0);
    TEST_ASSERT_TRUE(traps(PRV_M, csr_insn(CSR_TIME, CSRRW, 0, X5)));
    TEST_ASSERT_TRUE(traps(PRV_M, csr_insn(CSR_TIMEH, CSRRW, 0, X5)));
}

void test_d2_csrrs_and_csrrc_with_a_source_are_illegal(void) {
    cpu_write_reg(&cpu, X5, 1);
    TEST_ASSERT_TRUE(traps(PRV_M, csr_insn(CSR_TIME, CSRRS, 0, X5)));
    TEST_ASSERT_TRUE(traps(PRV_M, csr_insn(CSR_TIME, CSRRC, 0, X5)));
}

void test_d3_the_immediate_forms(void) {
    TEST_ASSERT_TRUE(traps(PRV_M, csr_insn(CSR_TIME, CSRRWI, 0, 1)));
    TEST_ASSERT_TRUE(traps(PRV_M, csr_insn(CSR_TIME, CSRRSI, 0, 1)));
}

void test_d4_a_read_with_no_write_is_allowed(void) {
    cpu.clint.mtime = 42;
    TEST_ASSERT_FALSE(traps(PRV_M, csr_insn(CSR_TIME, CSRRS, X1, 0)));
    TEST_ASSERT_EQUAL_UINT32(42, cpu_read_reg(&cpu, X1));
    TEST_ASSERT_FALSE(traps(PRV_M, csr_insn(CSR_TIME, CSRRSI, X1, 0)));    // uimm = 0 does not write
}

void test_d5_csrrc_with_x0_on_timeh(void) {
    cpu.clint.mtime = 0x700000000ull;
    TEST_ASSERT_FALSE(traps(PRV_M, csr_insn(CSR_TIMEH, CSRRC, X1, 0)));
    TEST_ASSERT_EQUAL_UINT32(7, cpu_read_reg(&cpu, X1));
}

void test_d6_an_illegal_write_leaves_mtime_alone(void) {
    cpu.clint.mtime = 1000;
    cpu_write_reg(&cpu, X5, 5);
    TEST_ASSERT_TRUE(traps(PRV_M, csr_insn(CSR_TIME, CSRRW, 0, X5)));
    TEST_ASSERT_EQUAL_UINT64(1000, cpu.clint.mtime);
    cpu_write_csr(&cpu, CSR_TIME, 5);                                       // and so does a direct write
    TEST_ASSERT_EQUAL_UINT64(1000, cpu.clint.mtime);
}

/* ---- E. Existence ------------------------------------------------------------------------------------- */

void test_e1_hpm_counters_still_do_not_exist(void) {
    TEST_ASSERT_TRUE(traps(PRV_M, csr_insn(0xC03, CSRRS, X1, 0)));         // hpmcounter3
    TEST_ASSERT_TRUE(traps(PRV_M, csr_insn(0xC83, CSRRS, X1, 0)));         // hpmcounter3h
    TEST_ASSERT_TRUE(traps(PRV_M, csr_insn(0xC1F, CSRRS, X1, 0)));         // hpmcounter31
    TEST_ASSERT_FALSE(traps(PRV_M, csr_insn(CSR_TIME, CSRRS, X1, 0)));
    TEST_ASSERT_FALSE(traps(PRV_M, csr_insn(CSR_TIMEH, CSRRS, X1, 0)));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_b1_the_two_halves);
    RUN_TEST(test_b2_a_write_through_the_clint_shows_at_once);
    RUN_TEST(test_b3_crossing_32_bits);
    RUN_TEST(test_b4_time_follows_cpu_steps);
    RUN_TEST(test_b5_time_and_cycle);
    RUN_TEST(test_c1_m_mode_is_never_gated);
    RUN_TEST(test_c2_s_mode_needs_mcounteren_tm);
    RUN_TEST(test_c3_s_mode_does_not_need_scounteren);
    RUN_TEST(test_c4_u_mode_needs_scounteren_too);
    RUN_TEST(test_c5_u_mode_needs_mcounteren_too);
    RUN_TEST(test_c6_u_mode_with_both);
    RUN_TEST(test_c7_timeh_is_gated_the_same_way);
    RUN_TEST(test_c8_cycle_and_instret_gating_is_unchanged);
    RUN_TEST(test_c9_tm_is_independent_of_cy_and_ir);
    RUN_TEST(test_d1_csrrw_is_illegal);
    RUN_TEST(test_d2_csrrs_and_csrrc_with_a_source_are_illegal);
    RUN_TEST(test_d3_the_immediate_forms);
    RUN_TEST(test_d4_a_read_with_no_write_is_allowed);
    RUN_TEST(test_d5_csrrc_with_x0_on_timeh);
    RUN_TEST(test_d6_an_illegal_write_leaves_mtime_alone);
    RUN_TEST(test_e1_hpm_counters_still_do_not_exist);
    return UNITY_END();
}
