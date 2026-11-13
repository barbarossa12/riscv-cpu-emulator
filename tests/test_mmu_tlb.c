#include "unity.h"
#include "cpu_test_util.h"
#include "../src/execute.h"
#include "../src/mmu.h"

#define ROOT   0x1000u
#define L0     0x2000u
#define ROOT_B 0x3000u   // a second address space
#define L0_B   0x4000u
#define MTVEC  0x300u

#define V 0x01u
#define R 0x02u
#define W 0x04u
#define X 0x08u
#define U 0x10u
#define G 0x20u
#define A 0x40u
#define D 0x80u
#define RWXAD (R | W | X | A | D)

#define SATP_SV32 0x80000000u

static CPU cpu;
static Memory mem;
static uint32_t pa;

static void set_pte(uint32_t table, uint32_t index, uint32_t ppn, uint32_t flags) {
	mem_write32(&mem, table + index * 4, (ppn << 10) | flags);
}

static uint32_t get_pte(uint32_t table, uint32_t index) {
	return mem_read32(&mem, table + index * 4);
}

static void map(uint32_t vpn, uint32_t ppn, uint32_t flags) {
	set_pte(L0, vpn, ppn, flags);
}

static void set_satp(uint32_t root, uint32_t asid) {
	cpu_write_csr(&cpu, CSR_SATP, SATP_SV32 | (asid << 22) | (root >> 12));
}

static XlateResult xl(uint32_t va, AccessType type, PrivMode priv) {
	pa = 0xFFFFFFFF;
	return mmu_translate(&cpu, &mem, va, type, priv, &pa);
}

static void expect_load(uint32_t va, uint32_t want_pa) {
	TEST_ASSERT_EQUAL(XLATE_OK, xl(va, ACCESS_LOAD, PRV_S));
	TEST_ASSERT_EQUAL_HEX32(want_pa, pa);
}

static void flush_all(void) {
	mmu_flush(&cpu, 0, 0, true, true);
}

// Sv32 on (ASID 1), S-mode, root[0] -> L0, virtual page 5 -> physical page 0x50 (all permissions, not a user page)
void setUp(void) {
	mem_init(&mem, 0x100000);
	test_cpu_reset_in_mode(&cpu, PRV_S);
	cpu_write_csr(&cpu, CSR_MTVEC, MTVEC);
	set_satp(ROOT, 1);
	set_pte(ROOT, 0, L0 >> 12, V);
	map(5, 0x50, V | RWXAD);
}

void tearDown(void) {
	mem_free(&mem);
}

/* ---- A. The cache is used, and a fence empties it ---------------------------------------------- */

void test_a1_a_cached_entry_is_used(void) {
	expect_load(0x5000, 0x50000);
	map(5, 0x60, V | RWXAD);
	expect_load(0x5000, 0x50000);                      // stale: the walk is skipped (legal until a fence)
}

void test_a2_a_fence_drops_it(void) {
	expect_load(0x5000, 0x50000);
	map(5, 0x60, V | RWXAD);
	flush_all();
	expect_load(0x5000, 0x60000);
}

void test_a3_a_fault_is_not_cached(void) {
	TEST_ASSERT_EQUAL(XLATE_PAGE_FAULT, xl(0x6000, ACCESS_LOAD, PRV_S));
	map(6, 0x60, V | RWXAD);
	expect_load(0x6000, 0x60000);                      // no fence needed
}

void test_a4_an_invalidated_page_stays_until_a_fence(void) {
	expect_load(0x5000, 0x50000);
	map(5, 0x50, 0);                                   // V = 0
	expect_load(0x5000, 0x50000);
	flush_all();
	TEST_ASSERT_EQUAL(XLATE_PAGE_FAULT, xl(0x5000, ACCESS_LOAD, PRV_S));
}

