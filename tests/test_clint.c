#include <string.h>
#include "unity.h"
#include "cpu_test_util.h"
#include "../src/execute.h"
#include "../src/clint.h"

#define RAM_BASE 0x80000000u
#define MTVEC    (RAM_BASE + 0x100u)
#define PC       (RAM_BASE + 0x400u)

static CPU cpu;
static Memory mem;

// The CLINT is registered with the address of the CPU's own Clint, which stays valid across cpu_reset
void setUp(void) {
    mem_init_at(&mem, RAM_BASE, 0x10000);
    test_cpu_reset_in_mode(&cpu, PRV_M);
    mem_add_device(&mem, clint_device(&cpu.clint));
}

void tearDown(void) {
    mem_free(&mem);
}

static bool rd(uint32_t offset, uint32_t *v) {
    return mem_bus_read(&mem, CLINT_BASE + offset, 4, v);
}

static bool wr(uint32_t offset, uint32_t v) {
    return mem_bus_write(&mem, CLINT_BASE + offset, 4, v);
}

static uint32_t reg(uint32_t offset) {
    uint32_t v = 0xDEADBEEF;
    TEST_ASSERT_TRUE(rd(offset, &v));
    return v;
}

static void tick(unsigned n) {
    while (n--) {
        clint_tick(&cpu.clint);
    }
}

/* ---- A. The registers -------------------------------------------------------------------------- */

void test_a1_reset_values(void) {
    TEST_ASSERT_EQUAL_HEX32(0, reg(CLINT_MTIME));
    TEST_ASSERT_EQUAL_HEX32(0, reg(CLINT_MTIME + 4));
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFF, reg(CLINT_MTIMECMP));
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFF, reg(CLINT_MTIMECMP + 4));
    TEST_ASSERT_EQUAL_HEX32(0, reg(CLINT_MSIP));
}

void test_a2_mtimecmp_halves(void) {
    TEST_ASSERT_TRUE(wr(CLINT_MTIMECMP, 0x11111111));
    TEST_ASSERT_EQUAL_HEX32(0x11111111, reg(CLINT_MTIMECMP));
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFF, reg(CLINT_MTIMECMP + 4));           // the high half is untouched
    TEST_ASSERT_TRUE(wr(CLINT_MTIMECMP + 4, 0x22222222));
    TEST_ASSERT_EQUAL_HEX32(0x11111111, reg(CLINT_MTIMECMP));
    TEST_ASSERT_EQUAL_HEX32(0x22222222, reg(CLINT_MTIMECMP + 4));
}

void test_a3_mtime_halves(void) {
    TEST_ASSERT_TRUE(wr(CLINT_MTIME + 4, 0x33333333));
    TEST_ASSERT_EQUAL_HEX32(0, reg(CLINT_MTIME));
    TEST_ASSERT_EQUAL_HEX32(0x33333333, reg(CLINT_MTIME + 4));
    TEST_ASSERT_TRUE(wr(CLINT_MTIME, 0x44444444));
    TEST_ASSERT_EQUAL_HEX32(0x44444444, reg(CLINT_MTIME));
    TEST_ASSERT_EQUAL_HEX32(0x33333333, reg(CLINT_MTIME + 4));
}

void test_a4_msip_is_bit_0_only(void) {
    TEST_ASSERT_TRUE(wr(CLINT_MSIP, 0xFFFFFFFF));
    TEST_ASSERT_EQUAL_HEX32(1, reg(CLINT_MSIP));
    TEST_ASSERT_TRUE(clint_software_pending(&cpu.clint));
    TEST_ASSERT_TRUE(wr(CLINT_MSIP, 2));
    TEST_ASSERT_EQUAL_HEX32(0, reg(CLINT_MSIP));
    TEST_ASSERT_FALSE(clint_software_pending(&cpu.clint));
    TEST_ASSERT_TRUE(wr(CLINT_MSIP, 1));
    TEST_ASSERT_EQUAL_HEX32(1, reg(CLINT_MSIP));
}

