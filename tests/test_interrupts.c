#include "unity.h"
#include "cpu_test_util.h"
#include "../src/execute.h"

#define RAM_BASE 0x80000000u
#define MTVEC    (RAM_BASE + 0x100u)
#define STVEC    (RAM_BASE + 0x200u)
#define PC       (RAM_BASE + 0x400u)
#define SENTINEL 0x11111111u

#define NOP         0x00000013u
#define ADDI_X1_5   0x00500093u            // addi x1, x0, 5: a run of it sets x1 to 5, so x1 shows whether it ran
#define MRET        0x30200073u
#define SRET        0x10200073u
#define CSRRSI_MSTATUS_MIE 0x30046073u     // csrrsi x0, mstatus, 8

static CPU cpu;
static Memory mem;
static uint32_t before_mcause, before_mepc, before_mtval;       // the machine trap CSRs when a step began

static void snapshot_machine_trap_csrs(void) {
    before_mcause = cpu_read_csr(&cpu, CSR_MCAUSE);
    before_mepc = cpu_read_csr(&cpu, CSR_MEPC);
    before_mtval = cpu_read_csr(&cpu, CSR_MTVAL);
}

static void put(uint32_t addr, uint32_t word) {
    mem_write32(&mem, addr, word);
}

// A CPU in the given mode, NOPs everywhere, ADDI x1, 5 at PC, and the handlers at mtvec and stvec
static void setup(PrivMode mode) {
    test_cpu_reset_in_mode(&cpu, mode);
    for (uint32_t a = RAM_BASE; a < RAM_BASE + 0x1000; a += 4) {
        put(a, NOP);
    }
    put(PC, ADDI_X1_5);
    cpu_write_csr(&cpu, CSR_MTVEC, MTVEC);
    cpu_write_csr(&cpu, CSR_STVEC, STVEC);
    cpu_write_reg(&cpu, 1, SENTINEL);
    cpu.pc = PC;
    snapshot_machine_trap_csrs();
}

// Go back to the interrupted spot, in a given mode, to see which interrupt comes next
static void again(PrivMode mode) {
    cpu.privilege_mode = mode;
    cpu.pc = PC;
    cpu_write_reg(&cpu, 1, SENTINEL);
    snapshot_machine_trap_csrs();
}

void setUp(void) {
    mem_init_at(&mem, RAM_BASE, 0x2000);
}

void tearDown(void) {
    mem_free(&mem);
}

static void timer_on(void)  { cpu.clint.mtimecmp = 0; }
static void timer_off(void) { cpu.clint.mtimecmp = UINT64_MAX; }

static void step(void) {
    cpu_step(&cpu, &mem);
}

static uint32_t mstatus(void) {
    return cpu_read_csr(&cpu, CSR_MSTATUS);
}

static void expect_not_taken(void) {
    TEST_ASSERT_EQUAL_HEX32(PC + 4, cpu.pc);
    TEST_ASSERT_EQUAL_UINT32(5, cpu_read_reg(&cpu, 1));
}

// Taken to M-mode: the instruction at PC did not run, and the trap CSRs are set
static void expect_taken_m(uint32_t code, PrivMode from) {
    TEST_ASSERT_EQUAL_HEX32(MTVEC, cpu.pc);
    TEST_ASSERT_EQUAL_HEX32(0x80000000u | code, cpu_read_csr(&cpu, CSR_MCAUSE));
    TEST_ASSERT_EQUAL_HEX32(PC, cpu_read_csr(&cpu, CSR_MEPC));
    TEST_ASSERT_EQUAL_HEX32(0, cpu_read_csr(&cpu, CSR_MTVAL));
    TEST_ASSERT_EQUAL_INT(PRV_M, cpu.privilege_mode);
    TEST_ASSERT_EQUAL_UINT32(from, (mstatus() & MSTATUS_MPP) >> 11);
    TEST_ASSERT_EQUAL_HEX32(SENTINEL, cpu_read_reg(&cpu, 1));
}

