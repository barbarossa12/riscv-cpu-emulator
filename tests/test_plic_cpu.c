#include <string.h>
#include "unity.h"
#include "cpu_test_util.h"
#include "../src/execute.h"

// The PLIC's two outputs as the hart sees them: MEIP and SEIP in mip and sip, the SEIP read-modify-write
// rule (machine.adoc:1436-1443), and the interrupts being taken.
#define RAM_BASE 0x80000000u
#define MTVEC    (RAM_BASE + 0x100u)
#define STVEC    (RAM_BASE + 0x200u)
#define PC       (RAM_BASE + 0x400u)
#define SENTINEL 0x11111111u

#define NOP           0x00000013u
#define ADDI_X1_5     0x00500093u
#define CSRRS_X5_MIP  0x344022F3u   // csrrs x5, mip, x0
#define CSRRSI_MIP_2  0x34416073u   // csrrsi x0, mip, 2   (SSIP)
#define CSRRCI_MIP_2  0x34417073u   // csrrci x0, mip, 2   (SSIP)

static CPU cpu;
static Memory mem;
static bool level[PLIC_NSOURCES + 1];

static bool test_line(void *ctx) { return *(bool *)ctx; }

static void put(uint32_t addr, uint32_t word) { mem_write32(&mem, addr, word); }

// Source 10 is connected with priority 3 and enabled for the given contexts (a bit mask: 1 = M, 2 = S)
static void setup(PrivMode mode, unsigned contexts) {
    test_cpu_reset_in_mode(&cpu, mode);
    memset(level, 0, sizeof level);
    for (uint32_t a = RAM_BASE; a < RAM_BASE + 0x1000; a += 4) put(a, NOP);
    put(PC, ADDI_X1_5);
    cpu_write_csr(&cpu, CSR_MTVEC, MTVEC);
    cpu_write_csr(&cpu, CSR_STVEC, STVEC);
    cpu_write_reg(&cpu, 1, SENTINEL);
    cpu.pc = PC;
    plic_set_source(&cpu.plic, 10, test_line, &level[10]);
    cpu.plic.priority[10] = 3;
    for (uint32_t c = 0; c < PLIC_NCONTEXTS; c++) {
        if (contexts & (1u << c)) cpu.plic.enable[c] = 1u << 10;
    }
}

void setUp(void) { mem_init_at(&mem, RAM_BASE, 0x2000); }
void tearDown(void) { mem_free(&mem); }

static void step(void) { cpu_step(&cpu, &mem); }

static void expect_not_taken(void) {
    TEST_ASSERT_EQUAL_HEX32(PC + 4, cpu.pc);
    TEST_ASSERT_EQUAL_UINT32(5, cpu_read_reg(&cpu, 1));
}

/* ---- The outputs in mip and sip ---- */

void test_e1_context_0_drives_meip_only(void) {
    setup(PRV_M, 1);
    TEST_ASSERT_EQUAL_HEX32(0, cpu_read_csr(&cpu, CSR_MIP));
    level[10] = true;
    TEST_ASSERT_EQUAL_HEX32(INT_MEI, cpu_read_csr(&cpu, CSR_MIP));
}

void test_e2_context_1_drives_seip_only(void) {
    setup(PRV_M, 2);
    level[10] = true;
    TEST_ASSERT_EQUAL_HEX32(INT_SEI, cpu_read_csr(&cpu, CSR_MIP));
}

void test_e3_both_contexts_drive_both_bits(void) {
    setup(PRV_M, 3);
    level[10] = true;
    TEST_ASSERT_EQUAL_HEX32(INT_MEI | INT_SEI, cpu_read_csr(&cpu, CSR_MIP));
}

void test_e4_sip_shows_seip_only_when_delegated(void) {
    setup(PRV_M, 2);
    level[10] = true;
    TEST_ASSERT_EQUAL_HEX32(0, cpu_read_csr(&cpu, CSR_SIP));
    cpu_write_csr(&cpu, CSR_MIDELEG, INT_SEI);
    TEST_ASSERT_EQUAL_HEX32(INT_SEI, cpu_read_csr(&cpu, CSR_SIP));
}