void test_a5_gaps_fault_and_change_nothing(void) {
    const uint32_t gaps[] = {0x4, 0x8, 0x3FFC, 0x4008, 0x400C, 0xBFF0, 0xC000, 0xFFFC};
    for (unsigned i = 0; i < sizeof gaps / sizeof gaps[0]; i++) {
        Clint before = cpu.clint;
        uint32_t v;
        TEST_ASSERT_FALSE_MESSAGE(rd(gaps[i], &v), "read of a gap");
        TEST_ASSERT_FALSE_MESSAGE(wr(gaps[i], 0xFFFFFFFF), "write to a gap");
        TEST_ASSERT_EQUAL_MEMORY(&before, &cpu.clint, sizeof before);
    }
}

void test_a5b_only_five_words_exist(void) {
    for (uint32_t off = 0; off < CLINT_SIZE; off += 4) {
        bool real = off == CLINT_MSIP || off == CLINT_MTIMECMP || off == CLINT_MTIMECMP + 4 ||
                    off == CLINT_MTIME || off == CLINT_MTIME + 4;
        uint32_t v;
        TEST_ASSERT_EQUAL_MESSAGE(real, rd(off, &v), "read");
        TEST_ASSERT_EQUAL_MESSAGE(real, wr(off, 0), "write");
    }
}

void test_a6_only_32_bit_accesses(void) {
    const uint32_t regs[] = {CLINT_MSIP, CLINT_MTIMECMP, CLINT_MTIMECMP + 4, CLINT_MTIME, CLINT_MTIME + 4};
    for (unsigned i = 0; i < sizeof regs / sizeof regs[0]; i++) {
        uint32_t v;
        TEST_ASSERT_FALSE(mem_bus_read(&mem, CLINT_BASE + regs[i], 1, &v));
        TEST_ASSERT_FALSE(mem_bus_read(&mem, CLINT_BASE + regs[i], 2, &v));
        TEST_ASSERT_FALSE(mem_bus_write(&mem, CLINT_BASE + regs[i], 1, 0));
        TEST_ASSERT_FALSE(mem_bus_write(&mem, CLINT_BASE + regs[i], 2, 0));
    }
    uint32_t v;
    TEST_ASSERT_FALSE(mem_bus_read(&mem, CLINT_BASE + CLINT_MSIP + 2, 4, &v));   // misaligned word
}

void test_a7_registers_are_independent(void) {
    TEST_ASSERT_TRUE(wr(CLINT_MTIMECMP, 5));
    TEST_ASSERT_EQUAL_HEX32(0, reg(CLINT_MTIME));
    TEST_ASSERT_EQUAL_HEX32(0, reg(CLINT_MSIP));
    TEST_ASSERT_TRUE(wr(CLINT_MSIP, 1));
    TEST_ASSERT_EQUAL_HEX32(5, reg(CLINT_MTIMECMP));
    TEST_ASSERT_EQUAL_HEX32(0, reg(CLINT_MTIME));
}

/* ---- B. Time ----------------------------------------------------------------------------------- */

void test_b1_one_tick_per_ten_cycles(void) {
    tick(9);
    TEST_ASSERT_EQUAL_UINT64(0, cpu.clint.mtime);
    tick(1);
    TEST_ASSERT_EQUAL_UINT64(1, cpu.clint.mtime);
    tick(90);
    TEST_ASSERT_EQUAL_UINT64(10, cpu.clint.mtime);
}

static void step_nops(unsigned n) {
    for (uint32_t i = 0; i < 0x40; i += 4) {
        mem_write32(&mem, PC + i, 0x00000013);                  // NOP
    }
    cpu.pc = PC;
    while (n--) {
        cpu_step(&cpu, &mem);
    }
}

void test_b2_the_cpu_ticks_it_every_step(void) {
    step_nops(10);
    TEST_ASSERT_EQUAL_UINT64(1, cpu.clint.mtime);
}

void test_b2b_a_trapping_step_ticks_too(void) {
    cpu_write_csr(&cpu, CSR_MTVEC, 0x10000000);                 // unmapped: every fetch faults
    cpu.pc = 0x10000000;
    for (int i = 0; i < 10; i++) {
        cpu_step(&cpu, &mem);
    }
    TEST_ASSERT_EQUAL_UINT64(1, cpu.clint.mtime);
}