void test_a5_a_cached_megapage(void) {
	set_pte(ROOT, 2, 0x000, V | RWXAD);
	expect_load(0x00812345, 0x12345);
	set_pte(ROOT, 2, 0x400, V | RWXAD);                // 4 MiB aligned, pa 0x400000
	expect_load(0x00812345, 0x12345);
	flush_all();
	expect_load(0x00812345, 0x412345);
}

/* ---- B. Permissions are checked again on every hit ---------------------------------------------- */

void test_b1_sum_takes_effect_at_once(void) {
	map(6, 0x60, V | R | W | U | A | D);
	cpu_write_csr(&cpu, CSR_MSTATUS, MSTATUS_SUM);
	expect_load(0x6000, 0x60000);                      // fills the cache
	cpu_write_csr(&cpu, CSR_MSTATUS, 0);
	TEST_ASSERT_EQUAL(XLATE_PAGE_FAULT, xl(0x6000, ACCESS_LOAD, PRV_S));
	cpu_write_csr(&cpu, CSR_MSTATUS, MSTATUS_SUM);
	expect_load(0x6000, 0x60000);
}

void test_b2_mxr_takes_effect_at_once(void) {
	map(6, 0x60, V | X | A | D);
	TEST_ASSERT_EQUAL(XLATE_PAGE_FAULT, xl(0x6000, ACCESS_LOAD, PRV_S));
	cpu_write_csr(&cpu, CSR_MSTATUS, MSTATUS_MXR);
	expect_load(0x6000, 0x60000);                      // fills the cache
	cpu_write_csr(&cpu, CSR_MSTATUS, 0);
	TEST_ASSERT_EQUAL(XLATE_PAGE_FAULT, xl(0x6000, ACCESS_LOAD, PRV_S));
}

void test_b3_the_privilege_is_checked_on_a_hit(void) {
	expect_load(0x5000, 0x50000);                      // S-mode fills the entry for a U = 0 page
	TEST_ASSERT_EQUAL(XLATE_PAGE_FAULT, xl(0x5000, ACCESS_LOAD, PRV_U));
}

void test_b4_a_store_to_a_cached_read_only_page(void) {
	map(6, 0x60, V | R | A | D);
	expect_load(0x6000, 0x60000);
	TEST_ASSERT_EQUAL(XLATE_PAGE_FAULT, xl(0x6000, ACCESS_STORE, PRV_S));
}

/* ---- C. Accessed and dirty bits through the cache ------------------------------------------------ */

void test_c1_a_store_after_a_load_sets_dirty(void) {
	map(6, 0x60, V | R | W);
	expect_load(0x6000, 0x60000);
	TEST_ASSERT_EQUAL_HEX32((0x60u << 10) | V | R | W | A, get_pte(L0, 6));
	TEST_ASSERT_EQUAL(XLATE_OK, xl(0x6000, ACCESS_STORE, PRV_S));
	TEST_ASSERT_EQUAL_HEX32((0x60u << 10) | V | R | W | A | D, get_pte(L0, 6));
}

void test_c2_dirty_already_set_in_memory(void) {
	map(6, 0x60, V | R | W);
	expect_load(0x6000, 0x60000);                      // cached with D = 0
	map(6, 0x60, V | R | W | A | D);                   // software sets it
	TEST_ASSERT_EQUAL(XLATE_OK, xl(0x6000, ACCESS_STORE, PRV_S));
	TEST_ASSERT_EQUAL_HEX32((0x60u << 10) | V | R | W | A | D, get_pte(L0, 6));
}

void test_c3_a_cached_dirty_entry_writes_nothing(void) {
	map(6, 0x60, V | R | W | A | D);
	TEST_ASSERT_EQUAL(XLATE_OK, xl(0x6000, ACCESS_STORE, PRV_S));
	map(6, 0x60, V | R | W);                           // software clears A and D, no fence
	TEST_ASSERT_EQUAL(XLATE_OK, xl(0x6000, ACCESS_STORE, PRV_S));
	TEST_ASSERT_EQUAL_HEX32((0x60u << 10) | V | R | W, get_pte(L0, 6));
}