void test_e5_meip_is_not_visible_in_sip(void) {
    setup(PRV_M, 1);
    cpu_write_csr(&cpu, CSR_MIDELEG, INT_S_ALL);
    level[10] = true;
    TEST_ASSERT_EQUAL_HEX32(0, cpu_read_csr(&cpu, CSR_SIP));
}

void test_e6_the_signal_leaves_with_a_claim(void) {
    setup(PRV_M, 1);
    level[10] = true;
    TEST_ASSERT_TRUE(cpu_read_csr(&cpu, CSR_MIP) & INT_MEI);
    mem_add_device(&mem, plic_device(&cpu.plic));
    uint32_t id = 0;
    TEST_ASSERT_TRUE(mem_bus_read(&mem, PLIC_BASE + PLIC_CONTEXT + 4, 4, &id));
    TEST_ASSERT_EQUAL_UINT32(10, id);
    TEST_ASSERT_EQUAL_HEX32(0, cpu_read_csr(&cpu, CSR_MIP));    // the line is high but the source is in flight
}

/* ---- The SEIP read-modify-write rule ---- */

// The controller signal E must not be copied into the software bit B by csrrs, csrrc, csrrsi or csrrci
static void drop_controller_signal(void) {
    cpu.plic.threshold[1] = 7;   // EIP goes low, whatever the latched request
}

void test_f1_csrrs_reads_the_or(void) {
    setup(PRV_M, 2);
    level[10] = true;
    put(PC, CSRRS_X5_MIP);
    step();
    TEST_ASSERT_EQUAL_HEX32(INT_SEI, cpu_read_reg(&cpu, 5) & INT_SEI);
}

void test_f2_csrrsi_does_not_copy_the_signal_into_the_software_bit(void) {
    setup(PRV_M, 2);
    level[10] = true;
    put(PC, CSRRSI_MIP_2);
    step();
    TEST_ASSERT_EQUAL_HEX32(INT_SSI, cpu.csr[CSR_MIP]);         // B = 0 for SEIP, SSIP set
    drop_controller_signal();
    TEST_ASSERT_EQUAL_HEX32(INT_SSI, cpu_read_csr(&cpu, CSR_MIP));
}

void test_f3_csrrci_does_not_copy_the_signal_into_the_software_bit(void) {
    setup(PRV_M, 2);
    cpu_write_csr(&cpu, CSR_MIP, INT_SSI);
    level[10] = true;
    put(PC, CSRRCI_MIP_2);
    step();
    TEST_ASSERT_EQUAL_HEX32(0, cpu.csr[CSR_MIP]);               // SSIP cleared, SEIP software bit still 0
    drop_controller_signal();
    TEST_ASSERT_EQUAL_HEX32(0, cpu_read_csr(&cpu, CSR_MIP));
}

void test_f4_the_software_bit_survives_the_signal_dropping(void) {
    setup(PRV_M, 2);
    cpu_write_csr(&cpu, CSR_MIP, INT_SEI);                      // B = 1
    level[10] = true;
    put(PC, CSRRSI_MIP_2);
    step();
    drop_controller_signal();
    TEST_ASSERT_EQUAL_HEX32(INT_SEI | INT_SSI, cpu_read_csr(&cpu, CSR_MIP));
}

void test_f5_csrrci_leaves_the_software_bit_when_it_is_not_in_the_mask(void) {
    setup(PRV_M, 2);
    cpu_write_csr(&cpu, CSR_MIP, INT_SEI | INT_SSI);
    put(PC, CSRRCI_MIP_2);
    step();
    TEST_ASSERT_EQUAL_HEX32(INT_SEI, cpu.csr[CSR_MIP]);
}

/* ---- Taking the interrupts ---- */

