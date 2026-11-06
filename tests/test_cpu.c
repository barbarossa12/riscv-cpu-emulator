#include <stdio.h>
#include "unity.h"
#include "../src/cpu.h"
#include "cpu_test_util.h"

void setUp(void) {} // Unity calles this before each test function
void tearDown(void) {} // Unity calls this after eacht test function

void test_cpu_write_read_roundtrip_x5(void) {
	CPU test_cpu;
	cpu_write_reg(&test_cpu, 5, 0x64);
	uint32_t val = cpu_read_reg(&test_cpu, 5);
	TEST_ASSERT_EQUAL_UINT32(0x64, val);
}

void test_cpu_write_read_roundtrip_x31(void) {
	CPU test_cpu;
	cpu_write_reg(&test_cpu, 31, 0xDEADBEEF);
	uint32_t val = cpu_read_reg(&test_cpu, 31);
	TEST_ASSERT_EQUAL_UINT32(0xDEADBEEF, val);
}

void test_cpu_x0_read_ignores_underlying_value(void) {
	CPU test_cpu;
	test_cpu.regs[0] = 0xDEADBEEF;
	uint32_t val = cpu_read_reg(&test_cpu, 0);
	TEST_ASSERT_EQUAL_UINT32(0, val);
}

void test_cpu_x0_write_is_discarded(void) {
	CPU test_cpu = {0};
	test_cpu.regs[0] = 0xDEADBEEF;
	cpu_write_reg(&test_cpu, 0, 0x12345);
	TEST_ASSERT_EQUAL_UINT32(0xDEADBEEF, test_cpu.regs[0]);
}

void test_cpu_write_to_x0_does_not_affect_other_registers(void) {
	CPU test_cpu = {0};
	cpu_write_reg(&test_cpu, 1, 0x42);
	cpu_write_reg(&test_cpu, 0, 0xDEADBEEF);
	uint32_t val = cpu_read_reg(&test_cpu, 1);
	TEST_ASSERT_EQUAL_UINT32(0x42, val);
}

void test_cpu_csr_write_read_roundtrip(void) {
	CPU test_cpu = {0};
	cpu_write_csr(&test_cpu, CSR_MTVEC, 0x1000);
	uint32_t val = cpu_read_csr(&test_cpu, CSR_MTVEC);
	TEST_ASSERT_EQUAL_UINT32(0x1000, val);
}

void test_cpu_csr_writes_are_independent(void) {
	CPU test_cpu = {0};
	cpu_write_csr(&test_cpu, CSR_MEPC, 0xAAA8);   // 2-aligned: mepc only drops bit 0
	cpu_write_csr(&test_cpu, CSR_MCAUSE, 0xBBBB);
	TEST_ASSERT_EQUAL_UINT32(0xAAA8, cpu_read_csr(&test_cpu, CSR_MEPC));
	TEST_ASSERT_EQUAL_UINT32(0xBBBB, cpu_read_csr(&test_cpu, CSR_MCAUSE));
}

void test_cpu_mtvec_mode_bits_read_as_zero(void) {
	CPU test_cpu = {0};
	cpu_write_csr(&test_cpu, CSR_MTVEC, 0x1001); // Vectored: not supported yet
	TEST_ASSERT_EQUAL_UINT32(0x1000, cpu_read_csr(&test_cpu, CSR_MTVEC));
	cpu_write_csr(&test_cpu, CSR_MTVEC, 0x1002); // Reserved
	TEST_ASSERT_EQUAL_UINT32(0x1000, cpu_read_csr(&test_cpu, CSR_MTVEC));
	cpu_write_csr(&test_cpu, CSR_MTVEC, 0x1003); // Reserved
	TEST_ASSERT_EQUAL_UINT32(0x1000, cpu_read_csr(&test_cpu, CSR_MTVEC));
}

void test_cpu_other_csrs_keep_low_bits(void) {
	CPU test_cpu = {0};
	cpu_write_csr(&test_cpu, CSR_MSCRATCH, 0x1003);
	TEST_ASSERT_EQUAL_UINT32(0x1003, cpu_read_csr(&test_cpu, CSR_MSCRATCH));
}

void test_cpu_misa_reports_the_implemented_extensions(void) {
	CPU test_cpu = {0};
	TEST_ASSERT_EQUAL_UINT32(0x40141105, cpu_read_csr(&test_cpu, CSR_MISA)); // MXL=1, A, C, I, M, S, U
}

void test_cpu_misa_writes_are_ignored(void) {
	CPU test_cpu = {0};
	cpu_write_csr(&test_cpu, CSR_MISA, 0xFFFFFFFF); // tries to enable C, F, ...
	TEST_ASSERT_EQUAL_UINT32(0x40141105, cpu_read_csr(&test_cpu, CSR_MISA));
	cpu_write_csr(&test_cpu, CSR_MISA, 0);
	TEST_ASSERT_EQUAL_UINT32(0x40141105, cpu_read_csr(&test_cpu, CSR_MISA));
	cpu_write_csr(&test_cpu, CSR_MISA, 0x40141101); // tries to switch C off: IALIGN stays 16
	TEST_ASSERT_EQUAL_UINT32(0x40141105, cpu_read_csr(&test_cpu, CSR_MISA));
}

void test_cpu_counteren_reads_zero_after_reset(void) {
	CPU c; cpu_reset(&c);
	TEST_ASSERT_EQUAL_UINT32(0, cpu_read_csr(&c, CSR_MCOUNTEREN));
	TEST_ASSERT_EQUAL_UINT32(0, cpu_read_csr(&c, CSR_SCOUNTEREN));
}