// Taken to S-mode, and the machine trap CSRs were left alone
static void expect_taken_s(uint32_t code, PrivMode from) {
    TEST_ASSERT_EQUAL_HEX32(STVEC, cpu.pc);
    TEST_ASSERT_EQUAL_HEX32(0x80000000u | code, cpu_read_csr(&cpu, CSR_SCAUSE));
    TEST_ASSERT_EQUAL_HEX32(PC, cpu_read_csr(&cpu, CSR_SEPC));
    TEST_ASSERT_EQUAL_HEX32(0, cpu_read_csr(&cpu, CSR_STVAL));
    TEST_ASSERT_EQUAL_INT(PRV_S, cpu.privilege_mode);
    TEST_ASSERT_EQUAL_UINT32(from == PRV_U ? 0 : 1, (mstatus() & MSTATUS_SPP) >> 8);
    TEST_ASSERT_EQUAL_HEX32(before_mcause, cpu_read_csr(&cpu, CSR_MCAUSE));
    TEST_ASSERT_EQUAL_HEX32(before_mepc, cpu_read_csr(&cpu, CSR_MEPC));
    TEST_ASSERT_EQUAL_HEX32(before_mtval, cpu_read_csr(&cpu, CSR_MTVAL));
    TEST_ASSERT_EQUAL_HEX32(SENTINEL, cpu_read_reg(&cpu, 1));
}

/* ---- A. Interrupts to M-mode ---------------------------------------------------------------------- */

void test_a1_machine_timer_interrupt(void) {
    setup(PRV_M);
    cpu_write_csr(&cpu, CSR_MSTATUS, MSTATUS_MIE);
    cpu_write_csr(&cpu, CSR_MIE, INT_MTI);
    timer_on();
    step();
    expect_taken_m(7, PRV_M);
    TEST_ASSERT_TRUE(mstatus() & MSTATUS_MPIE);                 // MPIE = the old MIE
    TEST_ASSERT_FALSE(mstatus() & MSTATUS_MIE);
}

void test_a2_mie_clear_in_m_mode(void) {
    setup(PRV_M);
    cpu_write_csr(&cpu, CSR_MIE, INT_MTI);
    timer_on();
    step();
    expect_not_taken();
}

void test_a3_enable_bit_clear(void) {
    setup(PRV_M);
    cpu_write_csr(&cpu, CSR_MSTATUS, MSTATUS_MIE);
    timer_on();
    step();
    expect_not_taken();
    TEST_ASSERT_TRUE(cpu_read_csr(&cpu, CSR_MIP) & INT_MTI);    // still pending
}

void test_a4_nothing_pending(void) {
    setup(PRV_M);
    cpu_write_csr(&cpu, CSR_MSTATUS, MSTATUS_MIE);
    cpu_write_csr(&cpu, CSR_MIE, INT_MTI);
    step();
    expect_not_taken();
}

void test_a5_s_mode_ignores_mie(void) {
    setup(PRV_S);
    cpu_write_csr(&cpu, CSR_MIE, INT_MTI);
    timer_on();
    step();
    expect_taken_m(7, PRV_S);
}

void test_a6_u_mode_ignores_mie(void) {
    setup(PRV_U);
    cpu_write_csr(&cpu, CSR_MIE, INT_MTI);
    timer_on();
    step();
    expect_taken_m(7, PRV_U);
}

void test_a7_machine_software_interrupt(void) {
    setup(PRV_M);
    cpu_write_csr(&cpu, CSR_MSTATUS, MSTATUS_MIE);
    cpu_write_csr(&cpu, CSR_MIE, INT_MSI);
    cpu.clint.msip = 1;
    step();
    expect_taken_m(3, PRV_M);
}

void test_a8_meie_with_nothing_driving_meip(void) {
    setup(PRV_M);
    cpu_write_csr(&cpu, CSR_MSTATUS, MSTATUS_MIE);
    cpu_write_csr(&cpu, CSR_MIE, INT_MEI);
    step();
    expect_not_taken();
}

void test_a9_an_undelegated_supervisor_interrupt_goes_to_m(void) {
    setup(PRV_S);
    cpu_write_csr(&cpu, CSR_MIE, INT_SSI);
    cpu_write_csr(&cpu, CSR_MIP, INT_SSI);
    step();
    expect_taken_m(1, PRV_S);
}

/* ---- B. Priority ----------------------------------------------------------------------------------- */

void test_b1_software_before_timer(void) {
    setup(PRV_U);
    cpu_write_csr(&cpu, CSR_MIE, INT_MSI | INT_MTI);
    cpu.clint.msip = 1;
    timer_on();
    step();
    expect_taken_m(3, PRV_U);
    cpu.clint.msip = 0;
    again(PRV_U);
    step();
    expect_taken_m(7, PRV_U);
}

