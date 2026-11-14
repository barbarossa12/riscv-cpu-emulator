#include "unity.h"
#include "cpu_test_util.h"
#include "../src/execute.h"
#include "../src/clint.h"
#include "../src/uart.h"

// WFI with the CLINT timer (Milestone 6.6). It may simply return (machine.adoc:2705); here it also lets idle time
// pass by jumping mtime to mtimecmp when MTIE is set and no locally enabled interrupt is already pending.

#define RAM_BASE 0x80000000u
#define MTVEC    (RAM_BASE + 0x100u)
#define PC       (RAM_BASE + 0x400u)

#define WFI  0x10500073u
#define NOP  0x00000013u
#define MRET 0x30200073u

static CPU cpu;
static Memory mem;

void setUp(void) {
    mem_init_at(&mem, RAM_BASE, 0x2000);
}

void tearDown(void) {
    mem_free(&mem);
}

// A CPU in a mode with the CLINT registered, a WFI at PC and a NOP after it. mtime starts at 100.
static void setup(PrivMode mode) {
    mem.device_count = 0;
    test_cpu_reset_in_mode(&cpu, mode);
    mem_add_device(&mem, clint_device(&cpu.clint));
    for (uint32_t a = RAM_BASE; a < RAM_BASE + 0x1000; a += 4) {
        mem_write32(&mem, a, NOP);
    }
    mem_write32(&mem, PC, WFI);
    mem_write32(&mem, MTVEC, MRET);
    cpu_write_csr(&cpu, CSR_MTVEC, MTVEC);
    cpu.clint.mtime = 100;
    cpu.pc = PC;
}

static void step(void) {
    cpu_step(&cpu, &mem);
}

/* ---- A. When it jumps time ---------------------------------------------------------------------- */

void test_a1_jumps_to_mtimecmp(void) {
    setup(PRV_M);
    cpu_write_csr(&cpu, CSR_MSTATUS, MSTATUS_MIE | MSTATUS_SIE);
    cpu_write_csr(&cpu, CSR_MIE, INT_MTI);
    cpu.clint.mtimecmp = 5000;
    step();
    TEST_ASSERT_EQUAL_UINT64(5000, cpu.clint.mtime);
    TEST_ASSERT_EQUAL_HEX32(PC + 4, cpu.pc);
    TEST_ASSERT_TRUE(cpu_read_csr(&cpu, CSR_MIP) & INT_MTI);
}

void test_a2_a_target_above_32_bits(void) {
    setup(PRV_M);
    cpu_write_csr(&cpu, CSR_MIE, INT_MTI);
    cpu.clint.mtimecmp = 0x100000005ull;
    step();
    TEST_ASSERT_EQUAL_UINT64(0x100000005ull, cpu.clint.mtime);
    TEST_ASSERT_EQUAL_HEX32(5, cpu_read_csr(&cpu, CSR_TIME));
    TEST_ASSERT_EQUAL_HEX32(1, cpu_read_csr(&cpu, CSR_TIMEH));
}

void test_a3_the_global_enables_do_not_matter(void) {
    setup(PRV_M);
    cpu_write_csr(&cpu, CSR_MSTATUS, 0);                        // MIE and SIE clear
    cpu_write_csr(&cpu, CSR_MIE, INT_MTI);
    cpu.clint.mtimecmp = 5000;
    step();
    TEST_ASSERT_EQUAL_UINT64(5000, cpu.clint.mtime);
}

void test_a4_from_s_mode(void) {
    setup(PRV_S);
    cpu_write_csr(&cpu, CSR_MIE, INT_MTI);
    cpu.clint.mtimecmp = 5000;
    step();
    TEST_ASSERT_EQUAL_UINT64(5000, cpu.clint.mtime);
    TEST_ASSERT_EQUAL_HEX32(PC + 4, cpu.pc);
}

void test_a5_the_tick_phase_after_the_jump(void) {
    setup(PRV_M);
    cpu.clint.phase = 7;
    cpu_write_csr(&cpu, CSR_MIE, INT_MTI);
    cpu.clint.mtimecmp = 5000;
    step();
    TEST_ASSERT_EQUAL_UINT32(1, cpu.clint.phase);               // restarted at 0, then the WFI's own cycle
    TEST_ASSERT_EQUAL_UINT64(5000, cpu.clint.mtime);
}

/* ---- B. When it does not jump -------------------------------------------------------------------- */