void test_g1_machine_external_interrupt(void) {
    setup(PRV_M, 1);
    cpu_write_csr(&cpu, CSR_MSTATUS, MSTATUS_MIE);
    cpu_write_csr(&cpu, CSR_MIE, INT_MEI);
    level[10] = true;
    step();
    TEST_ASSERT_EQUAL_HEX32(MTVEC, cpu.pc);
    TEST_ASSERT_EQUAL_HEX32(0x8000000Bu, cpu_read_csr(&cpu, CSR_MCAUSE));
    TEST_ASSERT_EQUAL_HEX32(PC, cpu_read_csr(&cpu, CSR_MEPC));
    TEST_ASSERT_EQUAL_HEX32(SENTINEL, cpu_read_reg(&cpu, 1));
}

void test_g2_meie_clear_blocks_it(void) {
    setup(PRV_M, 1);
    cpu_write_csr(&cpu, CSR_MSTATUS, MSTATUS_MIE);
    level[10] = true;
    step();
    expect_not_taken();
}

void test_g3_threshold_masks_the_interrupt(void) {
    setup(PRV_M, 1);
    cpu_write_csr(&cpu, CSR_MSTATUS, MSTATUS_MIE);
    cpu_write_csr(&cpu, CSR_MIE, INT_MEI);
    cpu.plic.threshold[0] = 3;
    level[10] = true;
    step();
    expect_not_taken();
}

void test_g4_delegated_supervisor_external_interrupt_from_s_mode(void) {
    setup(PRV_S, 2);
    cpu_write_csr(&cpu, CSR_MIDELEG, INT_SEI);
    cpu_write_csr(&cpu, CSR_MIE, INT_SEI);
    cpu_write_csr(&cpu, CSR_MSTATUS, MSTATUS_SIE);
    level[10] = true;
    step();
    TEST_ASSERT_EQUAL_HEX32(STVEC, cpu.pc);
    TEST_ASSERT_EQUAL_HEX32(0x80000009u, cpu_read_csr(&cpu, CSR_SCAUSE));
    TEST_ASSERT_EQUAL_HEX32(PC, cpu_read_csr(&cpu, CSR_SEPC));
    TEST_ASSERT_EQUAL_INT(PRV_S, cpu.privilege_mode);
}

void test_g5_delegated_supervisor_external_interrupt_from_u_mode(void) {
    setup(PRV_U, 2);
    cpu_write_csr(&cpu, CSR_MIDELEG, INT_SEI);
    cpu_write_csr(&cpu, CSR_MIE, INT_SEI);
    level[10] = true;
    step();
    TEST_ASSERT_EQUAL_HEX32(STVEC, cpu.pc);
    TEST_ASSERT_EQUAL_HEX32(0x80000009u, cpu_read_csr(&cpu, CSR_SCAUSE));
}

void test_g6_not_taken_in_s_mode_with_sie_clear(void) {
    setup(PRV_S, 2);
    cpu_write_csr(&cpu, CSR_MIDELEG, INT_SEI);
    cpu_write_csr(&cpu, CSR_MIE, INT_SEI);
    level[10] = true;
    step();
    expect_not_taken();
}

void test_g7_undelegated_supervisor_external_goes_to_m_with_cause_9(void) {
    setup(PRV_S, 2);
    cpu_write_csr(&cpu, CSR_MIE, INT_SEI);
    level[10] = true;
    step();
    TEST_ASSERT_EQUAL_HEX32(MTVEC, cpu.pc);
    TEST_ASSERT_EQUAL_HEX32(0x80000009u, cpu_read_csr(&cpu, CSR_MCAUSE));
}

