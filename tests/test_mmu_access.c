#include "unity.h"
#include "cpu_test_util.h"
#include "../src/execute.h"

#define ROOT   0x1000u
#define L0     0x2000u
#define PC     0x200u    // where the instruction under test "is"
#define MTVEC  0x300u
#define STVEC  0x400u

#define V 0x01u
#define R 0x02u
#define W 0x04u
#define X 0x08u
#define U 0x10u
#define A 0x40u
#define D 0x80u
#define RWAD (R | W | A | D)

#define SATP_SV32 0x80000000u

// Registers: x1 = rd, x2 = address, x3 = store data / AMO operand
#define RD   1
#define ADDR 2
#define SRC  3

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

// Sv32 on, S-mode, root[0] -> L0, and virtual page 5 -> physical page 0x50 (not a user page)
void setUp(void) {
	mem_init(&mem, 0x100000);
	test_cpu_reset_in_mode(&cpu, PRV_S);
	cpu_write_csr(&cpu, CSR_SATP, SATP_SV32 | (ROOT >> 12));
	cpu_write_csr(&cpu, CSR_MTVEC, MTVEC);
	cpu_write_csr(&cpu, CSR_STVEC, STVEC);
	set_pte(ROOT, 0, L0 >> 12, V);
	map(5, 0x50, V | R | W | X | A | D);
}

void tearDown(void) {
	mem_free(&mem);
}

/* ---- instruction encodings (immediate 0) ---------------------------------------------------- */

static uint32_t enc_load(uint32_t funct3)  { return (ADDR << 15) | (funct3 << 12) | (RD << 7) | 0x03; }
static uint32_t enc_store(uint32_t funct3) { return (SRC << 20) | (ADDR << 15) | (funct3 << 12) | 0x23; }
#define LB  enc_load(0)
#define LH  enc_load(1)
#define LW  enc_load(2)
#define LBU enc_load(4)
#define LHU enc_load(5)
#define SB  enc_store(0)
#define SH  enc_store(1)
#define SW  enc_store(2)
#define AMOADD_W 0x003120AFu   // AMOADD.W x1, x3, (x2)
#define LR_W     0x100120AFu   // LR.W x1, (x2)
#define SC_W     0x183120AFu   // SC.W x1, x3, (x2)

// Executes one instruction at PC with the address register already holding va
static void run(uint32_t raw, uint32_t va) {
	cpu_write_reg(&cpu, ADDR, va);
	cpu.pc = PC;
	DecodedInstruction instr = decode_instruction(raw);
	execute(&cpu, &mem, &instr);
}

static void expect_trap(uint32_t cause, uint32_t tval) {
	TEST_ASSERT_EQUAL_HEX32(MTVEC, cpu.pc);
	TEST_ASSERT_EQUAL_UINT32(cause, cpu_read_csr(&cpu, CSR_MCAUSE));
	TEST_ASSERT_EQUAL_HEX32(tval, cpu_read_csr(&cpu, CSR_MTVAL));
	TEST_ASSERT_EQUAL_HEX32(PC, cpu_read_csr(&cpu, CSR_MEPC));
	TEST_ASSERT_EQUAL_INT(PRV_M, cpu.privilege_mode);
}

static void expect_no_trap(void) {
	TEST_ASSERT_EQUAL_HEX32(PC + 4, cpu.pc);
}

static void fill(uint32_t pa, uint32_t len, uint8_t byte) {
	for (uint32_t i = 0; i < len; i++) {
		mem_write8(&mem, pa + i, byte);
	}
}

/* ---- A. Translated loads and stores ----------------------------------------------------------- */

void test_a1_lw(void) {
	mem_write32(&mem, 0x50004, 0xDEADBEEF);
	run(LW, 0x5004);
	expect_no_trap();
	TEST_ASSERT_EQUAL_HEX32(0xDEADBEEF, cpu_read_reg(&cpu, RD));
}

void test_a2_a3_lb_lbu(void) {
	mem_write8(&mem, 0x50010, 0x80);
	run(LB, 0x5010);
	TEST_ASSERT_EQUAL_HEX32(0xFFFFFF80, cpu_read_reg(&cpu, RD));
	run(LBU, 0x5010);
	TEST_ASSERT_EQUAL_HEX32(0x80, cpu_read_reg(&cpu, RD));
}