void test_b1_mtie_clear(void) {
    setup(PRV_M);
    cpu.clint.mtimecmp = 5000;
    step();
    TEST_ASSERT_EQUAL_UINT64(100, cpu.clint.mtime);
}

void test_b2_no_timer_armed(void) {
    setup(PRV_M);
    cpu_write_csr(&cpu, CSR_MIE, INT_MTI);                      // mtimecmp is still all ones
    step();
    TEST_ASSERT_EQUAL_UINT64(100, cpu.clint.mtime);
}

void test_b3_the_timer_is_already_pending(void) {
    setup(PRV_M);
    cpu_write_csr(&cpu, CSR_MIE, INT_MTI);
    cpu.clint.mtimecmp = 50;
    step();
    TEST_ASSERT_EQUAL_UINT64(100, cpu.clint.mtime);
}

void test_b4_another_enabled_interrupt_is_pending(void) {
    setup(PRV_M);
    cpu_write_csr(&cpu, CSR_MIE, INT_MSI | INT_MTI);
    cpu.clint.msip = 1;
    cpu.clint.mtimecmp = 5000;
    step();
    TEST_ASSERT_EQUAL_UINT64(100, cpu.clint.mtime);
    TEST_ASSERT_EQUAL_HEX32(PC + 4, cpu.pc);
}

void test_b5_a_pending_but_disabled_interrupt_is_no_reason_to_wake(void) {
    setup(PRV_M);
    cpu_write_csr(&cpu, CSR_MIP, INT_SSI);                      // pending, SSIE clear
    cpu_write_csr(&cpu, CSR_MIE, INT_MTI);
    cpu.clint.mtimecmp = 5000;
    step();
    TEST_ASSERT_EQUAL_UINT64(5000, cpu.clint.mtime);
}

void test_b6_nothing_enabled(void) {
    setup(PRV_M);
    cpu.clint.mtimecmp = 5000;
    step();
    TEST_ASSERT_EQUAL_UINT64(100, cpu.clint.mtime);
    TEST_ASSERT_EQUAL_HEX32(PC + 4, cpu.pc);
}

/* ---- C. After the wake-up ------------------------------------------------------------------------- */

void test_c1_the_interrupt_is_taken_on_the_next_instruction(void) {
    setup(PRV_M);
    cpu_write_csr(&cpu, CSR_MSTATUS, MSTATUS_MIE);
    cpu_write_csr(&cpu, CSR_MIE, INT_MTI);
    cpu.clint.mtimecmp = 5000;
    step();                                                     // the WFI
    TEST_ASSERT_EQUAL_HEX32(PC + 4, cpu.pc);
    step();
    TEST_ASSERT_EQUAL_HEX32(MTVEC, cpu.pc);
    TEST_ASSERT_EQUAL_HEX32(0x80000007u, cpu_read_csr(&cpu, CSR_MCAUSE));
    TEST_ASSERT_EQUAL_HEX32(PC + 4, cpu_read_csr(&cpu, CSR_MEPC));      // machine.adoc:2694
}

void test_c2_with_interrupts_disabled_execution_just_continues(void) {
    setup(PRV_M);
    cpu_write_csr(&cpu, CSR_MIE, INT_MTI);
    cpu.clint.mtimecmp = 5000;
    step();
    step();                                                     // the NOP after the WFI
    TEST_ASSERT_EQUAL_HEX32(PC + 8, cpu.pc);
    TEST_ASSERT_TRUE(cpu_read_csr(&cpu, CSR_MIP) & INT_MTI);    // still pending
}

void test_c3_a_globally_disabled_pending_interrupt_wakes_it_without_a_jump(void) {
    setup(PRV_S);
    cpu_write_csr(&cpu, CSR_MIDELEG, INT_STI);
    cpu_write_csr(&cpu, CSR_MIP, INT_STI);                      // pending and locally enabled, but SIE is clear
    cpu_write_csr(&cpu, CSR_MIE, INT_MTI | INT_STI);
    cpu_write_csr(&cpu, CSR_STVEC, MTVEC);
    cpu.clint.mtimecmp = 5000;
    step();                                                     // the WFI: a reason to wake, so time does not jump
    TEST_ASSERT_EQUAL_UINT64(100, cpu.clint.mtime);
    TEST_ASSERT_EQUAL_HEX32(PC + 4, cpu.pc);
    TEST_ASSERT_EQUAL_HEX32(0, cpu_read_csr(&cpu, CSR_SCAUSE)); // nothing was taken: SIE is clear (:2718)
    cpu_write_csr(&cpu, CSR_MSTATUS, MSTATUS_SIE);              // now interrupts are enabled
    step();
    TEST_ASSERT_EQUAL_HEX32(0x80000005u, cpu_read_csr(&cpu, CSR_SCAUSE));
    TEST_ASSERT_EQUAL_HEX32(PC + 4, cpu_read_csr(&cpu, CSR_SEPC));       // taken before the instruction after the WFI
}