void test_b2_supervisor_order(void) {
    setup(PRV_U);
    cpu_write_csr(&cpu, CSR_MIDELEG, 0x222);
    cpu_write_csr(&cpu, CSR_MIE, 0x222);
    cpu_write_csr(&cpu, CSR_MIP, 0x222);
    step();
    expect_taken_s(9, PRV_U);                                   // SEI
    cpu_write_csr(&cpu, CSR_MIP, 0x022);
    again(PRV_U);
    step();
    expect_taken_s(1, PRV_U);                                   // SSI
    cpu_write_csr(&cpu, CSR_MIP, 0x020);
    again(PRV_U);
    step();
    expect_taken_s(5, PRV_U);                                   // STI
}

void test_b3_machine_destination_beats_supervisor_destination(void) {
    setup(PRV_U);
    cpu_write_csr(&cpu, CSR_MIDELEG, INT_STI);
    cpu_write_csr(&cpu, CSR_MIE, INT_MTI | INT_STI);
    cpu_write_csr(&cpu, CSR_MIP, INT_STI);
    timer_on();
    step();
    expect_taken_m(7, PRV_U);
    timer_off();
    again(PRV_U);
    step();
    expect_taken_s(5, PRV_U);
}

void test_b4_timer_before_an_undelegated_supervisor_external(void) {
    setup(PRV_U);
    cpu_write_csr(&cpu, CSR_MIE, INT_MTI | INT_SEI);
    cpu_write_csr(&cpu, CSR_MIP, INT_SEI);
    timer_on();
    step();
    expect_taken_m(7, PRV_U);
    timer_off();
    again(PRV_U);
    step();
    expect_taken_m(9, PRV_U);
}

/* ---- C. Interrupts to S-mode (delegated) ------------------------------------------------------------ */

static void delegate_stimer(void) {
    cpu_write_csr(&cpu, CSR_MIDELEG, INT_STI);
    cpu_write_csr(&cpu, CSR_MIE, INT_STI);
    cpu_write_csr(&cpu, CSR_MIP, INT_STI);
}

void test_c1_from_u_mode_with_sie_clear(void) {
    setup(PRV_U);
    delegate_stimer();
    step();
    expect_taken_s(5, PRV_U);
    TEST_ASSERT_FALSE(mstatus() & MSTATUS_SPIE);                // SPIE = the old SIE, which was 0
    TEST_ASSERT_FALSE(mstatus() & MSTATUS_SIE);
}

void test_c2_from_u_mode_with_sie_set(void) {
    setup(PRV_U);
    delegate_stimer();
    cpu_write_csr(&cpu, CSR_MSTATUS, MSTATUS_SIE);
    step();
    expect_taken_s(5, PRV_U);
    TEST_ASSERT_TRUE(mstatus() & MSTATUS_SPIE);
    TEST_ASSERT_FALSE(mstatus() & MSTATUS_SIE);
}

void test_c3_from_s_mode_with_sie_set(void) {
    setup(PRV_S);
    delegate_stimer();
    cpu_write_csr(&cpu, CSR_MSTATUS, MSTATUS_SIE);
    step();
    expect_taken_s(5, PRV_S);
}

void test_c4_from_s_mode_with_sie_clear(void) {
    setup(PRV_S);
    delegate_stimer();
    step();
    expect_not_taken();
}

void test_c5_never_in_m_mode(void) {
    setup(PRV_M);
    delegate_stimer();
    cpu_write_csr(&cpu, CSR_MSTATUS, MSTATUS_MIE | MSTATUS_SIE);
    step();
    expect_not_taken();
}

void test_c6_enable_bit_clear(void) {
    setup(PRV_S);
    delegate_stimer();
    cpu_write_csr(&cpu, CSR_MIE, 0);
    cpu_write_csr(&cpu, CSR_MSTATUS, MSTATUS_SIE);
    step();
    expect_not_taken();
}

void test_c7_the_enable_through_sie(void) {
    setup(PRV_S);
    cpu_write_csr(&cpu, CSR_MIDELEG, INT_STI);
    cpu_write_csr(&cpu, CSR_MIP, INT_STI);
    cpu_write_csr(&cpu, CSR_MSTATUS, MSTATUS_SIE);
    cpu_write_csr(&cpu, CSR_SIE, INT_STI);                      // delegated: this sets mie.STIE
    step();
    expect_taken_s(5, PRV_S);
}

void test_c7b_sie_cannot_enable_what_is_not_delegated(void) {
    setup(PRV_S);
    cpu_write_csr(&cpu, CSR_MIP, INT_STI);
    cpu_write_csr(&cpu, CSR_MSTATUS, MSTATUS_SIE);
    cpu_write_csr(&cpu, CSR_SIE, INT_STI);                      // not delegated: ignored
    TEST_ASSERT_EQUAL_HEX32(0, cpu_read_csr(&cpu, CSR_MIE));
    step();
    expect_not_taken();
}