void test_a4_a5_lh_lhu(void) {
	mem_write16(&mem, 0x50020, 0x8001);
	run(LH, 0x5020);
	TEST_ASSERT_EQUAL_HEX32(0xFFFF8001, cpu_read_reg(&cpu, RD));
	run(LHU, 0x5020);
	TEST_ASSERT_EQUAL_HEX32(0x8001, cpu_read_reg(&cpu, RD));
}

void test_a6_stores(void) {
	fill(0x500F0, 0x30, 0xEE);
	cpu_write_reg(&cpu, SRC, 0x11223344);
	run(SW, 0x5100);
	expect_no_trap();
	TEST_ASSERT_EQUAL_HEX32(0x11223344, mem_read32(&mem, 0x50100));
	TEST_ASSERT_EQUAL_HEX8(0xEE, mem_read8(&mem, 0x50104));
	TEST_ASSERT_EQUAL_HEX8(0xEE, mem_read8(&mem, 0x500FF));

	run(SH, 0x5108);
	TEST_ASSERT_EQUAL_HEX8(0x44, mem_read8(&mem, 0x50108));
	TEST_ASSERT_EQUAL_HEX8(0x33, mem_read8(&mem, 0x50109));
	TEST_ASSERT_EQUAL_HEX8(0xEE, mem_read8(&mem, 0x5010A));
	TEST_ASSERT_EQUAL_HEX8(0xEE, mem_read8(&mem, 0x50107));

	run(SB, 0x510C);
	TEST_ASSERT_EQUAL_HEX8(0x44, mem_read8(&mem, 0x5010C));
	TEST_ASSERT_EQUAL_HEX8(0xEE, mem_read8(&mem, 0x5010D));
	TEST_ASSERT_EQUAL_HEX8(0xEE, mem_read8(&mem, 0x5010B));
}

void test_a7_megapage(void) {
	set_pte(ROOT, 2, 0x000, V | R | W | A | D);        // va 0x800000.. -> pa 0x0..: a 4 MiB megapage
	mem_write32(&mem, 0x60008, 0xCAFEBABE);
	run(LW, 0x00860008);
	TEST_ASSERT_EQUAL_HEX32(0xCAFEBABE, cpu_read_reg(&cpu, RD));
	cpu_write_reg(&cpu, SRC, 0x01020304);
	run(SW, 0x00860010);
	TEST_ASSERT_EQUAL_HEX32(0x01020304, mem_read32(&mem, 0x60010));
}

void test_a8_load_sets_accessed(void) {
	map(6, 0x60, V | R | W);
	run(LW, 0x6000);
	expect_no_trap();
	TEST_ASSERT_EQUAL_HEX32((0x60u << 10) | V | R | W | A, get_pte(L0, 6));
}

void test_a9_store_sets_accessed_and_dirty(void) {
	map(6, 0x60, V | R | W);
	run(SW, 0x6000);
	expect_no_trap();
	TEST_ASSERT_EQUAL_HEX32((0x60u << 10) | V | R | W | A | D, get_pte(L0, 6));
}

/* ---- B. Faults -------------------------------------------------------------------------------- */

void test_b1_load_page_fault(void) {
	cpu_write_reg(&cpu, RD, 0x12345678);
	run(LW, 0x7004);
	expect_trap(13, 0x7004);
	TEST_ASSERT_EQUAL_HEX32(0x12345678, cpu_read_reg(&cpu, RD));
}

void test_b2_store_page_fault(void) {
	fill(0x70000, 8, 0xEE);
	run(SW, 0x7004);
	expect_trap(15, 0x7004);
	TEST_ASSERT_EQUAL_HEX8(0xEE, mem_read8(&mem, 0x70004));
}

void test_b3_store_to_read_only_page(void) {
	map(6, 0x60, V | R | A | D);
	run(SW, 0x6000);
	expect_trap(15, 0x6000);
}

void test_b4_load_from_execute_only_page(void) {
	map(6, 0x60, V | X | A | D);
	run(LW, 0x6000);
	expect_trap(13, 0x6000);
}