void test_b3_writing_mtime_keeps_the_phase(void) {
    tick(5);
    TEST_ASSERT_TRUE(wr(CLINT_MTIME, 100));
    tick(4);
    TEST_ASSERT_EQUAL_UINT64(100, cpu.clint.mtime);
    tick(1);
    TEST_ASSERT_EQUAL_UINT64(101, cpu.clint.mtime);             // five more cycles in all
}

void test_b4_mtime_wraps_and_carries(void) {
    cpu.clint.mtime = UINT64_MAX;
    tick(10);
    TEST_ASSERT_EQUAL_UINT64(0, cpu.clint.mtime);
    cpu.clint.mtime = 0xFFFFFFFFull;
    tick(10);
    TEST_ASSERT_EQUAL_HEX32(0, reg(CLINT_MTIME));
    TEST_ASSERT_EQUAL_HEX32(1, reg(CLINT_MTIME + 4));
}

void test_b5_mcycle_does_not_move_mtime(void) {
    cpu_write_csr(&cpu, CSR_MCYCLE, 0xFFFFFF00u);
    step_nops(10);
    TEST_ASSERT_EQUAL_UINT64(1, cpu.clint.mtime);
}

void test_b5b_an_mcycle_write_still_ticks_it(void) {
    mem_write32(&mem, PC, 0xB0009073);                          // csrw mcycle, x1: this step suppresses mcycle's own increment
    for (uint32_t i = 4; i < 0x40; i += 4) {
        mem_write32(&mem, PC + i, 0x00000013);
    }
    cpu_write_reg(&cpu, 1, 0x1000);
    cpu.pc = PC;
    for (int i = 0; i < 10; i++) {
        cpu_step(&cpu, &mem);
    }
    TEST_ASSERT_EQUAL_UINT64(1, cpu.clint.mtime);               // but mtime followed all ten cycles
}

void test_b6_two_runs_agree(void) {
    step_nops(37);
    uint64_t first = cpu.clint.mtime;
    uint32_t first_phase = cpu.clint.phase;
    test_cpu_reset_in_mode(&cpu, PRV_M);
    step_nops(37);
    TEST_ASSERT_EQUAL_UINT64(first, cpu.clint.mtime);
    TEST_ASSERT_EQUAL_UINT32(first_phase, cpu.clint.phase);
    TEST_ASSERT_EQUAL_UINT64(3, first);
}

/* ---- C. Pending logic -------------------------------------------------------------------------- */

void test_c1_pending_is_greater_or_equal(void) {
    cpu.clint.mtime = 100;
    cpu.clint.mtimecmp = 100;
    TEST_ASSERT_TRUE(clint_timer_pending(&cpu.clint));
    cpu.clint.mtimecmp = 101;
    TEST_ASSERT_FALSE(clint_timer_pending(&cpu.clint));
    cpu.clint.mtimecmp = 99;
    TEST_ASSERT_TRUE(clint_timer_pending(&cpu.clint));
}

void test_c2_the_comparison_is_unsigned(void) {
    cpu.clint.mtime = 0x8000000000000000ull;
    cpu.clint.mtimecmp = 1;
    TEST_ASSERT_TRUE(clint_timer_pending(&cpu.clint));
}

void test_c3_writing_mtimecmp_clears_it(void) {
    cpu.clint.mtime = 50;
    TEST_ASSERT_TRUE(wr(CLINT_MTIMECMP, 0));
    TEST_ASSERT_TRUE(wr(CLINT_MTIMECMP + 4, 0));
    TEST_ASSERT_TRUE(clint_timer_pending(&cpu.clint));
    TEST_ASSERT_TRUE(wr(CLINT_MTIMECMP, 51));
    TEST_ASSERT_FALSE(clint_timer_pending(&cpu.clint));
}

