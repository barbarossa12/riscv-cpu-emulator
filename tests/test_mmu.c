#include "unity.h"
#include "cpu_test_util.h"
#include "../src/mmu.h"

#define ROOT 0x1000u   // root page table
#define L0   0x2000u   // level-0 table that root[0] points to
#define L0B  0x3000u   // level-0 table that root[1] points to

// PTE flag bits
#define V 0x01u
#define R 0x02u
#define W 0x04u
#define X 0x08u
#define U 0x10u
#define A 0x40u
#define D 0x80u

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

static XlateResult walk(uint32_t va, AccessType type, PrivMode priv) {
	pa = 0xFFFFFFFF;
	return mmu_translate(&cpu, &mem, va, type, priv, &pa);
}

// Sv32 on with the root at ROOT, root[0] -> L0, and L0[5] mapping va 0x5000 to pa 0x50000 (all permissions)
void setUp(void) {
	mem_init(&mem, 0x100000);
	test_cpu_reset_in_mode(&cpu, PRV_S);
	cpu_write_csr(&cpu, CSR_SATP, SATP_SV32 | (ROOT >> 12));
	set_pte(ROOT, 0, L0 >> 12, V);
	set_pte(L0, 5, 0x50, V | R | W | X | U | A | D);
}

void tearDown(void) {
	mem_free(&mem);
}

/* ---- A. Bypass ------------------------------------------------------------------------------ */

void test_a1_bare_mode(void) {
	cpu_write_csr(&cpu, CSR_SATP, 0);
	TEST_ASSERT_EQUAL(XLATE_OK, walk(0x1234, ACCESS_LOAD, PRV_S));
	TEST_ASSERT_EQUAL_HEX32(0x1234, pa);
}

void test_a2_m_mode_ignores_satp(void) {
	TEST_ASSERT_EQUAL(XLATE_OK, walk(0xDEAD0000, ACCESS_LOAD, PRV_M));
	TEST_ASSERT_EQUAL_HEX32(0xDEAD0000, pa);
}

void test_a3_asid_bits_do_not_matter(void) {
	cpu_write_csr(&cpu, CSR_SATP, SATP_SV32 | (0x1FFu << 22) | (ROOT >> 12));
	TEST_ASSERT_EQUAL(XLATE_OK, walk(0x5ABC, ACCESS_LOAD, PRV_U));
	TEST_ASSERT_EQUAL_HEX32(0x50ABC, pa);
}

/* ---- B. Basic 4 KiB walk -------------------------------------------------------------------- */

void test_b1_offset_kept(void) {
	TEST_ASSERT_EQUAL(XLATE_OK, walk(0x5ABC, ACCESS_LOAD, PRV_U));
	TEST_ASSERT_EQUAL_HEX32(0x50ABC, pa);
}

void test_b2_offset_fff(void) {
	TEST_ASSERT_EQUAL(XLATE_OK, walk(0x5FFF, ACCESS_LOAD, PRV_U));
	TEST_ASSERT_EQUAL_HEX32(0x50FFF, pa);
}

void test_b3_nonzero_vpn1(void) {
	set_pte(ROOT, 1, L0B >> 12, V);
	set_pte(L0B, 2, 0x70, V | R | U | A | D);
	TEST_ASSERT_EQUAL(XLATE_OK, walk(0x00402000, ACCESS_LOAD, PRV_U));
	TEST_ASSERT_EQUAL_HEX32(0x70000, pa);
}

/* ---- C. Invalid entries --------------------------------------------------------------------- */

void test_c1_invalid_at_level0(void) {
	TEST_ASSERT_EQUAL(XLATE_PAGE_FAULT, walk(0x6000, ACCESS_LOAD, PRV_U));
}

void test_c2_invalid_at_level1(void) {
	TEST_ASSERT_EQUAL(XLATE_PAGE_FAULT, walk(0x00400000, ACCESS_LOAD, PRV_U));
}

void test_c3_write_without_read(void) {
	set_pte(L0, 6, 0x60, V | W | U | A | D);
	TEST_ASSERT_EQUAL(XLATE_PAGE_FAULT, walk(0x6000, ACCESS_LOAD, PRV_U));
}