/* ---- D. Effects and edge cases ------------------------------------------------------------------------ */

void test_d1_the_instruction_does_not_retire(void) {
    setup(PRV_M);
    cpu_write_csr(&cpu, CSR_MSTATUS, MSTATUS_MIE);
    cpu_write_csr(&cpu, CSR_MIE, INT_MTI);
    timer_on();
    uint64_t instret = cpu.minstret, cycle = cpu.mcycle;
    step();
    expect_taken_m(7, PRV_M);
    TEST_ASSERT_EQUAL_UINT64(instret, cpu.minstret);
    TEST_ASSERT_EQUAL_UINT64(cycle + 1, cpu.mcycle);
    TEST_ASSERT_EQUAL_UINT32(1, cpu.clint.phase);               // the cycle still counted for mtime
}

void test_d2_mret_resumes_the_interrupted_instruction(void) {
    setup(PRV_M);
    cpu_write_csr(&cpu, CSR_MSTATUS, MSTATUS_MIE);
    cpu_write_csr(&cpu, CSR_MIE, INT_MTI);
    timer_on();
    step();
    expect_taken_m(7, PRV_M);
    timer_off();                                                // the handler has cleared the source
    put(MTVEC, MRET);
    step();                                                     // mret
    TEST_ASSERT_EQUAL_HEX32(PC, cpu.pc);
    TEST_ASSERT_EQUAL_INT(PRV_M, cpu.privilege_mode);
    TEST_ASSERT_TRUE(mstatus() & MSTATUS_MIE);
    step();                                                     // the interrupted instruction, at last
    expect_not_taken();
}

void test_d3_no_nesting_while_in_the_handler(void) {
    setup(PRV_M);
    cpu_write_csr(&cpu, CSR_MSTATUS, MSTATUS_MIE);
    cpu_write_csr(&cpu, CSR_MIE, INT_MTI);
    timer_on();
    step();
    step();                                                     // the timer is still pending, but MIE is 0
    TEST_ASSERT_EQUAL_HEX32(MTVEC + 4, cpu.pc);
    TEST_ASSERT_EQUAL_HEX32(PC, cpu_read_csr(&cpu, CSR_MEPC));
}

void test_d4_an_interrupt_wins_over_a_fetch_fault(void) {
    setup(PRV_M);
    cpu_write_csr(&cpu, CSR_MSTATUS, MSTATUS_MIE);
    cpu_write_csr(&cpu, CSR_MIE, INT_MTI);
    timer_on();
    cpu.pc = 0x10000000u;                                       // nothing there: a fetch would fault
    step();
    TEST_ASSERT_EQUAL_HEX32(MTVEC, cpu.pc);
    TEST_ASSERT_EQUAL_HEX32(0x80000007u, cpu_read_csr(&cpu, CSR_MCAUSE));
    TEST_ASSERT_EQUAL_HEX32(0x10000000u, cpu_read_csr(&cpu, CSR_MEPC));
}

void test_d5_a_csr_write_that_enables_it_acts_before_the_next_instruction(void) {
    setup(PRV_M);
    cpu_write_csr(&cpu, CSR_MIE, INT_MTI);
    timer_on();
    put(PC, CSRRSI_MSTATUS_MIE);
    put(PC + 4, ADDI_X1_5);
    step();                                                     // sets MIE; MIE was 0, so nothing yet
    TEST_ASSERT_EQUAL_HEX32(PC + 4, cpu.pc);
    TEST_ASSERT_TRUE(mstatus() & MSTATUS_MIE);
    step();
    TEST_ASSERT_EQUAL_HEX32(MTVEC, cpu.pc);
    TEST_ASSERT_EQUAL_HEX32(PC + 4, cpu_read_csr(&cpu, CSR_MEPC));
    TEST_ASSERT_EQUAL_HEX32(SENTINEL, cpu_read_reg(&cpu, 1));   // the ADDI did not run
}

void test_d6_mret_that_reenables_a_pending_interrupt(void) {
    setup(PRV_M);
    cpu_write_csr(&cpu, CSR_MSTATUS, MSTATUS_MIE);
    cpu_write_csr(&cpu, CSR_MIE, INT_MTI);
    timer_on();
    step();
    put(MTVEC, MRET);
    step();                                                     // mret: MIE is back on, the timer is still pending
    TEST_ASSERT_EQUAL_HEX32(PC, cpu.pc);
    step();
    TEST_ASSERT_EQUAL_HEX32(MTVEC, cpu.pc);                     // taken again before the instruction at PC
    TEST_ASSERT_EQUAL_HEX32(PC, cpu_read_csr(&cpu, CSR_MEPC));
    TEST_ASSERT_EQUAL_HEX32(SENTINEL, cpu_read_reg(&cpu, 1));
}