void test_b5_mxr_makes_execute_only_readable(void) {
	map(6, 0x60, V | X | A | D);
	cpu_write_csr(&cpu, CSR_MSTATUS, MSTATUS_MXR);
	mem_write32(&mem, 0x60000, 0x1234ABCD);
	run(LW, 0x6000);
	expect_no_trap();
	TEST_ASSERT_EQUAL_HEX32(0x1234ABCD, cpu_read_reg(&cpu, RD));
}

void test_b6_s_mode_load_of_user_page_without_sum(void) {
	map(6, 0x60, V | R | W | U | A | D);
	run(LW, 0x6000);
	expect_trap(13, 0x6000);
}

void test_b7_s_mode_load_of_user_page_with_sum(void) {
	map(6, 0x60, V | R | W | U | A | D);
	cpu_write_csr(&cpu, CSR_MSTATUS, MSTATUS_SUM);
	mem_write32(&mem, 0x60000, 0x55AA55AA);
	run(LW, 0x6000);
	expect_no_trap();
	TEST_ASSERT_EQUAL_HEX32(0x55AA55AA, cpu_read_reg(&cpu, RD));
}

void test_b8_pmp_denies_the_pte_read(void) {
	cpu.pmpcfg[0] = 0x10;                      // entry 0: NA4, no permissions, exactly root[0]
	cpu.pmpaddr[0] = ROOT >> 2;
	cpu.pmpcfg[1] = 0x1F;
	cpu.pmpaddr[1] = 0x7FFFFFFF;
	run(LW, 0x5004);
	expect_trap(5, 0x5004);
}

void test_b9_leaf_above_4gib(void) {
	map(6, 0x300000, V | R | A | D);
	run(LW, 0x6000);
	expect_trap(5, 0x6000);
}

void test_b10_pmp_denies_the_store(void) {
	cpu.pmpcfg[0] = 0x19;          // entry 0: NAPOT 4 KiB at pa 0x50000, read only
	cpu.pmpaddr[0] = (0x50000u >> 2) | 0x1FF;
	cpu.pmpcfg[1] = 0x1F;
	cpu.pmpaddr[1] = 0x7FFFFFFF;
	run(SW, 0x5000);
	expect_trap(7, 0x5000);
	run(LW, 0x5000);
	expect_no_trap();
}

void test_b11_u_mode_on_a_supervisor_page(void) {
	cpu.privilege_mode = PRV_U;
	run(LW, 0x5004);                           // virtual page 5 has U = 0
	TEST_ASSERT_EQUAL_HEX32(MTVEC, cpu.pc);
	TEST_ASSERT_EQUAL_UINT32(13, cpu_read_csr(&cpu, CSR_MCAUSE));
	TEST_ASSERT_EQUAL_HEX32(0x5004, cpu_read_csr(&cpu, CSR_MTVAL));
}

void test_b12_delegated_page_fault(void) {
	cpu_write_csr(&cpu, CSR_MEDELEG, 1u << 13);
	run(LW, 0x7004);
	TEST_ASSERT_EQUAL_HEX32(STVEC, cpu.pc);
	TEST_ASSERT_EQUAL_UINT32(13, cpu_read_csr(&cpu, CSR_SCAUSE));
	TEST_ASSERT_EQUAL_HEX32(0x7004, cpu_read_csr(&cpu, CSR_STVAL));
	TEST_ASSERT_EQUAL_HEX32(PC, cpu_read_csr(&cpu, CSR_SEPC));
	TEST_ASSERT_EQUAL_INT(PRV_S, cpu.privilege_mode);
}

/* ---- C. MPRV ---------------------------------------------------------------------------------- */

static void set_mprv(PrivMode mpp) {
	cpu.privilege_mode = PRV_M;
	cpu_write_csr(&cpu, CSR_MSTATUS, MSTATUS_MPRV | ((uint32_t)mpp << 11));
	mem_write32(&mem, 0x50004, 0xAAAAAAAA);    // what a translated load of va 0x5004 sees
	mem_write32(&mem, 0x5004, 0xBBBBBBBB);     // what an untranslated load sees
}

void test_c1_mprv_with_mpp_s_translates(void) {
	set_mprv(PRV_S);
	run(LW, 0x5004);
	expect_no_trap();
	TEST_ASSERT_EQUAL_HEX32(0xAAAAAAAA, cpu_read_reg(&cpu, RD));
}