void test_c4_pointer_at_last_level(void) {
	set_pte(L0, 7, 0x3, V);
	TEST_ASSERT_EQUAL(XLATE_PAGE_FAULT, walk(0x7000, ACCESS_LOAD, PRV_U));
}

void test_c5_root_outside_memory(void) {
	cpu_write_csr(&cpu, CSR_SATP, SATP_SV32 | 0x3FFFFF);
	TEST_ASSERT_EQUAL(XLATE_ACCESS_FAULT, walk(0x5000, ACCESS_LOAD, PRV_U));
}

void test_c6_pointer_to_table_outside_memory(void) {
	set_pte(ROOT, 1, 0x3FFFFF, V);
	TEST_ASSERT_EQUAL(XLATE_ACCESS_FAULT, walk(0x00400000, ACCESS_LOAD, PRV_U));
}

/* ---- D. Permissions (U-mode) ---------------------------------------------------------------- */

static void check_perm(uint32_t flags, AccessType type, bool mxr, XlateResult want) {
	set_pte(L0, 8, 0x80, V | U | A | D | flags);
	cpu_write_csr(&cpu, CSR_MSTATUS, mxr ? MSTATUS_MXR : 0);
	TEST_ASSERT_EQUAL(want, walk(0x8000, type, PRV_U));
}

void test_d_permissions(void) {
	const struct {
		const char *name;
		uint32_t flags; AccessType type; bool mxr; XlateResult want;
	} v[] = {
		{"D1 R, load",           R,         ACCESS_LOAD,  false, XLATE_OK},
		{"D2 R, store",          R,         ACCESS_STORE, false, XLATE_PAGE_FAULT},
		{"D3 R, fetch",          R,         ACCESS_FETCH, false, XLATE_PAGE_FAULT},
		{"D4 RW, store",         R | W,     ACCESS_STORE, false, XLATE_OK},
		{"D5 X, fetch",          X,         ACCESS_FETCH, false, XLATE_OK},
		{"D6 X, load",           X,         ACCESS_LOAD,  false, XLATE_PAGE_FAULT},
		{"D7 X, load, MXR",      X,         ACCESS_LOAD,  true,  XLATE_OK},
		{"D8 X, store, MXR",     X,         ACCESS_STORE, true,  XLATE_PAGE_FAULT},
		{"D9 R, load, MXR",      R,         ACCESS_LOAD,  true,  XLATE_OK},
		{"D10 RWX, load",        R | W | X, ACCESS_LOAD,  false, XLATE_OK},
		{"D10 RWX, store",       R | W | X, ACCESS_STORE, false, XLATE_OK},
		{"D10 RWX, fetch",       R | W | X, ACCESS_FETCH, false, XLATE_OK},
	};
	for (unsigned i = 0; i < sizeof v / sizeof v[0]; i++) {
		tearDown();
		setUp();
		TEST_MESSAGE(v[i].name);
		check_perm(v[i].flags, v[i].type, v[i].mxr, v[i].want);
	}
}

/* ---- E. User / supervisor and SUM ----------------------------------------------------------- */

static void check_user_bit(uint32_t user_flag, PrivMode priv, AccessType type, bool sum, XlateResult want) {
	set_pte(L0, 9, 0x90, V | R | W | X | A | D | user_flag);
	cpu_write_csr(&cpu, CSR_MSTATUS, sum ? MSTATUS_SUM : 0);
	TEST_ASSERT_EQUAL(want, walk(0x9000, type, priv));
}