void test_c3b_an_enabled_interrupt_is_taken_before_the_wfi_runs(void) {
    setup(PRV_S);
    cpu_write_csr(&cpu, CSR_MIDELEG, INT_STI);
    cpu_write_csr(&cpu, CSR_MIP, INT_STI);
    cpu_write_csr(&cpu, CSR_MIE, INT_STI);
    cpu_write_csr(&cpu, CSR_MSTATUS, MSTATUS_SIE);
    cpu_write_csr(&cpu, CSR_STVEC, MTVEC);
    step();                                                     // already pending and enabled: taken at once, the WFI never runs
    TEST_ASSERT_EQUAL_HEX32(PC, cpu_read_csr(&cpu, CSR_SEPC));
}

void test_c4_a_second_wfi_jumps_to_the_new_target(void) {
    setup(PRV_M);
    mem_write32(&mem, PC + 4, WFI);
    cpu_write_csr(&cpu, CSR_MSTATUS, MSTATUS_MIE);
    cpu_write_csr(&cpu, CSR_MIE, INT_MTI);
    cpu.clint.mtimecmp = 5000;
    step();                                                     // WFI: jumps to 5000
    step();                                                     // the interrupt
    TEST_ASSERT_EQUAL_HEX32(MTVEC, cpu.pc);
    cpu.clint.mtimecmp = 9000;                                  // what a handler does: rearm the timer
    step();                                                     // mret, back to PC + 4
    TEST_ASSERT_EQUAL_HEX32(PC + 4, cpu.pc);
    step();                                                     // the second WFI
    TEST_ASSERT_EQUAL_UINT64(9000, cpu.clint.mtime);
}

/* ---- D. Counters ------------------------------------------------------------------------------------ */

void test_d_counters(void) {
    setup(PRV_M);
    cpu_write_csr(&cpu, CSR_MIE, INT_MTI);
    cpu.clint.mtimecmp = 0x2000000005ull;
    uint64_t instret = cpu.minstret, cycle = cpu.mcycle;
    step();
    TEST_ASSERT_EQUAL_UINT64(instret + 1, cpu.minstret);        // the WFI retires
    TEST_ASSERT_EQUAL_UINT64(cycle + 1, cpu.mcycle);            // and costs one cycle, whatever time did
    TEST_ASSERT_EQUAL_HEX32(5, cpu_read_csr(&cpu, CSR_TIME));
    TEST_ASSERT_EQUAL_HEX32(0x20, cpu_read_csr(&cpu, CSR_TIMEH));
    TEST_ASSERT_EQUAL_UINT32(cycle + 1, cpu_read_csr(&cpu, CSR_CYCLE));
}

/* ---- E. Privilege ------------------------------------------------------------------------------------ */

void test_e1_u_mode_wfi_is_illegal_and_moves_nothing(void) {
    setup(PRV_U);
    cpu_write_csr(&cpu, CSR_MIE, INT_MTI);
    cpu.clint.mtimecmp = 5000;
    step();
    TEST_ASSERT_EQUAL_UINT32(2, cpu_read_csr(&cpu, CSR_MCAUSE));
    TEST_ASSERT_EQUAL_HEX32(PC, cpu_read_csr(&cpu, CSR_MEPC));
    TEST_ASSERT_EQUAL_UINT64(100, cpu.clint.mtime);
}

void test_e2_s_mode_with_tw_is_illegal_and_moves_nothing(void) {
    setup(PRV_S);
    cpu_write_csr(&cpu, CSR_MSTATUS, MSTATUS_TW);
    cpu_write_csr(&cpu, CSR_MIE, INT_MTI);
    cpu.clint.mtimecmp = 5000;
    step();
    TEST_ASSERT_EQUAL_UINT32(2, cpu_read_csr(&cpu, CSR_MCAUSE));
    TEST_ASSERT_EQUAL_UINT64(100, cpu.clint.mtime);
}

/* ---- F. Determinism ------------------------------------------------------------------------------------ */