void test_cpu_counteren_writable_bits(void) {
	/* written -> read back; mcounteren and scounteren behave the same */
	const struct { uint32_t written, expected; } v[] = {
		{0x00000000, 0x00000000},
		{0x00000001, 0x00000001}, // CY
		{0x00000004, 0x00000004}, // IR
		{0x00000005, 0x00000005}, // CY | IR
		{0x00000002, 0x00000000}, // TM: read-only 0 until Milestone 6
		{0xFFFFFFF8, 0x00000000}, // TM and HPM3..31: read-only 0
		{0xFFFFFFFF, 0x00000005},
	};
	const uint16_t csrs[] = {CSR_MCOUNTEREN, CSR_SCOUNTEREN};
	for (unsigned c = 0; c < 2; c++) {
		for (unsigned i = 0; i < sizeof v / sizeof v[0]; i++) {
			CPU cpu; cpu_reset(&cpu);
			cpu_write_csr(&cpu, csrs[c], v[i].written);
			TEST_ASSERT_EQUAL_UINT32(v[i].expected, cpu_read_csr(&cpu, csrs[c]));
		}
	}
}

void test_cpu_counteren_registers_are_independent(void) {
	CPU c; cpu_reset(&c);
	cpu_write_csr(&c, CSR_MCOUNTEREN, 0x1);
	cpu_write_csr(&c, CSR_SCOUNTEREN, 0x4);
	TEST_ASSERT_EQUAL_UINT32(0x1, cpu_read_csr(&c, CSR_MCOUNTEREN));
	TEST_ASSERT_EQUAL_UINT32(0x4, cpu_read_csr(&c, CSR_SCOUNTEREN));
}

void test_cpu_id_csrs_read_zero(void) {
	CPU test_cpu = {0};
	test_cpu.csr[CSR_MVENDORID] = 0xDEAD; // raw storage must not leak through
	test_cpu.csr[CSR_MARCHID] = 0xDEAD;
	test_cpu.csr[CSR_MIMPID] = 0xDEAD;
	test_cpu.csr[CSR_MHARTID] = 0xDEAD;
	TEST_ASSERT_EQUAL_UINT32(0, cpu_read_csr(&test_cpu, CSR_MVENDORID));
	TEST_ASSERT_EQUAL_UINT32(0, cpu_read_csr(&test_cpu, CSR_MARCHID));
	TEST_ASSERT_EQUAL_UINT32(0, cpu_read_csr(&test_cpu, CSR_MIMPID));
	TEST_ASSERT_EQUAL_UINT32(0, cpu_read_csr(&test_cpu, CSR_MHARTID));
}

void test_cpu_trigger_csrs_are_hardwired_zero(void) {
	CPU test_cpu = {0};
	cpu_write_csr(&test_cpu, CSR_TSELECT, 5);
	cpu_write_csr(&test_cpu, CSR_TDATA1, 0x20001040);
	cpu_write_csr(&test_cpu, CSR_TDATA2, 0x80000000);
	TEST_ASSERT_EQUAL_UINT32(0, cpu_read_csr(&test_cpu, CSR_TSELECT));
	TEST_ASSERT_EQUAL_UINT32(0, cpu_read_csr(&test_cpu, CSR_TDATA1));
	TEST_ASSERT_EQUAL_UINT32(0, cpu_read_csr(&test_cpu, CSR_TDATA2));
}

void test_cpu_mstatus_reads_zero_after_reset(void) {
	CPU c; cpu_reset(&c);
	TEST_ASSERT_EQUAL_UINT32(0, cpu_read_csr(&c, CSR_MSTATUS));
}

void test_cpu_mstatus_writable_bits_round_trip(void) {
	CPU c; cpu_reset(&c);
	cpu_write_csr(&c, CSR_MSTATUS, 0x007E19AA);   // every writable bit, MPP = M
	TEST_ASSERT_EQUAL_UINT32(0x007E19AA, cpu_read_csr(&c, CSR_MSTATUS));
}

void test_cpu_mstatus_unimplemented_bits_read_zero(void) {
	CPU c; cpu_reset(&c);
	cpu_write_csr(&c, CSR_MSTATUS, 0xFFFFFFFF);   // MPP = 3, so legal
	// everything outside the writable set (UBE, VS, FS, XS, SD) is dropped
	TEST_ASSERT_EQUAL_UINT32(0x007E19AA, cpu_read_csr(&c, CSR_MSTATUS));
}

void test_cpu_mstatus_mpp_accepts_u_s_and_m(void) {
	CPU c; cpu_reset(&c);
	cpu_write_csr(&c, CSR_MSTATUS, 0x1800);       // M
	TEST_ASSERT_EQUAL_UINT32(0x1800, cpu_read_csr(&c, CSR_MSTATUS));
	cpu_write_csr(&c, CSR_MSTATUS, 0x0800);       // S
	TEST_ASSERT_EQUAL_UINT32(0x0800, cpu_read_csr(&c, CSR_MSTATUS));
	cpu_write_csr(&c, CSR_MSTATUS, 0x0000);       // U
	TEST_ASSERT_EQUAL_UINT32(0x0000, cpu_read_csr(&c, CSR_MSTATUS));
}

void test_cpu_mstatus_mpp_reserved_write_keeps_old(void) {
	CPU c; cpu_reset(&c);
	cpu_write_csr(&c, CSR_MSTATUS, 0x1800);       // MPP = M
	cpu_write_csr(&c, CSR_MSTATUS, 0x1000);       // write the reserved value 2
	TEST_ASSERT_EQUAL_UINT32(0x1800, cpu_read_csr(&c, CSR_MSTATUS));
	cpu_write_csr(&c, CSR_MSTATUS, 0x0000);       // MPP = U
	cpu_write_csr(&c, CSR_MSTATUS, 0x1000);       // reserved again
	TEST_ASSERT_EQUAL_UINT32(0x0000, cpu_read_csr(&c, CSR_MSTATUS));
}