/* ---- D. Address spaces --------------------------------------------------------------------------- */

static void build_second_space(void) {
	set_pte(ROOT_B, 0, L0_B >> 12, V);
	set_pte(L0_B, 5, 0x60, V | RWXAD);                 // the same virtual page, a different physical page
}

void test_d1_the_asid_selects_the_entry(void) {
	build_second_space();
	set_satp(ROOT, 1);
	expect_load(0x5000, 0x50000);
	set_satp(ROOT_B, 2);
	expect_load(0x5000, 0x60000);                      // no fence between the two
	set_satp(ROOT, 1);
	expect_load(0x5000, 0x50000);
}

void test_d2_an_entry_survives_a_switch_of_address_space(void) {
	build_second_space();
	set_pte(L0_B, 6, 0x70, V | RWXAD);
	set_satp(ROOT, 1);
	expect_load(0x5000, 0x50000);                      // ASID 1, slot of page 5
	set_satp(ROOT_B, 2);
	expect_load(0x6000, 0x70000);                      // ASID 2, a different slot
	map(5, 0x80, V | RWXAD);
	set_satp(ROOT, 1);
	expect_load(0x5000, 0x50000);                      // still the cached ASID 1 entry
}

/* ---- E. SFENCE.VMA ------------------------------------------------------------------------------- */

// Pages 5, 6 and 8 and 7: virtual page n -> physical page 16 * n. Page 6 is global.
// Filled: pages 5 and 6 under ASID 1, pages 7 and 8 under ASID 2.
static uint32_t page_flags(uint32_t n) {
	return V | RWXAD | (n == 6 ? G : 0);
}

static uint32_t pa_of(uint32_t n) {
	set_satp(ROOT, n <= 6 ? 1 : 2);
	TEST_ASSERT_EQUAL(XLATE_OK, xl(n << 12, ACCESS_LOAD, PRV_S));
	return pa;
}

static void fill_four(void) {
	for (uint32_t n = 5; n <= 8; n++) {
		set_pte(L0, n, 16 * n, page_flags(n));
	}
	for (uint32_t n = 5; n <= 8; n++) {
		TEST_ASSERT_EQUAL_HEX32(n * 0x10000, pa_of(n));
	}
	for (uint32_t n = 5; n <= 8; n++) {
		set_pte(L0, n, 16 * n + 0x10, page_flags(n));  // repoint every page, no fence
	}
}

// For each of pages 5 to 8: did the fence drop its entry, so that it now translates to the new page?
static void expect_dropped(bool d5, bool d6, bool d7, bool d8) {
	const bool dropped[4] = {d5, d6, d7, d8};
	for (uint32_t n = 5; n <= 8; n++) {
		uint32_t old_pa = n * 0x10000;
		TEST_ASSERT_EQUAL_HEX32(dropped[n - 5] ? old_pa + 0x10000 : old_pa, pa_of(n));
	}
}

// Executes SFENCE.VMA with rs1 = x10 (or x0) and rs2 = x11 (or x0)
static void sfence(bool use_rs1, uint32_t rs1_value, bool use_rs2, uint32_t rs2_value) {
	cpu_write_reg(&cpu, 10, rs1_value);
	cpu_write_reg(&cpu, 11, rs2_value);
	uint32_t raw = 0x12000073u | ((use_rs2 ? 11u : 0u) << 20) | ((use_rs1 ? 10u : 0u) << 15);
	DecodedInstruction instr = decode_instruction(raw);
	cpu.pc = 0x200;
	execute(&cpu, &mem, &instr);
	TEST_ASSERT_EQUAL_HEX32(0x204, cpu.pc);            // it did not trap
}

void test_e1_everything(void) {
	fill_four();
	sfence(false, 0, false, 0);
	expect_dropped(true, true, true, true);
}