void test_g8_machine_external_beats_supervisor_external(void) {
    setup(PRV_S, 3);
    cpu_write_csr(&cpu, CSR_MIDELEG, INT_SEI);
    cpu_write_csr(&cpu, CSR_MIE, INT_MEI | INT_SEI);
    cpu_write_csr(&cpu, CSR_MSTATUS, MSTATUS_SIE);
    level[10] = true;
    step();
    TEST_ASSERT_EQUAL_HEX32(MTVEC, cpu.pc);
    TEST_ASSERT_EQUAL_HEX32(0x8000000Bu, cpu_read_csr(&cpu, CSR_MCAUSE));
}

void test_g9_machine_external_beats_the_timer(void) {
    setup(PRV_M, 1);
    cpu_write_csr(&cpu, CSR_MSTATUS, MSTATUS_MIE);
    cpu_write_csr(&cpu, CSR_MIE, INT_MEI | INT_MTI);
    cpu.clint.mtimecmp = 0;
    level[10] = true;
    step();
    TEST_ASSERT_EQUAL_HEX32(0x8000000Bu, cpu_read_csr(&cpu, CSR_MCAUSE));
}

void test_g10_a_completed_source_interrupts_again(void) {
    setup(PRV_M, 1);
    mem_add_device(&mem, plic_device(&cpu.plic));
    cpu_write_csr(&cpu, CSR_MSTATUS, MSTATUS_MIE);
    cpu_write_csr(&cpu, CSR_MIE, INT_MEI);
    level[10] = true;
    step();                                                     // taken
    TEST_ASSERT_EQUAL_HEX32(MTVEC, cpu.pc);
    uint32_t id = 0;
    TEST_ASSERT_TRUE(mem_bus_read(&mem, PLIC_BASE + PLIC_CONTEXT + 4, 4, &id));
    TEST_ASSERT_EQUAL_UINT32(10, id);
    cpu_write_csr(&cpu, CSR_MSTATUS, MSTATUS_MIE);              // handler re-enables, line still high
    cpu.pc = PC;
    step();
    TEST_ASSERT_EQUAL_HEX32(PC + 4, cpu.pc);                    // in flight: no new interrupt
    TEST_ASSERT_TRUE(mem_bus_write(&mem, PLIC_BASE + PLIC_CONTEXT + 4, 4, 10));
    cpu.pc = PC;
    step();
    TEST_ASSERT_EQUAL_HEX32(MTVEC, cpu.pc);                     // completed, still asserted: again
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_e1_context_0_drives_meip_only);
    RUN_TEST(test_e2_context_1_drives_seip_only);
    RUN_TEST(test_e3_both_contexts_drive_both_bits);
    RUN_TEST(test_e4_sip_shows_seip_only_when_delegated);
    RUN_TEST(test_e5_meip_is_not_visible_in_sip);
    RUN_TEST(test_e6_the_signal_leaves_with_a_claim);
    RUN_TEST(test_f1_csrrs_reads_the_or);
    RUN_TEST(test_f2_csrrsi_does_not_copy_the_signal_into_the_software_bit);
    RUN_TEST(test_f3_csrrci_does_not_copy_the_signal_into_the_software_bit);
    RUN_TEST(test_f4_the_software_bit_survives_the_signal_dropping);
    RUN_TEST(test_f5_csrrci_leaves_the_software_bit_when_it_is_not_in_the_mask);
    RUN_TEST(test_g1_machine_external_interrupt);
    RUN_TEST(test_g2_meie_clear_blocks_it);
    RUN_TEST(test_g3_threshold_masks_the_interrupt);
    RUN_TEST(test_g4_delegated_supervisor_external_interrupt_from_s_mode);
    RUN_TEST(test_g5_delegated_supervisor_external_interrupt_from_u_mode);
    RUN_TEST(test_g6_not_taken_in_s_mode_with_sie_clear);
    RUN_TEST(test_g7_undelegated_supervisor_external_goes_to_m_with_cause_9);
    RUN_TEST(test_g8_machine_external_beats_supervisor_external);
    RUN_TEST(test_g9_machine_external_beats_the_timer);
    RUN_TEST(test_g10_a_completed_source_interrupts_again);
    return UNITY_END();
}