void test_cpu_mstatus_mpp_reserved_write_still_applies_other_fields(void) {
	CPU c; cpu_reset(&c);
	cpu_write_csr(&c, CSR_MSTATUS, 0x1800);       // MPP = M
	cpu_write_csr(&c, CSR_MSTATUS, 0x1008);       // MIE + reserved MPP
	TEST_ASSERT_EQUAL_UINT32(0x1808, cpu_read_csr(&c, CSR_MSTATUS)); // MIE set, MPP still M
}

void test_cpu_mepc_bit_zero_is_zero(void) {
	CPU c; cpu_reset(&c);
	const struct { uint32_t written, expected; } v[] = {
		{0x80000000, 0x80000000},
		{0x80000001, 0x80000000},
		{0x80000002, 0x80000002},
		{0x80000003, 0x80000002},
		{0x80000004, 0x80000004},
		{0xFFFFFFFF, 0xFFFFFFFE},
	};
	for (unsigned i = 0; i < sizeof v / sizeof v[0]; i++) {
		cpu_write_csr(&c, CSR_MEPC, v[i].written);
		TEST_ASSERT_EQUAL_UINT32(v[i].expected, cpu_read_csr(&c, CSR_MEPC));
	}
}

void test_cpu_counter_halves_round_trip(void) {
	CPU test_cpu = {0};
	cpu_write_csr(&test_cpu, CSR_MCYCLE, 0x11111111);
	cpu_write_csr(&test_cpu, CSR_MCYCLEH, 0x22222222);
	cpu_write_csr(&test_cpu, CSR_MINSTRET, 0x33333333);
	cpu_write_csr(&test_cpu, CSR_MINSTRETH, 0x44444444);
	TEST_ASSERT_EQUAL_UINT32(0x11111111, cpu_read_csr(&test_cpu, CSR_MCYCLE));
	TEST_ASSERT_EQUAL_UINT32(0x22222222, cpu_read_csr(&test_cpu, CSR_MCYCLEH));
	TEST_ASSERT_EQUAL_UINT32(0x33333333, cpu_read_csr(&test_cpu, CSR_MINSTRET));
	TEST_ASSERT_EQUAL_UINT32(0x44444444, cpu_read_csr(&test_cpu, CSR_MINSTRETH));
	TEST_ASSERT_EQUAL_UINT64(0x2222222211111111ull, test_cpu.mcycle);
	TEST_ASSERT_EQUAL_UINT64(0x4444444433333333ull, test_cpu.minstret);
}

void test_cpu_counter_half_writes_keep_the_other_half(void) {
	CPU test_cpu = {0};
	test_cpu.mcycle = 0xAAAAAAAABBBBBBBBull;
	cpu_write_csr(&test_cpu, CSR_MCYCLE, 0x1);
	TEST_ASSERT_EQUAL_UINT64(0xAAAAAAAA00000001ull, test_cpu.mcycle);
	cpu_write_csr(&test_cpu, CSR_MCYCLEH, 0x2);
	TEST_ASSERT_EQUAL_UINT64(0x0000000200000001ull, test_cpu.mcycle);
}

void test_cpu_counter_shadows_read_the_machine_counters(void) {
	CPU test_cpu = {0};
	test_cpu.mcycle = 0x0000000100000005ull;
	test_cpu.minstret = 0x0000000200000009ull;
	TEST_ASSERT_EQUAL_UINT32(5, cpu_read_csr(&test_cpu, CSR_CYCLE));
	TEST_ASSERT_EQUAL_UINT32(1, cpu_read_csr(&test_cpu, CSR_CYCLEH));
	TEST_ASSERT_EQUAL_UINT32(9, cpu_read_csr(&test_cpu, CSR_INSTRET));
	TEST_ASSERT_EQUAL_UINT32(2, cpu_read_csr(&test_cpu, CSR_INSTRETH));
}

void test_cpu_counter_shadows_ignore_writes(void) {
	CPU test_cpu = {0};
	test_cpu.mcycle = 7;
	test_cpu.minstret = 9;
	cpu_write_csr(&test_cpu, CSR_CYCLE, 0xFFFF);
	cpu_write_csr(&test_cpu, CSR_CYCLEH, 0xFFFF);
	cpu_write_csr(&test_cpu, CSR_INSTRET, 0xFFFF);
	cpu_write_csr(&test_cpu, CSR_INSTRETH, 0xFFFF);
	TEST_ASSERT_EQUAL_UINT64(7, test_cpu.mcycle);
	TEST_ASSERT_EQUAL_UINT64(9, test_cpu.minstret);
	TEST_ASSERT_FALSE(test_cpu.mcycle_written);
	TEST_ASSERT_FALSE(test_cpu.minstret_written);
}

void test_cpu_counter_write_marks_only_that_counter(void) {
	CPU test_cpu = {0};
	cpu_write_csr(&test_cpu, CSR_MINSTRETH, 1);
	TEST_ASSERT_TRUE(test_cpu.minstret_written);
	TEST_ASSERT_FALSE(test_cpu.mcycle_written);
	cpu_write_csr(&test_cpu, CSR_MCYCLE, 1);
	TEST_ASSERT_TRUE(test_cpu.mcycle_written);
}

void test_cpu_reset_enters_machine_mode(void) {
	CPU test_cpu = {0};
	TEST_ASSERT_EQUAL_INT(PRV_U, test_cpu.privilege_mode); // a zeroed CPU is U-mode
	cpu_reset(&test_cpu);
	TEST_ASSERT_EQUAL_INT(PRV_M, test_cpu.privilege_mode);
}