void test_c4_the_manuals_rv32_sequence_never_fires(void) {
    cpu.clint.mtime = 100;
    TEST_ASSERT_TRUE(wr(CLINT_MTIMECMP + 4, 0));
    TEST_ASSERT_TRUE(wr(CLINT_MTIMECMP, 200));                  // the old comparand: 200
    TEST_ASSERT_FALSE(clint_timer_pending(&cpu.clint));
    TEST_ASSERT_TRUE(wr(CLINT_MTIMECMP, 0xFFFFFFFF));           // store -1 to the low half first
    TEST_ASSERT_FALSE(clint_timer_pending(&cpu.clint));
    TEST_ASSERT_TRUE(wr(CLINT_MTIMECMP + 4, 0));                // then the high half of the new value
    TEST_ASSERT_FALSE(clint_timer_pending(&cpu.clint));
    TEST_ASSERT_TRUE(wr(CLINT_MTIMECMP, 300));                  // then the low half of the new value
    TEST_ASSERT_FALSE(clint_timer_pending(&cpu.clint));
    TEST_ASSERT_EQUAL_UINT64(300, cpu.clint.mtimecmp);
}

void test_c5_nothing_pending_after_reset(void) {
    TEST_ASSERT_FALSE(clint_timer_pending(&cpu.clint));
    TEST_ASSERT_FALSE(clint_software_pending(&cpu.clint));
}

/* ---- D. Through the CPU ------------------------------------------------------------------------ */

#define RD   1
#define ADDR 2
#define SRC  3

static uint32_t enc_load(uint32_t funct3)  { return (ADDR << 15) | (funct3 << 12) | (RD << 7) | 0x03; }
static uint32_t enc_store(uint32_t funct3) { return (SRC << 20) | (ADDR << 15) | (funct3 << 12) | 0x23; }
#define LB  enc_load(0)
#define LH  enc_load(1)
#define LW  enc_load(2)
#define SB  enc_store(0)
#define SH  enc_store(1)
#define SW  enc_store(2)
#define AMOADD_W 0x003120AFu
#define LR_W     0x100120AFu
#define SC_W     0x183120AFu

static void run(uint32_t raw, uint32_t addr) {
    cpu_write_csr(&cpu, CSR_MTVEC, MTVEC);
    cpu_write_reg(&cpu, ADDR, addr);
    cpu.pc = PC;
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);
}

static void expect_trap(uint32_t cause, uint32_t tval) {
    TEST_ASSERT_EQUAL_HEX32(MTVEC, cpu.pc);
    TEST_ASSERT_EQUAL_UINT32(cause, cpu_read_csr(&cpu, CSR_MCAUSE));
    TEST_ASSERT_EQUAL_HEX32(tval, cpu_read_csr(&cpu, CSR_MTVAL));
}

void test_d1_lw_reads_mtime(void) {
    cpu.clint.mtime = 0x0000000500000007ull;
    run(LW, CLINT_BASE + CLINT_MTIME);
    TEST_ASSERT_EQUAL_HEX32(PC + 4, cpu.pc);
    TEST_ASSERT_EQUAL_HEX32(7, cpu_read_reg(&cpu, RD));
    run(LW, CLINT_BASE + CLINT_MTIME + 4);
    TEST_ASSERT_EQUAL_HEX32(5, cpu_read_reg(&cpu, RD));
}

void test_d2_sw_sets_mtimecmp(void) {
    cpu.clint.mtime = 100;
    cpu_write_reg(&cpu, SRC, 50);
    run(SW, CLINT_BASE + CLINT_MTIMECMP);
    cpu_write_reg(&cpu, SRC, 0);
    run(SW, CLINT_BASE + CLINT_MTIMECMP + 4);
    TEST_ASSERT_TRUE(clint_timer_pending(&cpu.clint));
    cpu_write_reg(&cpu, SRC, 1);
    run(SW, CLINT_BASE + CLINT_MTIMECMP + 4);
    TEST_ASSERT_FALSE(clint_timer_pending(&cpu.clint));
    TEST_ASSERT_EQUAL_UINT64(0x100000032ull, cpu.clint.mtimecmp);
}