void test_c2_mprv_with_mpp_m_does_not_translate(void) {
	set_mprv(PRV_M);
	run(LW, 0x5004);
	TEST_ASSERT_EQUAL_HEX32(0xBBBBBBBB, cpu_read_reg(&cpu, RD));
}

void test_c3_mprv_with_mpp_u_checks_the_user_bit(void) {
	set_mprv(PRV_U);
	run(LW, 0x5004);                           // virtual page 5 has U = 0
	expect_trap(13, 0x5004);
}

void test_c4_mprv_with_bare_satp(void) {
	set_mprv(PRV_S);
	cpu_write_csr(&cpu, CSR_SATP, 0);
	run(LW, 0x5004);
	TEST_ASSERT_EQUAL_HEX32(0xBBBBBBBB, cpu_read_reg(&cpu, RD));
}

void test_c5_m_mode_without_mprv_ignores_satp(void) {
	set_mprv(PRV_S);
	cpu_write_csr(&cpu, CSR_MSTATUS, 0);
	run(LW, 0x5004);
	TEST_ASSERT_EQUAL_HEX32(0xBBBBBBBB, cpu_read_reg(&cpu, RD));
}

/* ---- D. Accesses that cross a page boundary --------------------------------------------------- */

// Virtual page 0x14 -> physical page 0x90, virtual page 0x15 -> physical page 0x20 (not contiguous)
static void map_split(uint32_t second_flags) {
	map(0x14, 0x90, V | R | W | A | D);
	if (second_flags) {
		map(0x15, 0x20, second_flags);
	}
	mem_write8(&mem, 0x90FFC, 0xA0); mem_write8(&mem, 0x90FFD, 0xA1);
	mem_write8(&mem, 0x90FFE, 0xA2); mem_write8(&mem, 0x90FFF, 0xA3);
	mem_write8(&mem, 0x20000, 0xB0); mem_write8(&mem, 0x20001, 0xB1);
	mem_write8(&mem, 0x20002, 0xB2); mem_write8(&mem, 0x20003, 0xB3);
}

void test_d1_lw_split_2_2(void) {
	map_split(V | R | W | A | D);
	run(LW, 0x14FFE);
	expect_no_trap();
	TEST_ASSERT_EQUAL_HEX32(0xB1B0A3A2, cpu_read_reg(&cpu, RD));
}

void test_d2_lw_split_1_3(void) {
	map_split(V | R | W | A | D);
	run(LW, 0x14FFF);
	TEST_ASSERT_EQUAL_HEX32(0xB2B1B0A3, cpu_read_reg(&cpu, RD));
}

void test_d3_lw_split_3_1(void) {
	map_split(V | R | W | A | D);
	run(LW, 0x14FFD);
	TEST_ASSERT_EQUAL_HEX32(0xB0A3A2A1, cpu_read_reg(&cpu, RD));
}

void test_d4_lh_split_1_1(void) {
	map_split(V | R | W | A | D);
	run(LH, 0x14FFF);
	TEST_ASSERT_EQUAL_HEX32(0xFFFFB0A3, cpu_read_reg(&cpu, RD));
}

void test_d5_sw_split(void) {
	map_split(V | R | W | A | D);
	cpu_write_reg(&cpu, SRC, 0x11223344);
	run(SW, 0x14FFE);
	expect_no_trap();
	TEST_ASSERT_EQUAL_HEX8(0x44, mem_read8(&mem, 0x90FFE));
	TEST_ASSERT_EQUAL_HEX8(0x33, mem_read8(&mem, 0x90FFF));
	TEST_ASSERT_EQUAL_HEX8(0x22, mem_read8(&mem, 0x20000));
	TEST_ASSERT_EQUAL_HEX8(0x11, mem_read8(&mem, 0x20001));
	TEST_ASSERT_EQUAL_HEX8(0xB2, mem_read8(&mem, 0x20002));   // beyond the access: untouched
}

void test_d6_load_second_page_unmapped(void) {
	map_split(0);
	run(LW, 0x14FFE);
	expect_trap(13, 0x15000);
}

void test_d7_load_first_page_unmapped(void) {
	map(0x15, 0x20, V | R | W | A | D);
	run(LW, 0x14FFE);
	expect_trap(13, 0x14FFE);
}