void test_cpu_reset_clears_all_other_state(void) {
	CPU test_cpu;
	test_cpu.privilege_mode = PRV_S;
	test_cpu.pc = 0x1234;
	test_cpu.regs[7] = 0xDEADBEEF;
	test_cpu.csr[CSR_MSCRATCH] = 0xCAFE;
	test_cpu.mcycle = 99;
	test_cpu.minstret = 98;
	test_cpu.trapped = true;

	cpu_reset(&test_cpu);

	TEST_ASSERT_EQUAL_INT(PRV_M, test_cpu.privilege_mode);
	TEST_ASSERT_EQUAL_UINT32(0, test_cpu.pc);
	TEST_ASSERT_EQUAL_UINT32(0, test_cpu.regs[7]);
	TEST_ASSERT_EQUAL_UINT32(0, test_cpu.csr[CSR_MSCRATCH]);
	TEST_ASSERT_EQUAL_UINT64(0, test_cpu.mcycle);
	TEST_ASSERT_EQUAL_UINT64(0, test_cpu.minstret);
	TEST_ASSERT_FALSE(test_cpu.trapped);
}

void test_cpu_test_helper_sets_requested_mode(void) {
	CPU test_cpu;
	test_cpu_reset_in_mode(&test_cpu, PRV_S);
	TEST_ASSERT_EQUAL_INT(PRV_S, test_cpu.privilege_mode);
	test_cpu_reset_in_mode(&test_cpu, PRV_U);
	TEST_ASSERT_EQUAL_INT(PRV_U, test_cpu.privilege_mode);
}

/* ---- S-mode CSRs ------------------------------------------------------------------------ */

void test_cpu_sstatus_reads_the_s_view_of_mstatus(void) {
	const struct { uint32_t mstatus, sstatus; } v[] = {
		{0x00000008, 0x00000000}, // MIE
		{0x00000002, 0x00000002}, // SIE
		{0x00000020, 0x00000020}, // SPIE
		{0x00000100, 0x00000100}, // SPP
		{0x00001800, 0x00000000}, // MPP
		{0x00040000, 0x00040000}, // SUM
		{0x00080000, 0x00080000}, // MXR
		{0x00100000, 0x00000000}, // TVM
		{0x007E19AA, 0x000C0122}, // every writable mstatus bit
	};
	for (unsigned i = 0; i < sizeof v / sizeof v[0]; i++) {
		CPU c; cpu_reset(&c);
		cpu_write_csr(&c, CSR_MSTATUS, v[i].mstatus);
		TEST_ASSERT_EQUAL_UINT32(v[i].sstatus, cpu_read_csr(&c, CSR_SSTATUS));
	}
}

void test_cpu_sstatus_writes_touch_only_the_s_bits(void) {
	CPU c; cpu_reset(&c);
	cpu_write_csr(&c, CSR_MSTATUS, 0x1888);           // MIE, MPIE, MPP = M
	cpu_write_csr(&c, CSR_SSTATUS, 0xFFFFFFFF);
	TEST_ASSERT_EQUAL_UINT32(0xC19AA, cpu_read_csr(&c, CSR_MSTATUS));
	TEST_ASSERT_EQUAL_UINT32(0xC0122, cpu_read_csr(&c, CSR_SSTATUS)); // SIE, SPIE, SPP, SUM, MXR; FS, ... stay 0
	cpu_write_csr(&c, CSR_SSTATUS, 0);
	TEST_ASSERT_EQUAL_UINT32(0x1888, cpu_read_csr(&c, CSR_MSTATUS));
}

void test_cpu_mie_and_mip_keep_only_the_supervisor_bits(void) {
	CPU c; cpu_reset(&c);
	cpu_write_csr(&c, CSR_MIE, 0xFFFFFFFF);
	cpu_write_csr(&c, CSR_MIP, 0xFFFFFFFF);
	TEST_ASSERT_EQUAL_UINT32(0x222, cpu_read_csr(&c, CSR_MIE));
	TEST_ASSERT_EQUAL_UINT32(0x222, cpu_read_csr(&c, CSR_MIP));
	cpu_write_csr(&c, CSR_MIE, 0x888);                // MSIE, MTIE, MEIE: read-only 0 for now
	cpu_write_csr(&c, CSR_MIP, 0x888);
	TEST_ASSERT_EQUAL_UINT32(0, cpu_read_csr(&c, CSR_MIE));
	TEST_ASSERT_EQUAL_UINT32(0, cpu_read_csr(&c, CSR_MIP));
}

void test_cpu_sie_and_sip_show_only_delegated_interrupts(void) {
	const struct { uint32_t mideleg, visible; } v[] = {
		{0x000, 0x000}, {0x002, 0x002}, {0x222, 0x222},
	};
	for (unsigned i = 0; i < sizeof v / sizeof v[0]; i++) {
		CPU c; cpu_reset(&c);
		cpu_write_csr(&c, CSR_MIE, 0x222);
		cpu_write_csr(&c, CSR_MIP, 0x222);
		c.csr[CSR_MIDELEG] = v[i].mideleg;            // the register itself arrives in step 4
		TEST_ASSERT_EQUAL_UINT32(v[i].visible, cpu_read_csr(&c, CSR_SIE));
		TEST_ASSERT_EQUAL_UINT32(v[i].visible, cpu_read_csr(&c, CSR_SIP));
	}
}

void test_cpu_sie_writes_touch_only_delegated_bits(void) {
	CPU c; cpu_reset(&c);
	cpu_write_csr(&c, CSR_MIE, 0x200);                // SEIE
	c.csr[CSR_MIDELEG] = 0x022;                       // SSI and STI delegated
	cpu_write_csr(&c, CSR_SIE, 0xFFFFFFFF);
	TEST_ASSERT_EQUAL_UINT32(0x222, cpu_read_csr(&c, CSR_MIE));
	TEST_ASSERT_EQUAL_UINT32(0x022, cpu_read_csr(&c, CSR_SIE));
	cpu_write_csr(&c, CSR_SIE, 0);
	TEST_ASSERT_EQUAL_UINT32(0x200, cpu_read_csr(&c, CSR_MIE));

	c.csr[CSR_MIDELEG] = 0;                           // nothing delegated
	cpu_write_csr(&c, CSR_SIE, 0xFFFFFFFF);
	TEST_ASSERT_EQUAL_UINT32(0x200, cpu_read_csr(&c, CSR_MIE));
}