void test_e_user_and_supervisor(void) {
	const struct {
		const char *name;
		uint32_t user_flag; PrivMode priv; AccessType type; bool sum; XlateResult want;
	} v[] = {
		{"E1 U=0, U-mode load",        0, PRV_U, ACCESS_LOAD,  false, XLATE_PAGE_FAULT},
		{"E2 U=0, S-mode load",        0, PRV_S, ACCESS_LOAD,  false, XLATE_OK},
		{"E2 U=0, S-mode store",       0, PRV_S, ACCESS_STORE, false, XLATE_OK},
		{"E2 U=0, S-mode fetch",       0, PRV_S, ACCESS_FETCH, false, XLATE_OK},
		{"E3 U=1, S-mode load",        U, PRV_S, ACCESS_LOAD,  false, XLATE_PAGE_FAULT},
		{"E4 U=1, S-mode load, SUM",   U, PRV_S, ACCESS_LOAD,  true,  XLATE_OK},
		{"E5 U=1, S-mode store, SUM",  U, PRV_S, ACCESS_STORE, true,  XLATE_OK},
		{"E6 U=1, S-mode fetch, SUM",  U, PRV_S, ACCESS_FETCH, true,  XLATE_PAGE_FAULT},
		{"E7 U=1, U-mode load",        U, PRV_U, ACCESS_LOAD,  false, XLATE_OK},
		{"E7 U=1, U-mode store",       U, PRV_U, ACCESS_STORE, false, XLATE_OK},
		{"E7 U=1, U-mode fetch",       U, PRV_U, ACCESS_FETCH, false, XLATE_OK},
	};
	for (unsigned i = 0; i < sizeof v / sizeof v[0]; i++) {
		tearDown();
		setUp();
		TEST_MESSAGE(v[i].name);
		check_user_bit(v[i].user_flag, v[i].priv, v[i].type, v[i].sum, v[i].want);
	}
}

/* ---- F. Megapages --------------------------------------------------------------------------- */

void test_f1_aligned_megapage(void) {
	set_pte(ROOT, 2, 0x800, V | R | W | U | A | D);
	TEST_ASSERT_EQUAL(XLATE_OK, walk(0x00812345, ACCESS_LOAD, PRV_U));
	TEST_ASSERT_EQUAL_HEX32(0x812345, pa);
}

void test_f2_low_ppn_bits_come_from_va(void) {
	set_pte(ROOT, 2, 0x800, V | R | W | U | A | D);
	TEST_ASSERT_EQUAL(XLATE_OK, walk(0x00BFF000, ACCESS_LOAD, PRV_U));
	TEST_ASSERT_EQUAL_HEX32(0xBFF000, pa);
}

void test_f3_misaligned_megapage(void) {
	set_pte(ROOT, 3, 0x801, V | R | W | U | A | D);
	TEST_ASSERT_EQUAL(XLATE_PAGE_FAULT, walk(0x00C00000, ACCESS_LOAD, PRV_U));
}

void test_f4_megapage_permissions(void) {
	set_pte(ROOT, 2, 0x800, V | R | U | A | D);
	TEST_ASSERT_EQUAL(XLATE_PAGE_FAULT, walk(0x00812345, ACCESS_STORE, PRV_U));
}

/* ---- G. Accessed / dirty bits --------------------------------------------------------------- */

void test_g_accessed_and_dirty(void) {
	const struct {
		const char *name;
		uint32_t before; AccessType type; uint32_t after;
	} v[] = {
		{"G1 A set on load",           V | R | W | U,         ACCESS_LOAD,  V | R | W | U | A},
		{"G2 A set on fetch",          V | X | U,            ACCESS_FETCH, V | X | U | A},
		{"G3 A and D set on store",    V | R | W | U,        ACCESS_STORE, V | R | W | U | A | D},
		{"G4 D set on store after load", V | R | W | U | A,   ACCESS_STORE, V | R | W | U | A | D},
		{"G5 already set, unchanged",  V | R | W | U | A | D, ACCESS_LOAD,  V | R | W | U | A | D},
	};
	for (unsigned i = 0; i < sizeof v / sizeof v[0]; i++) {
		tearDown();
		setUp();
		TEST_MESSAGE(v[i].name);
		set_pte(L0, 11, 0xB0, v[i].before);
		TEST_ASSERT_EQUAL(XLATE_OK, walk(0xB000, v[i].type, PRV_U));
		TEST_ASSERT_EQUAL_HEX32(0xB0000, pa);
		TEST_ASSERT_EQUAL_HEX32((0xB0u << 10) | v[i].after, get_pte(L0, 11));
	}
}