void test_d8_store_second_page_unmapped_leaves_first_untouched(void) {
	map_split(0);
	cpu_write_reg(&cpu, SRC, 0x11223344);
	run(SW, 0x14FFE);
	expect_trap(15, 0x15000);
	TEST_ASSERT_EQUAL_HEX8(0xA2, mem_read8(&mem, 0x90FFE));
	TEST_ASSERT_EQUAL_HEX8(0xA3, mem_read8(&mem, 0x90FFF));
}

void test_d9_store_second_page_read_only(void) {
	map_split(V | R | A | D);
	cpu_write_reg(&cpu, SRC, 0x11223344);
	run(SW, 0x14FFE);
	expect_trap(15, 0x15000);
	TEST_ASSERT_EQUAL_HEX8(0xA2, mem_read8(&mem, 0x90FFE));
}

void test_d10_store_second_page_outside_memory(void) {
	map_split(0);
	map(0x15, 0x1000, V | R | W | A | D);               // physical page 0x1000 is past the 1 MiB of memory
	cpu_write_reg(&cpu, SRC, 0x11223344);
	run(SW, 0x14FFE);
	expect_trap(7, 0x15000);
	TEST_ASSERT_EQUAL_HEX8(0xA2, mem_read8(&mem, 0x90FFE));
	TEST_ASSERT_EQUAL_HEX8(0xA3, mem_read8(&mem, 0x90FFF));
}

void test_d11_paging_off_is_one_physical_access(void) {
	cpu_write_csr(&cpu, CSR_SATP, 0);
	mem_write8(&mem, 0xFFE, 0x01); mem_write8(&mem, 0xFFF, 0x02);
	mem_write8(&mem, 0x1000, 0x03); mem_write8(&mem, 0x1001, 0x04);
	run(LW, 0xFFE);
	expect_no_trap();
	TEST_ASSERT_EQUAL_HEX32(0x04030201, cpu_read_reg(&cpu, RD));
}

/* ---- E. Atomics ------------------------------------------------------------------------------- */

void test_e1_amoadd(void) {
	mem_write32(&mem, 0x50040, 5);
	cpu_write_reg(&cpu, SRC, 3);
	run(AMOADD_W, 0x5040);
	expect_no_trap();
	TEST_ASSERT_EQUAL_UINT32(5, cpu_read_reg(&cpu, RD));
	TEST_ASSERT_EQUAL_UINT32(8, mem_read32(&mem, 0x50040));
}

void test_e2_amo_on_read_only_page_is_a_store_fault(void) {
	map(6, 0x60, V | R | A | D);
	run(AMOADD_W, 0x6000);
	expect_trap(15, 0x6000);
}

void test_e3_amo_on_unmapped_page(void) {
	run(AMOADD_W, 0x7000);
	expect_trap(15, 0x7000);
}

void test_e4_amo_on_execute_only_page_with_mxr(void) {
	map(8, 0x80, V | X | A | D);
	cpu_write_csr(&cpu, CSR_MSTATUS, MSTATUS_MXR);
	run(AMOADD_W, 0x8000);
	expect_trap(15, 0x8000);
}

void test_e5_lr_reserves_the_physical_address(void) {
	run(LR_W, 0x5040);
	expect_no_trap();
	TEST_ASSERT_TRUE(cpu.lr_valid);
	TEST_ASSERT_EQUAL_HEX32(0x50040, cpu.lr_addr);
}

void test_e6_sc_through_an_alias_succeeds(void) {
	map(9, 0x50, V | R | W | A | D);                    // a second virtual page onto physical page 0x50
	run(LR_W, 0x5040);
	cpu_write_reg(&cpu, SRC, 0x99);
	run(SC_W, 0x9040);
	expect_no_trap();
	TEST_ASSERT_EQUAL_UINT32(0, cpu_read_reg(&cpu, RD));
	TEST_ASSERT_EQUAL_UINT32(0x99, mem_read32(&mem, 0x50040));
}

