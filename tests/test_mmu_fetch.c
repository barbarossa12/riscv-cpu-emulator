#include "unity.h"
#include "cpu_test_util.h"
#include "../src/execute.h"
#include "../src/pmp.h"

#define ROOT   0x1000u
#define L0     0x2000u
#define MTVEC  0x300u
#define STVEC  0x400u

#define V 0x01u
#define R 0x02u
#define W 0x04u
#define X 0x08u
#define U 0x10u
#define A 0x40u
#define D 0x80u
#define RXAD (R | X | A | D)

#define SATP_SV32 0x80000000u

#define ADDI_X1_5 0x00500093u   // ADDI x1, x0, 5
#define ADDI_X1_1 0x00100093u   // ADDI x1, x0, 1
#define ADDI_X1_2 0x00200093u   // ADDI x1, x0, 2
#define ADDI_X1_7 0x00700093u   // ADDI x1, x0, 7
#define C_ADDI_X1_5 0x0095u     // C.ADDI x1, 5

static CPU cpu;
static Memory mem;

static void set_pte(uint32_t table, uint32_t index, uint32_t ppn, uint32_t flags) {
	mem_write32(&mem, table + index * 4, (ppn << 10) | flags);
}

static uint32_t get_pte(uint32_t table, uint32_t index) {
	return mem_read32(&mem, table + index * 4);
}

// Virtual page `vpn` (0..1023) of the first 4 MiB maps to physical page `ppn`
static void map(uint32_t vpn, uint32_t ppn, uint32_t flags) {
	set_pte(L0, vpn, ppn, flags);
}

// Sv32 on, S-mode, root[0] -> L0, virtual page 5 -> physical page 0x50 (execute, not a user page)
void setUp(void) {
	mem_init(&mem, 0x100000);
	test_cpu_reset_in_mode(&cpu, PRV_S);
	cpu_write_csr(&cpu, CSR_SATP, SATP_SV32 | (ROOT >> 12));
	cpu_write_csr(&cpu, CSR_MTVEC, MTVEC);
	cpu_write_csr(&cpu, CSR_STVEC, STVEC);
	cpu_write_reg(&cpu, 1, 0x12345678);
	set_pte(ROOT, 0, L0 >> 12, V);
	map(5, 0x50, V | RXAD);
}

void tearDown(void) {
	mem_free(&mem);
}

static void step_at(uint32_t pc) {
	cpu.pc = pc;
	cpu_step(&cpu, &mem);
}

static void expect_trap(uint32_t pc, uint32_t cause, uint32_t tval) {
	TEST_ASSERT_EQUAL_HEX32(MTVEC, cpu.pc);
	TEST_ASSERT_EQUAL_UINT32(cause, cpu_read_csr(&cpu, CSR_MCAUSE));
	TEST_ASSERT_EQUAL_HEX32(tval, cpu_read_csr(&cpu, CSR_MTVAL));
	TEST_ASSERT_EQUAL_HEX32(pc, cpu_read_csr(&cpu, CSR_MEPC));
	TEST_ASSERT_EQUAL_INT(PRV_M, cpu.privilege_mode);
	TEST_ASSERT_EQUAL_HEX32(0x12345678, cpu_read_reg(&cpu, 1));   // the instruction did not run
}

// PMP entry 0 grants only the given permissions on the 4 KiB at pa; entry 1 grants everything else
static void pmp_page_only(uint32_t pa, uint8_t perms) {
	cpu.pmpcfg[0] = 0x18 | perms;                      // NAPOT
	cpu.pmpaddr[0] = (pa >> 2) | 0x1FF;
	cpu.pmpcfg[1] = 0x1F;
	cpu.pmpaddr[1] = 0x7FFFFFFF;
}

/* ---- A. Successful fetches -------------------------------------------------------------------- */