static void idle_loop(unsigned rounds, uint64_t *mtime, uint64_t *instret, uint64_t *cycle) {
    setup(PRV_M);
    mem_write32(&mem, PC + 4, 0xFFDFF06Fu);                     // jal x0, -4: back to the WFI
    cpu_write_csr(&cpu, CSR_MSTATUS, MSTATUS_MIE);
    cpu_write_csr(&cpu, CSR_MIE, INT_MTI);
    cpu.clint.mtimecmp = 3000;
    for (unsigned i = 0; i < rounds; i++) {
        cpu.pc = PC;
        step();                                                 // WFI
        step();                                                 // the interrupt
        cpu.clint.mtimecmp += 3000;                             // the handler rearms
        step();                                                 // mret
    }
    *mtime = cpu.clint.mtime;
    *instret = cpu.minstret;
    *cycle = cpu.mcycle;
}

void test_f1_two_runs_agree(void) {
    uint64_t t1, i1, c1, t2, i2, c2;
    idle_loop(6, &t1, &i1, &c1);
    idle_loop(6, &t2, &i2, &c2);
    TEST_ASSERT_EQUAL_UINT64(t1, t2);
    TEST_ASSERT_EQUAL_UINT64(i1, i2);
    TEST_ASSERT_EQUAL_UINT64(c1, c2);
    TEST_ASSERT_EQUAL_UINT64(18000, t1);                        // the sixth target, reached exactly
}

/* ---- G. The helper on its own ---------------------------------------------------------------------------- */

void test_g1_idle_until_timer_never_moves_time_back_or_to_an_unarmed_timer(void) {
    Clint c;
    clint_reset(&c);
    TEST_ASSERT_FALSE(clint_idle_until_timer(&c));                          // mtimecmp is all ones: nothing is armed
    TEST_ASSERT_EQUAL_UINT64(0, c.mtime);
    c.mtime = 100;
    c.mtimecmp = 100;
    TEST_ASSERT_FALSE(clint_idle_until_timer(&c));                          // already pending: equal
    c.mtimecmp = 50;
    TEST_ASSERT_FALSE(clint_idle_until_timer(&c));                          // already pending: past
    TEST_ASSERT_EQUAL_UINT64(100, c.mtime);
    c.mtimecmp = 101;
    c.phase = 6;
    TEST_ASSERT_TRUE(clint_idle_until_timer(&c));
    TEST_ASSERT_EQUAL_UINT64(101, c.mtime);
    TEST_ASSERT_EQUAL_UINT32(0, c.phase);
    c.mtimecmp = UINT64_MAX - 1;                                            // the largest armed value
    TEST_ASSERT_TRUE(clint_idle_until_timer(&c));
    TEST_ASSERT_EQUAL_UINT64(UINT64_MAX - 1, c.mtime);
}

/* ---- H. Device events (Milestone 8.4) ---------------------------------------------------------------------------- */
// A UART with one received character, so its receive timeout comes up by itself after UART_TIMEOUT_CYCLES idle cycles.
// The interrupt reaches the hart through the PLIC (source 10, context 0, MEIP).

static Uart uart;
static bool uart_line_for_test(void *ctx) { return uart_interrupt_pending(ctx); }

static void uart_setup(PrivMode mode) {
    setup(mode);
    uart_reset(&uart);
    mem_add_device(&mem, uart_device(&uart));
    mem_bus_write(&mem, UART_BASE + UART_IER, 1, UART_IER_ERBFI);
    mem_bus_write(&mem, UART_BASE + UART_FCR, 1, 0x81);                     // FIFOs on, trigger 8
    uart_receive(&uart, 0x41);
    plic_set_source(&cpu.plic, 10, uart_line_for_test, &uart);
    cpu.plic.priority[10] = 1;
    cpu.plic.enable[0] = 1u << 10;
    cpu_write_csr(&cpu, CSR_MSTATUS, MSTATUS_MIE);
}

void test_h1_a_device_event_before_the_timer_limits_the_jump(void) {
    uart_setup(PRV_M);
    cpu_write_csr(&cpu, CSR_MIE, INT_MTI | INT_MEI);
    cpu.clint.mtimecmp = 1000000;
    step();
    TEST_ASSERT_EQUAL_UINT64(100 + UART_TIMEOUT_CYCLES / CLINT_CYCLES_PER_TICK, cpu.clint.mtime);   // not mtimecmp
    TEST_ASSERT_EQUAL_UINT32(UART_TIMEOUT_CYCLES, uart.rx_idle);
    TEST_ASSERT_TRUE(cpu_read_csr(&cpu, CSR_MIP) & INT_MEI);
    step();                                                                  // the interrupt, on the next instruction
    TEST_ASSERT_EQUAL_HEX32(MTVEC, cpu.pc);
    TEST_ASSERT_EQUAL_HEX32(0x8000000Bu, cpu_read_csr(&cpu, CSR_MCAUSE));
    TEST_ASSERT_EQUAL_HEX32(PC + 4, cpu_read_csr(&cpu, CSR_MEPC));
}