void test_cpu_sip_writes_touch_only_ssip(void) {
	CPU c; cpu_reset(&c);
	cpu_write_csr(&c, CSR_MIP, 0x220);                // STIP, SEIP
	c.csr[CSR_MIDELEG] = 0x222;
	cpu_write_csr(&c, CSR_SIP, 0xFFFFFFFF);
	TEST_ASSERT_EQUAL_UINT32(0x222, cpu_read_csr(&c, CSR_MIP)); // only SSIP changed
	cpu_write_csr(&c, CSR_SIP, 0);
	TEST_ASSERT_EQUAL_UINT32(0x220, cpu_read_csr(&c, CSR_MIP));

	c.csr[CSR_MIDELEG] = 0;                           // not delegated: SSIP is not writable
	cpu_write_csr(&c, CSR_SIP, 0xFFFFFFFF);
	TEST_ASSERT_EQUAL_UINT32(0x220, cpu_read_csr(&c, CSR_MIP));
}

void test_cpu_stvec_drops_two_low_bits_and_sepc_drops_bit_zero(void) {
	const struct { uint16_t csr; uint32_t written, expected; } v[] = {
		{CSR_STVEC, 0x80000003, 0x80000000},
		{CSR_STVEC, 0x00001001, 0x00001000},
		{CSR_SEPC,  0x80000003, 0x80000002},
		{CSR_SEPC,  0x80000001, 0x80000000},
		{CSR_SEPC,  0xFFFFFFFF, 0xFFFFFFFE},
	};
	for (unsigned i = 0; i < sizeof v / sizeof v[0]; i++) {
		CPU c; cpu_reset(&c);
		cpu_write_csr(&c, v[i].csr, v[i].written);
		TEST_ASSERT_EQUAL_UINT32(v[i].expected, cpu_read_csr(&c, v[i].csr));
	}
}

void test_cpu_plain_s_registers_round_trip(void) {
	const struct { uint16_t csr; uint32_t value; } v[] = {
		{CSR_SSCRATCH, 0xDEADBEEF}, {CSR_SCAUSE, 0x8000001F}, {CSR_STVAL, 0x12345678},
	};
	for (unsigned i = 0; i < sizeof v / sizeof v[0]; i++) {
		CPU c; cpu_reset(&c);
		cpu_write_csr(&c, v[i].csr, v[i].value);
		TEST_ASSERT_EQUAL_UINT32(v[i].value, cpu_read_csr(&c, v[i].csr));
	}
}

void test_cpu_s_registers_are_independent_of_the_m_registers(void) {
	const struct { uint16_t s, m; } pairs[] = {
		{CSR_SEPC, CSR_MEPC}, {CSR_SCAUSE, CSR_MCAUSE}, {CSR_STVAL, CSR_MTVAL},
		{CSR_SSCRATCH, CSR_MSCRATCH}, {CSR_STVEC, CSR_MTVEC},
	};
	for (unsigned i = 0; i < sizeof pairs / sizeof pairs[0]; i++) {
		CPU c; cpu_reset(&c);
		cpu_write_csr(&c, pairs[i].s, 0x100);
		cpu_write_csr(&c, pairs[i].m, 0x200);
		TEST_ASSERT_EQUAL_UINT32(0x100, cpu_read_csr(&c, pairs[i].s));
		TEST_ASSERT_EQUAL_UINT32(0x200, cpu_read_csr(&c, pairs[i].m));
	}
}

void test_cpu_satp_is_a_plain_32_bit_register(void) {
	CPU c; cpu_reset(&c);
	TEST_ASSERT_EQUAL_UINT32(0, cpu_read_csr(&c, CSR_SATP));
	cpu_write_csr(&c, CSR_SATP, 0xFFFFFFFF);          // MODE, a 9-bit ASID and a 22-bit PPN: all 32 bits exist
	TEST_ASSERT_EQUAL_UINT32(0xFFFFFFFF, cpu_read_csr(&c, CSR_SATP));
	cpu_write_csr(&c, CSR_SATP, 0x80012345);          // MODE = Sv32, ASID = 0, PPN = 0x12345
	TEST_ASSERT_EQUAL_UINT32(0x80012345, cpu_read_csr(&c, CSR_SATP));
	cpu_write_csr(&c, CSR_SATP, 0);
	TEST_ASSERT_EQUAL_UINT32(0, cpu_read_csr(&c, CSR_SATP));
}

void test_cpu_mstatus_sum_mxr_and_tvm_each_round_trip(void) {
	const uint32_t bits[] = {MSTATUS_SUM, MSTATUS_MXR, MSTATUS_TVM};
	for (unsigned i = 0; i < 3; i++) {
		CPU c; cpu_reset(&c);
		cpu_write_csr(&c, CSR_MSTATUS, 0x1800 | bits[i]);
		TEST_ASSERT_EQUAL_UINT32(0x1800 | bits[i], cpu_read_csr(&c, CSR_MSTATUS));
	}
}

/* ---- Delegation registers ---------------------------------------------------------------- */

void test_cpu_medeleg_writable_mask(void) {
	CPU c; cpu_reset(&c);
	cpu_write_csr(&c, CSR_MEDELEG, 0xFFFFFFFF);
	TEST_ASSERT_EQUAL_UINT32(0x0000B3FF, cpu_read_csr(&c, CSR_MEDELEG));
}