void test_a1_fetch_32_bit(void) {
	mem_write32(&mem, 0x50000, ADDI_X1_5);
	step_at(0x5000);
	TEST_ASSERT_EQUAL_UINT32(5, cpu_read_reg(&cpu, 1));
	TEST_ASSERT_EQUAL_HEX32(0x5004, cpu.pc);
}

void test_a2_fetch_compressed(void) {
	cpu_write_reg(&cpu, 1, 0);                         // C.ADDI adds to x1
	mem_write16(&mem, 0x50000, C_ADDI_X1_5);
	step_at(0x5000);
	TEST_ASSERT_EQUAL_UINT32(5, cpu_read_reg(&cpu, 1));
	TEST_ASSERT_EQUAL_HEX32(0x5002, cpu.pc);
}

void test_a3_page_offset_is_kept(void) {
	mem_write32(&mem, 0x50FFC, ADDI_X1_5);
	step_at(0x5FFC);
	TEST_ASSERT_EQUAL_UINT32(5, cpu_read_reg(&cpu, 1));
	TEST_ASSERT_EQUAL_HEX32(0x6000, cpu.pc);
}

void test_a4_u_mode_on_a_user_page(void) {
	cpu.privilege_mode = PRV_U;
	map(5, 0x50, V | U | RXAD);
	mem_write32(&mem, 0x50000, ADDI_X1_5);
	step_at(0x5000);
	TEST_ASSERT_EQUAL_UINT32(5, cpu_read_reg(&cpu, 1));
}

void test_a5_fetch_sets_accessed_not_dirty(void) {
	map(5, 0x50, V | R | X);
	mem_write32(&mem, 0x50000, ADDI_X1_5);
	step_at(0x5000);
	TEST_ASSERT_EQUAL_UINT32(5, cpu_read_reg(&cpu, 1));
	TEST_ASSERT_EQUAL_HEX32((0x50u << 10) | V | R | X | A, get_pte(L0, 5));
}

void test_a6_megapage(void) {
	set_pte(ROOT, 2, 0x000, V | RXAD);                 // va 0x800000.. -> pa 0x0..
	mem_write32(&mem, 0x60000, ADDI_X1_5);
	step_at(0x00860000);
	TEST_ASSERT_EQUAL_UINT32(5, cpu_read_reg(&cpu, 1));
	TEST_ASSERT_EQUAL_HEX32(0x00860004, cpu.pc);
}

void test_a7_m_mode_fetches_physically(void) {
	cpu.privilege_mode = PRV_M;
	mem_write32(&mem, 0x5000, ADDI_X1_7);              // what a physical fetch sees
	mem_write32(&mem, 0x50000, ADDI_X1_5);             // what a translated fetch would see
	step_at(0x5000);
	TEST_ASSERT_EQUAL_UINT32(7, cpu_read_reg(&cpu, 1));
}

/* ---- B. Fetch faults -------------------------------------------------------------------------- */

void test_b1_unmapped_page(void) {
	step_at(0x7000);
	expect_trap(0x7000, 12, 0x7000);
}

void test_b2_page_without_execute(void) {
	map(6, 0x60, V | R | W | A | D);
	step_at(0x6000);
	expect_trap(0x6000, 12, 0x6000);
}

void test_b3_s_mode_on_a_user_page(void) {
	map(6, 0x60, V | U | RXAD);
	mem_write32(&mem, 0x60000, ADDI_X1_5);
	step_at(0x6000);
	expect_trap(0x6000, 12, 0x6000);
}

void test_b4_s_mode_on_a_user_page_even_with_sum(void) {
	map(6, 0x60, V | U | RXAD);
	cpu_write_csr(&cpu, CSR_MSTATUS, MSTATUS_SUM);
	mem_write32(&mem, 0x60000, ADDI_X1_5);
	step_at(0x6000);
	expect_trap(0x6000, 12, 0x6000);
}

void test_b5_u_mode_on_a_supervisor_page(void) {
	cpu.privilege_mode = PRV_U;
	mem_write32(&mem, 0x50000, ADDI_X1_5);
	step_at(0x5000);                                   // virtual page 5 has U = 0
	expect_trap(0x5000, 12, 0x5000);
}