void test_e2_one_asid_keeps_global_entries(void) {
	fill_four();
	sfence(false, 0, true, 1);
	expect_dropped(true, false, false, false);         // page 5 only: page 6 is global, 7 and 8 are ASID 2
}

void test_e3_one_page_in_any_asid(void) {
	fill_four();
	sfence(true, 0x5000, false, 0);
	expect_dropped(true, false, false, false);
}

void test_e3b_the_page_need_not_belong_to_the_current_asid(void) {
	fill_four();
	set_satp(ROOT, 1);                                 // the running ASID is 1, the page's is 2
	sfence(true, 0x7000, false, 0);
	expect_dropped(false, false, true, false);
}

void test_e4_one_page_in_one_asid(void) {
	fill_four();
	sfence(true, 0x5000, true, 2);                     // page 5 was filled under ASID 1
	expect_dropped(false, false, false, false);
	sfence(true, 0x5000, true, 1);
	expect_dropped(true, false, false, false);
}

void test_e5_global_entries(void) {
	fill_four();
	sfence(true, 0x6000, true, 1);                     // rs2 != x0: global entries are not flushed
	expect_dropped(false, false, false, false);
	sfence(true, 0x6000, false, 0);                    // rs2 = x0: they are
	expect_dropped(false, true, false, false);
}

void test_e6_rs2_is_a_register_number_not_a_value(void) {
	fill_four();
	sfence(false, 0, true, 0);                         // x11 holds 0: ASID 0, not "all"
	expect_dropped(false, false, false, false);
}

void test_e7_high_bits_of_the_asid_are_ignored(void) {
	fill_four();
	sfence(false, 0, true, 0xFFFFFE01u);               // bits 8:0 are 1
	expect_dropped(true, false, false, false);
}

void test_e8_a_megapage_is_dropped_by_an_address_inside_it(void) {
	set_pte(ROOT, 2, 0x000, V | RWXAD);
	expect_load(0x00812345, 0x12345);
	set_pte(ROOT, 2, 0x400, V | RWXAD);
	sfence(true, 0x00400000, false, 0);                // outside the megapage
	expect_load(0x00812345, 0x12345);
	sfence(true, 0x00B00000, false, 0);                // inside it
	expect_load(0x00812345, 0x412345);
}

void test_e9_sfence_is_still_illegal_in_u_mode_and_with_tvm(void) {
	DecodedInstruction instr = decode_instruction(0x12000073u);
	cpu.privilege_mode = PRV_U;
	cpu.pc = 0x200;
	execute(&cpu, &mem, &instr);
	TEST_ASSERT_EQUAL_HEX32(MTVEC, cpu.pc);
	TEST_ASSERT_EQUAL_UINT32(2, cpu_read_csr(&cpu, CSR_MCAUSE));

	cpu.privilege_mode = PRV_S;
	cpu_write_csr(&cpu, CSR_MSTATUS, MSTATUS_TVM);
	cpu.pc = 0x200;
	execute(&cpu, &mem, &instr);
	TEST_ASSERT_EQUAL_HEX32(MTVEC, cpu.pc);
	TEST_ASSERT_EQUAL_UINT32(2, cpu_read_csr(&cpu, CSR_MCAUSE));
}

/* ---- F. Other ------------------------------------------------------------------------------------ */

void test_f1_a_reset_empties_the_cache(void) {
	expect_load(0x5000, 0x50000);
	map(5, 0x60, V | RWXAD);
	test_cpu_reset_in_mode(&cpu, PRV_S);
	set_satp(ROOT, 1);
	expect_load(0x5000, 0x60000);
}

void test_f2_pages_that_share_a_slot(void) {
	map(69, 0x70, V | RWXAD);                          // virtual page 69 = 5 + 64: the same slot as page 5
	for (int i = 0; i < 2; i++) {
		expect_load(0x5000, 0x50000);
		expect_load(0x45000, 0x70000);
	}
}