void test_h2_the_timer_first_advances_the_uart_by_the_same_time(void) {
    uart_setup(PRV_M);
    cpu_write_csr(&cpu, CSR_MIE, INT_MTI | INT_MEI);
    cpu.clint.mtimecmp = 150;                                                // 500 cycles away
    step();
    TEST_ASSERT_EQUAL_UINT64(150, cpu.clint.mtime);
    TEST_ASSERT_EQUAL_UINT32(500 + 1, uart.rx_idle);                         // the jump, and the WFI's own cycle
    TEST_ASSERT_FALSE(cpu_read_csr(&cpu, CSR_MIP) & INT_MEI);
    TEST_ASSERT_TRUE(cpu_read_csr(&cpu, CSR_MIP) & INT_MTI);
}

void test_h3_with_the_external_enables_clear_the_device_event_is_ignored(void) {
    uart_setup(PRV_M);
    cpu_write_csr(&cpu, CSR_MIE, INT_MTI);
    cpu.clint.mtimecmp = 100 + 5000;                                         // 50000 cycles, after the UART's 35000
    step();
    TEST_ASSERT_EQUAL_UINT64(5100, cpu.clint.mtime);
}

void test_h4_only_meie_or_seie_makes_the_device_event_count(void) {
    uart_setup(PRV_M);
    cpu_write_csr(&cpu, CSR_MIE, INT_SEI | INT_MTI);
    cpu.clint.mtimecmp = 1000000;
    step();
    TEST_ASSERT_EQUAL_UINT64(100 + UART_TIMEOUT_CYCLES / CLINT_CYCLES_PER_TICK, cpu.clint.mtime);
}

void test_h5_with_no_timer_armed_a_device_event_still_passes_time(void) {
    uart_setup(PRV_M);
    cpu_write_csr(&cpu, CSR_MIE, INT_MEI);
    step();                                                                  // mtimecmp is all ones
    TEST_ASSERT_EQUAL_UINT64(100 + UART_TIMEOUT_CYCLES / CLINT_CYCLES_PER_TICK, cpu.clint.mtime);
    step();
    TEST_ASSERT_EQUAL_HEX32(0x8000000Bu, cpu_read_csr(&cpu, CSR_MCAUSE));
}

void test_h6_no_device_event_pending_falls_back_to_the_timer(void) {
    uart_setup(PRV_M);
    uart_reset(&uart);                                                       // no character now
    cpu_write_csr(&cpu, CSR_MIE, INT_MTI | INT_MEI);
    cpu.clint.mtimecmp = 7000;
    step();
    TEST_ASSERT_EQUAL_UINT64(7000, cpu.clint.mtime);
}

void test_h7_nothing_enabled_nothing_passes(void) {
    uart_setup(PRV_M);
    cpu_write_csr(&cpu, CSR_MIE, 0);
    cpu.clint.mtimecmp = 1000;
    step();
    TEST_ASSERT_EQUAL_UINT64(100, cpu.clint.mtime);
    TEST_ASSERT_EQUAL_UINT32(1, uart.rx_idle);                               // only the WFI's own cycle
}

void test_h8_an_enabled_pending_interrupt_means_no_jump(void) {
    uart_setup(PRV_M);
    cpu_write_csr(&cpu, CSR_MSTATUS, 0);                                     // not taken, but pending and enabled
    cpu_write_csr(&cpu, CSR_MIE, INT_MTI | INT_MEI);
    cpu.clint.mtimecmp = 1000000;
    while (!(cpu_read_csr(&cpu, CSR_MIP) & INT_MEI)) uart_tick(&uart);
    step();
    TEST_ASSERT_EQUAL_UINT64(100, cpu.clint.mtime);
}

void test_h9_mcycle_and_minstret(void) {
    uart_setup(PRV_M);
    cpu_write_csr(&cpu, CSR_MIE, INT_MEI);
    uint64_t cycle = cpu.mcycle, instret = cpu.minstret;
    step();
    TEST_ASSERT_EQUAL_UINT64(cycle + 1, cpu.mcycle);                         // the WFI's own cycle, none for the idle time
    TEST_ASSERT_EQUAL_UINT64(instret + 1, cpu.minstret);
}

