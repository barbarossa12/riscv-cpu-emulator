#include "unity.h"
#include "../src/cpu.h"

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
	cpu_write_csr(&test_cpu, CSR_MEPC, 0xAAAA);
	cpu_write_csr(&test_cpu, CSR_MCAUSE, 0xBBBB);
	TEST_ASSERT_EQUAL_UINT32(0xAAAA, cpu_read_csr(&test_cpu, CSR_MEPC));
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

void test_cpu_misa_reports_rv32i(void) {
	CPU test_cpu = {0};
	TEST_ASSERT_EQUAL_UINT32(0x40000100, cpu_read_csr(&test_cpu, CSR_MISA));
}

void test_cpu_misa_writes_are_ignored(void) {
	CPU test_cpu = {0};
	cpu_write_csr(&test_cpu, CSR_MISA, 0xFFFFFFFF); // tries to enable C, S, U, F, ...
	TEST_ASSERT_EQUAL_UINT32(0x40000100, cpu_read_csr(&test_cpu, CSR_MISA));
	cpu_write_csr(&test_cpu, CSR_MISA, 0);
	TEST_ASSERT_EQUAL_UINT32(0x40000100, cpu_read_csr(&test_cpu, CSR_MISA));
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

void test_cpu_mstatus_mpp_is_m_from_reset(void) {
	CPU test_cpu = {0};
	TEST_ASSERT_EQUAL_UINT32(MSTATUS_MPP, cpu_read_csr(&test_cpu, CSR_MSTATUS));
}

void test_cpu_mstatus_mpp_ignores_writes(void) {
	CPU test_cpu = {0};
	cpu_write_csr(&test_cpu, CSR_MSTATUS, 0);      // MPP = U
	TEST_ASSERT_EQUAL_UINT32(MSTATUS_MPP, cpu_read_csr(&test_cpu, CSR_MSTATUS) & MSTATUS_MPP);
	cpu_write_csr(&test_cpu, CSR_MSTATUS, 0x800);  // MPP = S
	TEST_ASSERT_EQUAL_UINT32(MSTATUS_MPP, cpu_read_csr(&test_cpu, CSR_MSTATUS) & MSTATUS_MPP);
}

void test_cpu_mstatus_other_bits_round_trip(void) {
	CPU test_cpu = {0};
	cpu_write_csr(&test_cpu, CSR_MSTATUS, MSTATUS_MIE | MSTATUS_MPIE);
	TEST_ASSERT_EQUAL_UINT32(MSTATUS_MIE | MSTATUS_MPIE | MSTATUS_MPP, cpu_read_csr(&test_cpu, CSR_MSTATUS));
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
	RUN_TEST(test_cpu_misa_reports_rv32i);
	RUN_TEST(test_cpu_misa_writes_are_ignored);
	RUN_TEST(test_cpu_id_csrs_read_zero);
	RUN_TEST(test_cpu_trigger_csrs_are_hardwired_zero);
	RUN_TEST(test_cpu_mstatus_mpp_is_m_from_reset);
	RUN_TEST(test_cpu_mstatus_mpp_ignores_writes);
	RUN_TEST(test_cpu_mstatus_other_bits_round_trip);
	RUN_TEST(test_cpu_counter_halves_round_trip);
	RUN_TEST(test_cpu_counter_half_writes_keep_the_other_half);
	RUN_TEST(test_cpu_counter_shadows_read_the_machine_counters);
	RUN_TEST(test_cpu_counter_shadows_ignore_writes);
	RUN_TEST(test_cpu_counter_write_marks_only_that_counter);
	return UNITY_END();
}