void test_b6_mxr_does_not_make_a_page_executable(void) {
	map(6, 0x60, V | R | A | D);
	cpu_write_csr(&cpu, CSR_MSTATUS, MSTATUS_MXR);
	step_at(0x6000);
	expect_trap(0x6000, 12, 0x6000);
}

void test_b7_misaligned_megapage(void) {
	set_pte(ROOT, 3, 0x801, V | RXAD);
	step_at(0x00C00000);
	expect_trap(0x00C00000, 12, 0x00C00000);
}

void test_b8_delegated_page_fault(void) {
	cpu_write_csr(&cpu, CSR_MEDELEG, 1u << 12);
	step_at(0x7000);
	TEST_ASSERT_EQUAL_HEX32(STVEC, cpu.pc);
	TEST_ASSERT_EQUAL_UINT32(12, cpu_read_csr(&cpu, CSR_SCAUSE));
	TEST_ASSERT_EQUAL_HEX32(0x7000, cpu_read_csr(&cpu, CSR_STVAL));
	TEST_ASSERT_EQUAL_HEX32(0x7000, cpu_read_csr(&cpu, CSR_SEPC));
	TEST_ASSERT_EQUAL_INT(PRV_S, cpu.privilege_mode);
}

void test_b9_pmp_denies_the_pte_read(void) {
	cpu.pmpcfg[0] = 0x10;                              // entry 0: NA4, no permissions, exactly root[0]
	cpu.pmpaddr[0] = ROOT >> 2;
	cpu.pmpcfg[1] = 0x1F;
	cpu.pmpaddr[1] = 0x7FFFFFFF;
	step_at(0x5000);
	expect_trap(0x5000, 1, 0x5000);
}

void test_b10_pmp_denies_execute(void) {
	mem_write32(&mem, 0x50000, ADDI_X1_5);
	pmp_page_only(0x50000, PMP_R | PMP_W);
	step_at(0x5000);
	expect_trap(0x5000, 1, 0x5000);
}

void test_b11_leaf_outside_memory(void) {
	map(6, 0x1000, V | RXAD);                          // physical page 0x1000 is past the 1 MiB of memory
	step_at(0x6000);
	expect_trap(0x6000, 1, 0x6000);
}

void test_b12_leaf_above_4gib(void) {
	map(6, 0x300000, V | RXAD);
	step_at(0x6000);
	expect_trap(0x6000, 1, 0x6000);
}

void test_b13_a_faulting_fetch_does_not_retire(void) {
	uint64_t instret = cpu.minstret, cycle = cpu.mcycle;
	step_at(0x7000);
	TEST_ASSERT_EQUAL_UINT64(instret, cpu.minstret);
	TEST_ASSERT_EQUAL_UINT64(cycle + 1, cpu.mcycle);
}

/* ---- C. A 32-bit instruction across a page boundary ------------------------------------------- */

// Virtual page 0x14 -> physical page 0x90, virtual page 0x15 -> physical page 0x20 (not contiguous).
// ADDI x1, x0, 5 = 0x00500093: low half 0x0093 at the end of the first page, high half 0x0050 in the second.
static void map_split(uint32_t second_flags) {
	map(0x14, 0x90, V | RXAD);
	if (second_flags) {
		map(0x15, 0x20, second_flags);
	}
	mem_write16(&mem, 0x90FFE, 0x0093);
	mem_write16(&mem, 0x20000, 0x0050);
}

void test_c1_both_pages_executable(void) {
	map_split(V | RXAD);
	step_at(0x14FFE);
	TEST_ASSERT_EQUAL_UINT32(5, cpu_read_reg(&cpu, 1));
	TEST_ASSERT_EQUAL_HEX32(0x15002, cpu.pc);
}

void test_c2_second_page_unmapped(void) {
	map_split(0);
	step_at(0x14FFE);
	expect_trap(0x14FFE, 12, 0x15000);
}