void test_f3_a_page_and_a_megapage_together(void) {
	set_pte(ROOT, 2, 0x000, V | RWXAD);
	for (int i = 0; i < 2; i++) {
		expect_load(0x5000, 0x50000);
		expect_load(0x00812345, 0x12345);
	}
}

void test_f4_a_pmp_access_fault_is_not_cached(void) {
	cpu.pmpcfg[0] = 0x10;                              // entry 0: NA4, no permissions, exactly root[0]
	cpu.pmpaddr[0] = ROOT >> 2;
	cpu.pmpcfg[1] = 0x1F;
	cpu.pmpaddr[1] = 0x7FFFFFFF;
	TEST_ASSERT_EQUAL(XLATE_ACCESS_FAULT, xl(0x5000, ACCESS_LOAD, PRV_S));
	cpu.pmpcfg[0] = 0x1F;                              // entry 0: everything
	cpu.pmpaddr[0] = 0x7FFFFFFF;
	expect_load(0x5000, 0x50000);
}

void test_f5_a_fetch_and_a_load_share_an_entry(void) {
	map(6, 0x60, V | R | X | A | D);
	TEST_ASSERT_EQUAL(XLATE_OK, xl(0x6000, ACCESS_FETCH, PRV_S));
	TEST_ASSERT_EQUAL_HEX32(0x60000, pa);
	expect_load(0x6000, 0x60000);
	TEST_ASSERT_EQUAL(XLATE_PAGE_FAULT, xl(0x6000, ACCESS_STORE, PRV_S));
}

int main(void) {
	UNITY_BEGIN();
	RUN_TEST(test_a1_a_cached_entry_is_used);
	RUN_TEST(test_a2_a_fence_drops_it);
	RUN_TEST(test_a3_a_fault_is_not_cached);
	RUN_TEST(test_a4_an_invalidated_page_stays_until_a_fence);
	RUN_TEST(test_a5_a_cached_megapage);
	RUN_TEST(test_b1_sum_takes_effect_at_once);
	RUN_TEST(test_b2_mxr_takes_effect_at_once);
	RUN_TEST(test_b3_the_privilege_is_checked_on_a_hit);
	RUN_TEST(test_b4_a_store_to_a_cached_read_only_page);
	RUN_TEST(test_c1_a_store_after_a_load_sets_dirty);
	RUN_TEST(test_c2_dirty_already_set_in_memory);
	RUN_TEST(test_c3_a_cached_dirty_entry_writes_nothing);
	RUN_TEST(test_d1_the_asid_selects_the_entry);
	RUN_TEST(test_d2_an_entry_survives_a_switch_of_address_space);
	RUN_TEST(test_e1_everything);
	RUN_TEST(test_e2_one_asid_keeps_global_entries);
	RUN_TEST(test_e3_one_page_in_any_asid);
	RUN_TEST(test_e3b_the_page_need_not_belong_to_the_current_asid);
	RUN_TEST(test_e4_one_page_in_one_asid);
	RUN_TEST(test_e5_global_entries);
	RUN_TEST(test_e6_rs2_is_a_register_number_not_a_value);
	RUN_TEST(test_e7_high_bits_of_the_asid_are_ignored);
	RUN_TEST(test_e8_a_megapage_is_dropped_by_an_address_inside_it);
	RUN_TEST(test_e9_sfence_is_still_illegal_in_u_mode_and_with_tvm);
	RUN_TEST(test_f1_a_reset_empties_the_cache);
	RUN_TEST(test_f2_pages_that_share_a_slot);
	RUN_TEST(test_f3_a_page_and_a_megapage_together);
	RUN_TEST(test_f4_a_pmp_access_fault_is_not_cached);
	RUN_TEST(test_f5_a_fetch_and_a_load_share_an_entry);
	return UNITY_END();
}