void test_g6_permission_fault_leaves_pte_alone(void) {
	set_pte(L0, 12, 0xC0, V | R | U);
	TEST_ASSERT_EQUAL(XLATE_PAGE_FAULT, walk(0xC000, ACCESS_STORE, PRV_U));
	TEST_ASSERT_EQUAL_HEX32((0xC0u << 10) | V | R | U, get_pte(L0, 12));
}

void test_g7_megapage_accessed_and_dirty(void) {
	set_pte(ROOT, 2, 0x800, V | R | W | U);
	TEST_ASSERT_EQUAL(XLATE_OK, walk(0x00812345, ACCESS_STORE, PRV_U));
	TEST_ASSERT_EQUAL_HEX32((0x800u << 10) | V | R | W | U | A | D, get_pte(ROOT, 2));
}

void test_g8_pmp_denies_the_ad_write(void) {
	set_pte(L0, 11, 0xB0, V | R | W | U);
	cpu.pmpcfg[0] = 0x11;                              // entry 0: NA4, read only
	cpu.pmpaddr[0] = (L0 + 11 * 4) >> 2;               // exactly L0[11]
	cpu.pmpcfg[1] = 0x1F;                              // entry 1: everything else
	cpu.pmpaddr[1] = 0x7FFFFFFF;
	TEST_ASSERT_EQUAL(XLATE_ACCESS_FAULT, walk(0xB000, ACCESS_LOAD, PRV_U));
	TEST_ASSERT_EQUAL_HEX32((0xB0u << 10) | V | R | W | U, get_pte(L0, 11));
}

/* ---- H. Physical address and PMP on the walk ------------------------------------------------ */

void test_h1_result_above_4gib(void) {
	set_pte(L0, 13, 0x300000, V | R | U | A | D);
	TEST_ASSERT_EQUAL(XLATE_ACCESS_FAULT, walk(0xD000, ACCESS_LOAD, PRV_U));
}

void test_h2_pmp_denies_pte_read(void) {
	cpu.pmpcfg[0] = 0x10;                              // entry 0: NA4, no permissions
	cpu.pmpaddr[0] = ROOT >> 2;                        // exactly root[0]
	cpu.pmpcfg[1] = 0x1F;
	cpu.pmpaddr[1] = 0x7FFFFFFF;
	TEST_ASSERT_EQUAL(XLATE_ACCESS_FAULT, walk(0x5000, ACCESS_LOAD, PRV_U));
}

int main(void) {
	UNITY_BEGIN();
	RUN_TEST(test_a1_bare_mode);
	RUN_TEST(test_a2_m_mode_ignores_satp);
	RUN_TEST(test_a3_asid_bits_do_not_matter);
	RUN_TEST(test_b1_offset_kept);
	RUN_TEST(test_b2_offset_fff);
	RUN_TEST(test_b3_nonzero_vpn1);
	RUN_TEST(test_c1_invalid_at_level0);
	RUN_TEST(test_c2_invalid_at_level1);
	RUN_TEST(test_c3_write_without_read);
	RUN_TEST(test_c4_pointer_at_last_level);
	RUN_TEST(test_c5_root_outside_memory);
	RUN_TEST(test_c6_pointer_to_table_outside_memory);
	RUN_TEST(test_d_permissions);
	RUN_TEST(test_e_user_and_supervisor);
	RUN_TEST(test_f1_aligned_megapage);
	RUN_TEST(test_f2_low_ppn_bits_come_from_va);
	RUN_TEST(test_f3_misaligned_megapage);
	RUN_TEST(test_f4_megapage_permissions);
	RUN_TEST(test_g_accessed_and_dirty);
	RUN_TEST(test_g6_permission_fault_leaves_pte_alone);
	RUN_TEST(test_g7_megapage_accessed_and_dirty);
	RUN_TEST(test_g8_pmp_denies_the_ad_write);
	RUN_TEST(test_h1_result_above_4gib);
	RUN_TEST(test_h2_pmp_denies_pte_read);
	return UNITY_END();
}