static bool poll_delivers;
static unsigned polls;
static bool fake_poll(void *ctx) { (void)ctx; polls++; return poll_delivers; }

void test_h10_host_input_that_arrived_means_no_jump(void) {
    uart_setup(PRV_M);
    cpu_write_csr(&cpu, CSR_MIE, INT_MTI | INT_MEI);
    cpu.clint.mtimecmp = 5000;
    cpu.idle_poll = fake_poll;
    poll_delivers = true;
    polls = 0;
    step();
    TEST_ASSERT_EQUAL_UINT(1, polls);
    TEST_ASSERT_EQUAL_UINT64(100, cpu.clint.mtime);
    TEST_ASSERT_EQUAL_UINT32(1, uart.rx_idle);
}

void test_h11_a_poll_that_delivers_nothing_changes_nothing(void) {
    uart_setup(PRV_M);
    cpu_write_csr(&cpu, CSR_MIE, INT_MTI | INT_MEI);
    cpu.clint.mtimecmp = 5000;
    cpu.idle_poll = fake_poll;
    poll_delivers = false;
    polls = 0;
    step();
    TEST_ASSERT_EQUAL_UINT(1, polls);
    TEST_ASSERT_EQUAL_UINT64(100 + UART_TIMEOUT_CYCLES / CLINT_CYCLES_PER_TICK, cpu.clint.mtime);
}

void test_h12_other_instructions_never_poll(void) {
    setup(PRV_M);
    cpu.idle_poll = fake_poll;
    polls = 0;
    mem_write32(&mem, PC, NOP);
    step();
    TEST_ASSERT_EQUAL_UINT(0, polls);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_a1_jumps_to_mtimecmp);
    RUN_TEST(test_a2_a_target_above_32_bits);
    RUN_TEST(test_a3_the_global_enables_do_not_matter);
    RUN_TEST(test_a4_from_s_mode);
    RUN_TEST(test_a5_the_tick_phase_after_the_jump);
    RUN_TEST(test_b1_mtie_clear);
    RUN_TEST(test_b2_no_timer_armed);
    RUN_TEST(test_b3_the_timer_is_already_pending);
    RUN_TEST(test_b4_another_enabled_interrupt_is_pending);
    RUN_TEST(test_b5_a_pending_but_disabled_interrupt_is_no_reason_to_wake);
    RUN_TEST(test_b6_nothing_enabled);
    RUN_TEST(test_c1_the_interrupt_is_taken_on_the_next_instruction);
    RUN_TEST(test_c2_with_interrupts_disabled_execution_just_continues);
    RUN_TEST(test_c3_a_globally_disabled_pending_interrupt_wakes_it_without_a_jump);
    RUN_TEST(test_c3b_an_enabled_interrupt_is_taken_before_the_wfi_runs);
    RUN_TEST(test_c4_a_second_wfi_jumps_to_the_new_target);
    RUN_TEST(test_d_counters);
    RUN_TEST(test_e1_u_mode_wfi_is_illegal_and_moves_nothing);
    RUN_TEST(test_e2_s_mode_with_tw_is_illegal_and_moves_nothing);
    RUN_TEST(test_f1_two_runs_agree);
    RUN_TEST(test_g1_idle_until_timer_never_moves_time_back_or_to_an_unarmed_timer);
    RUN_TEST(test_h1_a_device_event_before_the_timer_limits_the_jump);
    RUN_TEST(test_h2_the_timer_first_advances_the_uart_by_the_same_time);
    RUN_TEST(test_h3_with_the_external_enables_clear_the_device_event_is_ignored);
    RUN_TEST(test_h4_only_meie_or_seie_makes_the_device_event_count);
    RUN_TEST(test_h5_with_no_timer_armed_a_device_event_still_passes_time);
    RUN_TEST(test_h6_no_device_event_pending_falls_back_to_the_timer);
    RUN_TEST(test_h7_nothing_enabled_nothing_passes);
    RUN_TEST(test_h8_an_enabled_pending_interrupt_means_no_jump);
    RUN_TEST(test_h9_mcycle_and_minstret);
    RUN_TEST(test_h10_host_input_that_arrived_means_no_jump);
    RUN_TEST(test_h11_a_poll_that_delivers_nothing_changes_nothing);
    RUN_TEST(test_h12_other_instructions_never_poll);
    return UNITY_END();
}