void test_c3_second_page_not_executable(void) {
	map_split(V | R | A | D);
	step_at(0x14FFE);
	expect_trap(0x14FFE, 12, 0x15000);
}

void test_c4_first_page_unmapped(void) {
	map(0x15, 0x20, V | RXAD);
	step_at(0x14FFE);
	expect_trap(0x14FFE, 12, 0x14FFE);
}

void test_c5_pmp_denies_execute_on_the_second_page(void) {
	map_split(V | RXAD);
	pmp_page_only(0x20000, PMP_R | PMP_W);
	step_at(0x14FFE);
	expect_trap(0x14FFE, 1, 0x15000);
}

void test_c6_second_page_outside_memory(void) {
	map_split(V | RXAD);
	map(0x15, 0x1000, V | RXAD);
	step_at(0x14FFE);
	expect_trap(0x14FFE, 1, 0x15000);
}

void test_c7_compressed_needs_no_second_page(void) {
	map_split(0);
	cpu_write_reg(&cpu, 1, 0);                         // C.ADDI adds to x1
	mem_write16(&mem, 0x90FFE, C_ADDI_X1_5);
	step_at(0x14FFE);
	TEST_ASSERT_EQUAL_UINT32(5, cpu_read_reg(&cpu, 1));
	TEST_ASSERT_EQUAL_HEX32(0x15000, cpu.pc);
}

/* ---- D. MPRV does not apply to fetches -------------------------------------------------------- */

void test_d1_mprv_is_ignored_for_fetch(void) {
	cpu.privilege_mode = PRV_M;
	cpu_write_csr(&cpu, CSR_MSTATUS, MSTATUS_MPRV | ((uint32_t)PRV_S << 11));
	mem_write32(&mem, 0x5000, ADDI_X1_1);              // a physical fetch
	mem_write32(&mem, 0x50000, ADDI_X1_2);             // a translated fetch
	step_at(0x5000);
	TEST_ASSERT_EQUAL_UINT32(1, cpu_read_reg(&cpu, 1));
}

int main(void) {
	UNITY_BEGIN();
	RUN_TEST(test_a1_fetch_32_bit);
	RUN_TEST(test_a2_fetch_compressed);
	RUN_TEST(test_a3_page_offset_is_kept);
	RUN_TEST(test_a4_u_mode_on_a_user_page);
	RUN_TEST(test_a5_fetch_sets_accessed_not_dirty);
	RUN_TEST(test_a6_megapage);
	RUN_TEST(test_a7_m_mode_fetches_physically);
	RUN_TEST(test_b1_unmapped_page);
	RUN_TEST(test_b2_page_without_execute);
	RUN_TEST(test_b3_s_mode_on_a_user_page);
	RUN_TEST(test_b4_s_mode_on_a_user_page_even_with_sum);
	RUN_TEST(test_b5_u_mode_on_a_supervisor_page);
	RUN_TEST(test_b6_mxr_does_not_make_a_page_executable);
	RUN_TEST(test_b7_misaligned_megapage);
	RUN_TEST(test_b8_delegated_page_fault);
	RUN_TEST(test_b9_pmp_denies_the_pte_read);
	RUN_TEST(test_b10_pmp_denies_execute);
	RUN_TEST(test_b11_leaf_outside_memory);
	RUN_TEST(test_b12_leaf_above_4gib);
	RUN_TEST(test_b13_a_faulting_fetch_does_not_retire);
	RUN_TEST(test_c1_both_pages_executable);
	RUN_TEST(test_c2_second_page_unmapped);
	RUN_TEST(test_c3_second_page_not_executable);
	RUN_TEST(test_c4_first_page_unmapped);
	RUN_TEST(test_c5_pmp_denies_execute_on_the_second_page);
	RUN_TEST(test_c6_second_page_outside_memory);
	RUN_TEST(test_c7_compressed_needs_no_second_page);
	RUN_TEST(test_d1_mprv_is_ignored_for_fetch);
	return UNITY_END();
}