void test_d3_narrow_accesses_and_gaps_fault(void) {
    run(LB, CLINT_BASE + CLINT_MSIP);
    expect_trap(5, CLINT_BASE + CLINT_MSIP);
    run(LH, CLINT_BASE + CLINT_MTIME);
    expect_trap(5, CLINT_BASE + CLINT_MTIME);
    run(SB, CLINT_BASE + CLINT_MSIP);
    expect_trap(7, CLINT_BASE + CLINT_MSIP);
    run(SH, CLINT_BASE + CLINT_MTIMECMP);
    expect_trap(7, CLINT_BASE + CLINT_MTIMECMP);
    run(LW, CLINT_BASE + 0x4008);
    expect_trap(5, CLINT_BASE + 0x4008);
}

void test_d4_fetch_from_the_clint(void) {
    cpu_write_csr(&cpu, CSR_MTVEC, MTVEC);
    cpu.pc = CLINT_BASE;
    cpu_step(&cpu, &mem);
    expect_trap(1, CLINT_BASE);
}

void test_d5_atomics_on_the_clint(void) {
    run(AMOADD_W, CLINT_BASE + CLINT_MSIP);
    expect_trap(7, CLINT_BASE + CLINT_MSIP);
    run(LR_W, CLINT_BASE + CLINT_MSIP);
    expect_trap(5, CLINT_BASE + CLINT_MSIP);
    run(SC_W, CLINT_BASE + CLINT_MSIP);
    expect_trap(7, CLINT_BASE + CLINT_MSIP);
    TEST_ASSERT_EQUAL_HEX32(0, reg(CLINT_MSIP));
}

void test_d7_reset_after_running(void) {
    cpu.clint.mtime = 12345;
    cpu.clint.mtimecmp = 7;
    cpu.clint.msip = 1;
    cpu.clint.phase = 4;
    cpu_reset(&cpu);                                            // the device keeps pointing at cpu.clint
    TEST_ASSERT_EQUAL_HEX32(0, reg(CLINT_MTIME));
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFF, reg(CLINT_MTIMECMP));
    TEST_ASSERT_EQUAL_HEX32(0, reg(CLINT_MSIP));
    TEST_ASSERT_EQUAL_UINT32(0, cpu.clint.phase);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_a1_reset_values);
    RUN_TEST(test_a2_mtimecmp_halves);
    RUN_TEST(test_a3_mtime_halves);
    RUN_TEST(test_a4_msip_is_bit_0_only);
    RUN_TEST(test_a5_gaps_fault_and_change_nothing);
    RUN_TEST(test_a5b_only_five_words_exist);
    RUN_TEST(test_a6_only_32_bit_accesses);
    RUN_TEST(test_a7_registers_are_independent);
    RUN_TEST(test_b1_one_tick_per_ten_cycles);
    RUN_TEST(test_b2_the_cpu_ticks_it_every_step);
    RUN_TEST(test_b2b_a_trapping_step_ticks_too);
    RUN_TEST(test_b3_writing_mtime_keeps_the_phase);
    RUN_TEST(test_b4_mtime_wraps_and_carries);
    RUN_TEST(test_b5_mcycle_does_not_move_mtime);
    RUN_TEST(test_b5b_an_mcycle_write_still_ticks_it);
    RUN_TEST(test_b6_two_runs_agree);
    RUN_TEST(test_c1_pending_is_greater_or_equal);
    RUN_TEST(test_c2_the_comparison_is_unsigned);
    RUN_TEST(test_c3_writing_mtimecmp_clears_it);
    RUN_TEST(test_c4_the_manuals_rv32_sequence_never_fires);
    RUN_TEST(test_c5_nothing_pending_after_reset);
    RUN_TEST(test_d1_lw_reads_mtime);
    RUN_TEST(test_d2_sw_sets_mtimecmp);
    RUN_TEST(test_d3_narrow_accesses_and_gaps_fault);
    RUN_TEST(test_d4_fetch_from_the_clint);
    RUN_TEST(test_d5_atomics_on_the_clint);
    RUN_TEST(test_d7_reset_after_running);
    return UNITY_END();
}