void test_cpu_medeleg_each_bit(void) {
	/* delegatable: 0-9 and the page faults 12, 13, 15. Not: 10 and 14 (reserved),
	 * 11 (ECALL from M), 16 (double trap), and everything from 17 up. */
	for (unsigned n = 0; n < 32; n++) {
		CPU c; cpu_reset(&c);
		cpu_write_csr(&c, CSR_MEDELEG, 1u << n);
		bool delegatable = n <= 9 || n == 12 || n == 13 || n == 15;
		uint32_t expected = delegatable ? (1u << n) : 0;
		char msg[32];
		snprintf(msg, sizeof msg, "medeleg bit %u", n);
		TEST_ASSERT_EQUAL_UINT32_MESSAGE(expected, cpu_read_csr(&c, CSR_MEDELEG), msg);
	}
}

void test_cpu_mideleg_writable_mask(void) {
	CPU c; cpu_reset(&c);
	cpu_write_csr(&c, CSR_MIDELEG, 0xFFFFFFFF);
	TEST_ASSERT_EQUAL_UINT32(0x222, cpu_read_csr(&c, CSR_MIDELEG));
	cpu_write_csr(&c, CSR_MIDELEG, 0x888);   // machine-level bits cannot be delegated
	TEST_ASSERT_EQUAL_UINT32(0, cpu_read_csr(&c, CSR_MIDELEG));
}

void test_cpu_medelegh_and_mstatush_are_read_only_zero(void) {
	const uint16_t csrs[] = {CSR_MEDELEGH, CSR_MSTATUSH};
	for (unsigned i = 0; i < 2; i++) {
		CPU c; cpu_reset(&c);
		cpu_write_csr(&c, csrs[i], 0xFFFFFFFF);
		TEST_ASSERT_EQUAL_UINT32(0, cpu_read_csr(&c, csrs[i]));
	}
}

void test_cpu_mideleg_write_feeds_the_sie_view(void) {
	CPU c; cpu_reset(&c);
	cpu_write_csr(&c, CSR_MIE, 0x222);
	cpu_write_csr(&c, CSR_MIDELEG, 0x22);          // SSI and STI delegated
	TEST_ASSERT_EQUAL_UINT32(0x22, cpu_read_csr(&c, CSR_SIE));
}

/* ---- PMP CSRs ---------------------------------------------------------------------------- */

/* Entry i's config byte, through the packed pmpcfg CSR. */
static uint8_t pmp_cfg(CPU *c, unsigned i) {
	return (uint8_t)(cpu_read_csr(c, CSR_PMPCFG0 + i / 4) >> (8 * (i % 4)));
}

/* Write only entry 0's byte: the other three bytes of pmpcfg0 are written as 0. */
static void write_pmp_cfg0_byte(CPU *c, uint8_t b) {
	cpu_write_csr(c, CSR_PMPCFG0, b);
}

void test_cpu_pmp_reads_zero_after_reset(void) {
	CPU c; cpu_reset(&c);
	for (unsigned i = 0; i < 16; i++) {
		TEST_ASSERT_EQUAL_UINT32(0, cpu_read_csr(&c, CSR_PMPCFG0 + i));
	}
	for (unsigned i = 0; i < 64; i++) {
		TEST_ASSERT_EQUAL_UINT32(0, cpu_read_csr(&c, CSR_PMPADDR0 + i));
	}
}

/* old byte (set first) -> written byte -> stored byte, on entry 0 */
void test_cpu_pmpcfg_byte_writes(void) {
	const struct { const char *why; uint8_t old, written, stored; } v[] = {
		{"NAPOT with RWX",               0x00, 0x1F, 0x1F},
		{"R=0 W=1 is reserved",          0x00, 0x02, 0x00},
		{"A=TOR applies, RWX stays 101", 0x05, 0x0A, 0x0D},
		{"A=NAPOT applies, RWX stays",   0x07, 0x1A, 0x1F},
		{"reserved bits 6:5 read 0",     0x00, 0x60, 0x00},
		{"L, A and RWX legal",           0x00, 0xFF, 0x9F},
		{"L applies, RWX stays 000",     0x00, 0x82, 0x80},
		{"WX with R=0 is reserved",      0x00, 0x06, 0x00},
		{"R is legal",                   0x00, 0x01, 0x01},
		{"X is legal",                   0x00, 0x04, 0x04},
		{"RW is legal",                  0x00, 0x03, 0x03},
		{"locked: write ignored",        0x9F, 0x00, 0x9F},
	};
	for (unsigned i = 0; i < sizeof v / sizeof v[0]; i++) {
		CPU c; cpu_reset(&c);
		write_pmp_cfg0_byte(&c, v[i].old);
		TEST_ASSERT_EQUAL_UINT8_MESSAGE(v[i].old, pmp_cfg(&c, 0), "setup");
		write_pmp_cfg0_byte(&c, v[i].written);
		TEST_ASSERT_EQUAL_UINT8_MESSAGE(v[i].stored, pmp_cfg(&c, 0), v[i].why);
	}
}

void test_cpu_pmpcfg_packs_four_entries_per_csr(void) {
	CPU c; cpu_reset(&c);
	cpu_write_csr(&c, CSR_PMPCFG0, 0x1F1B0F07);       // entries 0-3: 07, 0F, 1B, 1F
	TEST_ASSERT_EQUAL_UINT32(0x1F1B0F07, cpu_read_csr(&c, CSR_PMPCFG0));
	TEST_ASSERT_EQUAL_UINT8(0x07, c.pmpcfg[0]);
	TEST_ASSERT_EQUAL_UINT8(0x0F, c.pmpcfg[1]);
	TEST_ASSERT_EQUAL_UINT8(0x1B, c.pmpcfg[2]);
	TEST_ASSERT_EQUAL_UINT8(0x1F, c.pmpcfg[3]);

	cpu_write_csr(&c, CSR_PMPCFG0 + 3, 0x1F1B0F07);   // pmpcfg3 holds entries 12-15
	TEST_ASSERT_EQUAL_UINT32(0x1F1B0F07, cpu_read_csr(&c, CSR_PMPCFG0 + 3));
	TEST_ASSERT_EQUAL_UINT8(0x07, c.pmpcfg[12]);
	TEST_ASSERT_EQUAL_UINT8(0x1F, c.pmpcfg[15]);
	TEST_ASSERT_EQUAL_UINT32(0x1F1B0F07, cpu_read_csr(&c, CSR_PMPCFG0));   // entries 0-3 untouched
}