void test_e7_sc_at_another_address_fails(void) {
	mem_write32(&mem, 0x50080, 0x77);
	run(LR_W, 0x5040);
	cpu_write_reg(&cpu, SRC, 0x99);
	run(SC_W, 0x5080);
	expect_no_trap();
	TEST_ASSERT_EQUAL_UINT32(1, cpu_read_reg(&cpu, RD));
	TEST_ASSERT_EQUAL_UINT32(0x77, mem_read32(&mem, 0x50080));
}

void test_e8_sc_to_unmapped_page_faults_without_a_reservation(void) {
	run(SC_W, 0x7000);
	expect_trap(15, 0x7000);
}

void test_e9_lr_on_unmapped_page(void) {
	run(LR_W, 0x7000);
	expect_trap(13, 0x7000);
}

void test_e10_lr_misaligned_beats_translation(void) {
	run(LR_W, 0x7002);                                  // unmapped too, but misaligned is checked first
	expect_trap(4, 0x7002);
}

void test_e11_sc_and_amo_misaligned_beat_translation(void) {
	run(SC_W, 0x7002);
	expect_trap(6, 0x7002);
	run(AMOADD_W, 0x7002);
	expect_trap(6, 0x7002);
}

int main(void) {
	UNITY_BEGIN();
	RUN_TEST(test_a1_lw);
	RUN_TEST(test_a2_a3_lb_lbu);
	RUN_TEST(test_a4_a5_lh_lhu);
	RUN_TEST(test_a6_stores);
	RUN_TEST(test_a7_megapage);
	RUN_TEST(test_a8_load_sets_accessed);
	RUN_TEST(test_a9_store_sets_accessed_and_dirty);
	RUN_TEST(test_b1_load_page_fault);
	RUN_TEST(test_b2_store_page_fault);
	RUN_TEST(test_b3_store_to_read_only_page);
	RUN_TEST(test_b4_load_from_execute_only_page);
	RUN_TEST(test_b5_mxr_makes_execute_only_readable);
	RUN_TEST(test_b6_s_mode_load_of_user_page_without_sum);
	RUN_TEST(test_b7_s_mode_load_of_user_page_with_sum);
	RUN_TEST(test_b8_pmp_denies_the_pte_read);
	RUN_TEST(test_b9_leaf_above_4gib);
	RUN_TEST(test_b10_pmp_denies_the_store);
	RUN_TEST(test_b11_u_mode_on_a_supervisor_page);
	RUN_TEST(test_b12_delegated_page_fault);
	RUN_TEST(test_c1_mprv_with_mpp_s_translates);
	RUN_TEST(test_c2_mprv_with_mpp_m_does_not_translate);
	RUN_TEST(test_c3_mprv_with_mpp_u_checks_the_user_bit);
	RUN_TEST(test_c4_mprv_with_bare_satp);
	RUN_TEST(test_c5_m_mode_without_mprv_ignores_satp);
	RUN_TEST(test_d1_lw_split_2_2);
	RUN_TEST(test_d2_lw_split_1_3);
	RUN_TEST(test_d3_lw_split_3_1);
	RUN_TEST(test_d4_lh_split_1_1);
	RUN_TEST(test_d5_sw_split);
	RUN_TEST(test_d6_load_second_page_unmapped);
	RUN_TEST(test_d7_load_first_page_unmapped);
	RUN_TEST(test_d8_store_second_page_unmapped_leaves_first_untouched);
	RUN_TEST(test_d9_store_second_page_read_only);
	RUN_TEST(test_d10_store_second_page_outside_memory);
	RUN_TEST(test_d11_paging_off_is_one_physical_access);
	RUN_TEST(test_e1_amoadd);
	RUN_TEST(test_e2_amo_on_read_only_page_is_a_store_fault);
	RUN_TEST(test_e3_amo_on_unmapped_page);
	RUN_TEST(test_e4_amo_on_execute_only_page_with_mxr);
	RUN_TEST(test_e5_lr_reserves_the_physical_address);
	RUN_TEST(test_e6_sc_through_an_alias_succeeds);
	RUN_TEST(test_e7_sc_at_another_address_fails);
	RUN_TEST(test_e8_sc_to_unmapped_page_faults_without_a_reservation);
	RUN_TEST(test_e9_lr_on_unmapped_page);
	RUN_TEST(test_e10_lr_misaligned_beats_translation);
	RUN_TEST(test_e11_sc_and_amo_misaligned_beat_translation);
	return UNITY_END();
}