void test_d7_vectored_mtvec_still_goes_to_base(void) {
    setup(PRV_U);
    cpu_write_csr(&cpu, CSR_MTVEC, MTVEC | 1u);                 // MODE = Vectored: not supported, reads back as Direct
    TEST_ASSERT_EQUAL_HEX32(MTVEC, cpu_read_csr(&cpu, CSR_MTVEC));
    cpu_write_csr(&cpu, CSR_MIE, INT_MTI);
    timer_on();
    step();
    TEST_ASSERT_EQUAL_HEX32(MTVEC, cpu.pc);                     // not MTVEC + 4 * 7
}

void test_d8_sret_returns_from_an_interrupt(void) {
    setup(PRV_U);
    delegate_stimer();
    cpu_write_csr(&cpu, CSR_MSTATUS, MSTATUS_SIE);
    step();
    expect_taken_s(5, PRV_U);
    cpu_write_csr(&cpu, CSR_MIP, 0);
    put(STVEC, SRET);
    step();
    TEST_ASSERT_EQUAL_HEX32(PC, cpu.pc);
    TEST_ASSERT_EQUAL_INT(PRV_U, cpu.privilege_mode);
    TEST_ASSERT_TRUE(mstatus() & MSTATUS_SIE);                  // SIE = SPIE
}

void test_d9_medeleg_does_not_apply_to_interrupts(void) {
    setup(PRV_U);
    cpu_write_csr(&cpu, CSR_MEDELEG, 0xFFFFFFFF);
    cpu_write_csr(&cpu, CSR_MIE, INT_MTI);
    timer_on();
    step();
    expect_taken_m(7, PRV_U);
}

void test_d10_a_clear_medeleg_bit_does_not_stop_a_delegated_interrupt(void) {
    setup(PRV_U);
    cpu_write_csr(&cpu, CSR_MEDELEG, 0xB3FF & ~(1u << 5));
    delegate_stimer();
    step();
    expect_taken_s(5, PRV_U);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_a1_machine_timer_interrupt);
    RUN_TEST(test_a2_mie_clear_in_m_mode);
    RUN_TEST(test_a3_enable_bit_clear);
    RUN_TEST(test_a4_nothing_pending);
    RUN_TEST(test_a5_s_mode_ignores_mie);
    RUN_TEST(test_a6_u_mode_ignores_mie);
    RUN_TEST(test_a7_machine_software_interrupt);
    RUN_TEST(test_a8_meie_with_nothing_driving_meip);
    RUN_TEST(test_a9_an_undelegated_supervisor_interrupt_goes_to_m);
    RUN_TEST(test_b1_software_before_timer);
    RUN_TEST(test_b2_supervisor_order);
    RUN_TEST(test_b3_machine_destination_beats_supervisor_destination);
    RUN_TEST(test_b4_timer_before_an_undelegated_supervisor_external);
    RUN_TEST(test_c1_from_u_mode_with_sie_clear);
    RUN_TEST(test_c2_from_u_mode_with_sie_set);
    RUN_TEST(test_c3_from_s_mode_with_sie_set);
    RUN_TEST(test_c4_from_s_mode_with_sie_clear);
    RUN_TEST(test_c5_never_in_m_mode);
    RUN_TEST(test_c6_enable_bit_clear);
    RUN_TEST(test_c7_the_enable_through_sie);
    RUN_TEST(test_c7b_sie_cannot_enable_what_is_not_delegated);
    RUN_TEST(test_d1_the_instruction_does_not_retire);
    RUN_TEST(test_d2_mret_resumes_the_interrupted_instruction);
    RUN_TEST(test_d3_no_nesting_while_in_the_handler);
    RUN_TEST(test_d4_an_interrupt_wins_over_a_fetch_fault);
    RUN_TEST(test_d5_a_csr_write_that_enables_it_acts_before_the_next_instruction);
    RUN_TEST(test_d6_mret_that_reenables_a_pending_interrupt);
    RUN_TEST(test_d7_vectored_mtvec_still_goes_to_base);
    RUN_TEST(test_d8_sret_returns_from_an_interrupt);
    RUN_TEST(test_d9_medeleg_does_not_apply_to_interrupts);
    RUN_TEST(test_d10_a_clear_medeleg_bit_does_not_stop_a_delegated_interrupt);
    return UNITY_END();
}