/* One write with four bytes: an illegal RWX keeps the old RWX, but A still applies. */
void test_cpu_pmpcfg_mixed_write_keeps_rwx_per_byte(void) {
	CPU c; cpu_reset(&c);
	cpu_write_csr(&c, CSR_PMPCFG0, 0x07070707);       // RWX everywhere
	cpu_write_csr(&c, CSR_PMPCFG0, 0x1F020A1A);       // bytes 1A, 0A, 02, 1F (entry 0 first)
	TEST_ASSERT_EQUAL_UINT32(0x1F070F1F, cpu_read_csr(&c, CSR_PMPCFG0));
}

void test_cpu_pmpcfg_locked_entry_survives_a_mixed_write(void) {
	CPU c; cpu_reset(&c);
	cpu_write_csr(&c, CSR_PMPCFG0, 0x079F0707);       // entry 2 is locked
	cpu_write_csr(&c, CSR_PMPCFG0, 0);
	TEST_ASSERT_EQUAL_UINT32(0x009F0000, cpu_read_csr(&c, CSR_PMPCFG0));
}

void test_cpu_pmpaddr_round_trips_all_32_bits(void) {
	const uint32_t values[] = {0xFFFFFFFF, 0x12345678, 0};
	for (unsigned n = 0; n < 16; n++) {
		for (unsigned i = 0; i < 3; i++) {
			CPU c; cpu_reset(&c);
			cpu_write_csr(&c, CSR_PMPADDR0 + n, values[i]);
			TEST_ASSERT_EQUAL_UINT32(values[i], cpu_read_csr(&c, CSR_PMPADDR0 + n));
		}
	}
	CPU c; cpu_reset(&c);
	for (unsigned n = 0; n < 16; n++) {
		cpu_write_csr(&c, CSR_PMPADDR0 + n, (n + 1) * 0x111);
	}
	for (unsigned n = 0; n < 16; n++) {
		TEST_ASSERT_EQUAL_UINT32((n + 1) * 0x111, cpu_read_csr(&c, CSR_PMPADDR0 + n));   // independent
	}
}

/* pmpaddr0..3 start as 0x100, 0x200, 0x300, 0x400; entry 2's config byte is then set to `cfg2`.
 * Returns which of the four writes of 0xAAAA landed, as a bitmask (bit n = pmpaddr n changed). */
static unsigned pmpaddr_writes_that_land(uint8_t cfg2) {
	CPU c; cpu_reset(&c);
	for (unsigned n = 0; n < 4; n++) {
		cpu_write_csr(&c, CSR_PMPADDR0 + n, (n + 1) * 0x100);
	}
	cpu_write_csr(&c, CSR_PMPCFG0, (uint32_t)cfg2 << 16);
	unsigned landed = 0;
	for (unsigned n = 0; n < 4; n++) {
		cpu_write_csr(&c, CSR_PMPADDR0 + n, 0xAAAA);
		if (cpu_read_csr(&c, CSR_PMPADDR0 + n) == 0xAAAA) {
			landed |= 1u << n;
		}
	}
	return landed;
}

void test_cpu_pmp_locking_protects_pmpaddr(void) {
	TEST_ASSERT_EQUAL_UINT_MESSAGE(0xF, pmpaddr_writes_that_land(0x08), "entry 2 TOR, not locked: all land");
	TEST_ASSERT_EQUAL_UINT_MESSAGE(0x9, pmpaddr_writes_that_land(0x88), "entry 2 locked TOR: 1 (neighbour) and 2 (own) ignored");
	TEST_ASSERT_EQUAL_UINT_MESSAGE(0xB, pmpaddr_writes_that_land(0x9F), "entry 2 locked NAPOT: only 2 ignored");
	TEST_ASSERT_EQUAL_UINT_MESSAGE(0xB, pmpaddr_writes_that_land(0x80), "entry 2 locked with A=OFF: only 2 ignored");
}

void test_cpu_pmp_locked_tor_in_the_last_entry_protects_pmpaddr14(void) {
	CPU c; cpu_reset(&c);
	cpu_write_csr(&c, CSR_PMPCFG0 + 3, 0x88000000);   // entry 15: locked TOR
	cpu_write_csr(&c, CSR_PMPADDR0 + 13, 0x1111);
	cpu_write_csr(&c, CSR_PMPADDR0 + 14, 0x2222);
	cpu_write_csr(&c, CSR_PMPADDR0 + 15, 0x3333);
	TEST_ASSERT_EQUAL_UINT32(0x1111, cpu_read_csr(&c, CSR_PMPADDR0 + 13));
	TEST_ASSERT_EQUAL_UINT32(0, cpu_read_csr(&c, CSR_PMPADDR0 + 14));
	TEST_ASSERT_EQUAL_UINT32(0, cpu_read_csr(&c, CSR_PMPADDR0 + 15));
}

void test_cpu_pmp_entries_16_to_63_are_read_only_zero(void) {
	CPU c; cpu_reset(&c);
	cpu_write_csr(&c, CSR_PMPCFG0 + 3, 0x1F1F1F1F);   // pmpcfg3 (entries 12-15): real
	cpu_write_csr(&c, CSR_PMPADDR0 + 15, 0xFFFFFFFF); // pmpaddr15: real
	TEST_ASSERT_EQUAL_UINT32(0x1F1F1F1F, cpu_read_csr(&c, CSR_PMPCFG0 + 3));
	TEST_ASSERT_EQUAL_UINT32(0xFFFFFFFF, cpu_read_csr(&c, CSR_PMPADDR0 + 15));
	for (unsigned i = 4; i < 16; i++) {               // pmpcfg4..15
		cpu_write_csr(&c, CSR_PMPCFG0 + i, 0xFFFFFFFF);
		TEST_ASSERT_EQUAL_UINT32(0, cpu_read_csr(&c, CSR_PMPCFG0 + i));
	}
	for (unsigned i = 16; i < 64; i++) {              // pmpaddr16..63
		cpu_write_csr(&c, CSR_PMPADDR0 + i, 0xFFFFFFFF);
		TEST_ASSERT_EQUAL_UINT32(0, cpu_read_csr(&c, CSR_PMPADDR0 + i));
	}
	TEST_ASSERT_EQUAL_UINT32(0x1F1F1F1F, cpu_read_csr(&c, CSR_PMPCFG0 + 3));   // real ones untouched
	TEST_ASSERT_EQUAL_UINT32(0xFFFFFFFF, cpu_read_csr(&c, CSR_PMPADDR0 + 15));
}

int main(void) {
	UNITY_BEGIN();
	RUN_TEST(test_cpu_write_read_roundtrip_x5);
	RUN_TEST(test_cpu_write_read_roundtrip_x31);
	RUN_TEST(test_cpu_x0_read_ignores_underlying_value);
	RUN_TEST(test_cpu_x0_write_is_discarded);
	RUN_TEST(test_cpu_write_to_x0_does_not_affect_other_registers);
	RUN_TEST(test_cpu_csr_write_read_roundtrip);
	RUN_TEST(test_cpu_csr_writes_are_independent);
	RUN_TEST(test_cpu_mtvec_mode_bits_read_as_zero);
	RUN_TEST(test_cpu_other_csrs_keep_low_bits);
	RUN_TEST(test_cpu_misa_reports_the_implemented_extensions);
	RUN_TEST(test_cpu_misa_writes_are_ignored);
	RUN_TEST(test_cpu_counteren_reads_zero_after_reset);
	RUN_TEST(test_cpu_counteren_writable_bits);
	RUN_TEST(test_cpu_counteren_registers_are_independent);
	RUN_TEST(test_cpu_sstatus_reads_the_s_view_of_mstatus);
	RUN_TEST(test_cpu_sstatus_writes_touch_only_the_s_bits);
	RUN_TEST(test_cpu_mie_and_mip_keep_only_the_supervisor_bits);
	RUN_TEST(test_cpu_sie_and_sip_show_only_delegated_interrupts);
	RUN_TEST(test_cpu_sie_writes_touch_only_delegated_bits);
	RUN_TEST(test_cpu_sip_writes_touch_only_ssip);
	RUN_TEST(test_cpu_stvec_drops_two_low_bits_and_sepc_drops_bit_zero);
	RUN_TEST(test_cpu_plain_s_registers_round_trip);
	RUN_TEST(test_cpu_s_registers_are_independent_of_the_m_registers);
	RUN_TEST(test_cpu_satp_is_a_plain_32_bit_register);
	RUN_TEST(test_cpu_mstatus_sum_mxr_and_tvm_each_round_trip);
	RUN_TEST(test_cpu_medeleg_writable_mask);
	RUN_TEST(test_cpu_medeleg_each_bit);
	RUN_TEST(test_cpu_mideleg_writable_mask);
	RUN_TEST(test_cpu_medelegh_and_mstatush_are_read_only_zero);
	RUN_TEST(test_cpu_mideleg_write_feeds_the_sie_view);
	RUN_TEST(test_cpu_pmp_reads_zero_after_reset);
	RUN_TEST(test_cpu_pmpcfg_byte_writes);
	RUN_TEST(test_cpu_pmpcfg_packs_four_entries_per_csr);
	RUN_TEST(test_cpu_pmpcfg_mixed_write_keeps_rwx_per_byte);
	RUN_TEST(test_cpu_pmpcfg_locked_entry_survives_a_mixed_write);
	RUN_TEST(test_cpu_pmpaddr_round_trips_all_32_bits);
	RUN_TEST(test_cpu_pmp_locking_protects_pmpaddr);
	RUN_TEST(test_cpu_pmp_locked_tor_in_the_last_entry_protects_pmpaddr14);
	RUN_TEST(test_cpu_pmp_entries_16_to_63_are_read_only_zero);
	RUN_TEST(test_cpu_id_csrs_read_zero);
	RUN_TEST(test_cpu_trigger_csrs_are_hardwired_zero);
	RUN_TEST(test_cpu_mstatus_reads_zero_after_reset);
	RUN_TEST(test_cpu_mstatus_writable_bits_round_trip);
	RUN_TEST(test_cpu_mstatus_unimplemented_bits_read_zero);
	RUN_TEST(test_cpu_mstatus_mpp_accepts_u_s_and_m);
	RUN_TEST(test_cpu_mstatus_mpp_reserved_write_keeps_old);
	RUN_TEST(test_cpu_mstatus_mpp_reserved_write_still_applies_other_fields);
	RUN_TEST(test_cpu_mepc_bit_zero_is_zero);
	RUN_TEST(test_cpu_counter_halves_round_trip);
	RUN_TEST(test_cpu_counter_half_writes_keep_the_other_half);
	RUN_TEST(test_cpu_counter_shadows_read_the_machine_counters);
	RUN_TEST(test_cpu_counter_shadows_ignore_writes);
	RUN_TEST(test_cpu_counter_write_marks_only_that_counter);
	RUN_TEST(test_cpu_reset_enters_machine_mode);
	RUN_TEST(test_cpu_reset_clears_all_other_state);
	RUN_TEST(test_cpu_test_helper_sets_requested_mode);
	return UNITY_END();
}

