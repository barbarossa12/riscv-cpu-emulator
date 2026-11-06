#include "unity.h"
#include "../src/decode.h"

void setUp(void) {} // Unity calles this before each test function
void tearDown(void) {} // Unity calls this after eacht test function


void test_r_type1(void) {
	DecodedInstruction test_decode;
	test_decode = decode_instruction(0x003100B3);
	TEST_ASSERT_EQUAL_UINT8(0x33, test_decode.opcode);
	TEST_ASSERT_EQUAL_UINT8(1, test_decode.rd);
	TEST_ASSERT_EQUAL_UINT8(2, test_decode.rs1);
	TEST_ASSERT_EQUAL_UINT8(3, test_decode.rs2);
	TEST_ASSERT_EQUAL_UINT8(0, test_decode.funct3);
	TEST_ASSERT_EQUAL_UINT8(0, test_decode.funct7);
	TEST_ASSERT_EQUAL_INT(FMT_R, test_decode.format);
}

void test_r_type2(void) {
	DecodedInstruction test_decode;
	test_decode = decode_instruction(0x403100B3);
	TEST_ASSERT_EQUAL_UINT8(0x20, test_decode.funct7);
}

void test_i_type_immediate(void) {
	int32_t returnval = get_immediate(FMT_I, 0xFFF30293);
	TEST_ASSERT_EQUAL_INT32(-1, returnval);
}

void test_s_type_immediate(void) {
	int32_t returnval = get_immediate(FMT_S, 0x06312223);
	TEST_ASSERT_EQUAL_INT32(100, returnval);
}

void test_b_type_immediate(void) {
	int32_t returnval = get_immediate(FMT_B, 0xFE208EE3);
	TEST_ASSERT_EQUAL_INT32(-4, returnval);
}

void test_u_type_immediate(void) {
	int32_t returnval = get_immediate(FMT_U, 0x123452B7);
	TEST_ASSERT_EQUAL_INT32(0x12345000, returnval);
}

void test_j_type_immediate(void) {
	int32_t returnval = get_immediate(FMT_J, 0xFFFFF0EF);
	TEST_ASSERT_EQUAL_INT32(-2, returnval);
}

void test_opcode_lui(void) {
	DecodedInstruction test_instruction = decode_instruction(0x37);
	TEST_ASSERT_EQUAL_INT(FMT_U, test_instruction.format);
}

void test_opcode_auipc(void) {
	DecodedInstruction test_instruction = decode_instruction(0x17);
	TEST_ASSERT_EQUAL_INT(FMT_U, test_instruction.format);
}

void test_opcode_jal(void) {
	DecodedInstruction test_instruction = decode_instruction(0x6F);
	TEST_ASSERT_EQUAL_INT(FMT_J, test_instruction.format);
}

void test_opcode_jalr(void) {
	DecodedInstruction test_instruction = decode_instruction(0x67);
	TEST_ASSERT_EQUAL_INT(FMT_I, test_instruction.format);
}

void test_opcode_branch(void) {
	DecodedInstruction test_instruction = decode_instruction(0x63);
	TEST_ASSERT_EQUAL_INT(FMT_B, test_instruction.format);
}

void test_opcode_load(void) {
	DecodedInstruction test_instruction = decode_instruction(0x03);
	TEST_ASSERT_EQUAL_INT(FMT_I, test_instruction.format);
}

void test_opcode_store(void) {
	DecodedInstruction test_instruction = decode_instruction(0x23);
	TEST_ASSERT_EQUAL_INT(FMT_S, test_instruction.format);
}

void test_opcode_alu_imm(void) {
	DecodedInstruction test_instruction = decode_instruction(0x13);
	TEST_ASSERT_EQUAL_INT(FMT_I, test_instruction.format);
}

void test_opcode_alu_reg(void) {
	DecodedInstruction test_instruction = decode_instruction(0x33);
	TEST_ASSERT_EQUAL_INT(FMT_R, test_instruction.format);
}

void test_decode_length_is_four(void) {
	TEST_ASSERT_EQUAL_UINT8(4, decode_instruction(0x003100B3).length);   // ADD
	TEST_ASSERT_EQUAL_UINT8(4, decode_instruction(0x00500093).length);   // ADDI
	TEST_ASSERT_EQUAL_UINT8(4, decode_instruction(0xFFFFFFFF).length);   // unknown opcode
}

void test_decode_keeps_the_raw_bits(void) {
	TEST_ASSERT_EQUAL_HEX32(0x003100B3, decode_instruction(0x003100B3).raw);
}

void test_opcode_amo(void) {
	DecodedInstruction test_instruction = decode_instruction(0x2F);
	TEST_ASSERT_EQUAL_INT(FMT_R, test_instruction.format);
	TEST_ASSERT_EQUAL_INT(0, test_instruction.imm);
}

void test_amo_fields(void) {
	DecodedInstruction d = decode_instruction(0xE03120AF);   // AMOMAXU.W x1, x3, (x2)
	TEST_ASSERT_EQUAL_UINT8(0x2F, d.opcode);
	TEST_ASSERT_EQUAL_UINT8(1, d.rd);
	TEST_ASSERT_EQUAL_UINT8(2, d.rs1);
	TEST_ASSERT_EQUAL_UINT8(3, d.rs2);
	TEST_ASSERT_EQUAL_UINT8(2, d.funct3);
	TEST_ASSERT_EQUAL_UINT8(0x70, d.funct7);                  // funct5 0x1C, aq 0, rl 0
}

void test_opcode_fence(void) {
	uint32_t raw = (0xFFFFFFFF & ~0x7F) | 0x0F;
	DecodedInstruction test_instruction = decode_instruction(raw);
	TEST_ASSERT_EQUAL_INT(FMT_UNKNOWN, test_instruction.format);
	TEST_ASSERT_EQUAL_INT(0, test_instruction.imm);
}

void test_opcode_system(void) {
	DecodedInstruction test_instruction = decode_instruction(0x73);
	TEST_ASSERT_EQUAL_INT(FMT_I, test_instruction.format);
}

void test_opcode_unmapped(void) {
	DecodedInstruction test_instruction = decode_instruction(0x7F);
	TEST_ASSERT_EQUAL_INT(FMT_I, test_instruction.format);
}

// testing the immediate values
void test_i_type_immediate_zero(void) {
	int32_t returnval = get_immediate(FMT_I, 0x00000000);
	TEST_ASSERT_EQUAL_INT32(0, returnval);
}

void test_i_type_immediate_max_positive(void) {
	int32_t returnval = get_immediate(FMT_I, 0x7FF00000);
	TEST_ASSERT_EQUAL_INT32(2047, returnval);
}

void test_i_type_immediate_max_negative(void) {
	int32_t returnval = get_immediate(FMT_I, 0x80000000);
	TEST_ASSERT_EQUAL_INT32(-2048, returnval);
}

void test_s_type_immediate_zero(void) {
	int32_t returnval = get_immediate(FMT_S, 0x00000000);
	TEST_ASSERT_EQUAL_INT32(0, returnval);
}

void test_s_type_immediate_max_positive(void) {
	int32_t returnval = get_immediate(FMT_S, 0x7E000F80);
	TEST_ASSERT_EQUAL_INT32(2047, returnval);
}

void test_s_type_immediate_max_negative(void) {
	int32_t returnval = get_immediate(FMT_S, 0x80000000);
	TEST_ASSERT_EQUAL_INT32(-2048, returnval);
}

void test_b_type_immediate_zero(void) {
	int32_t returnval = get_immediate(FMT_B, 0x00000000);
	TEST_ASSERT_EQUAL_INT32(0, returnval);
}

void test_b_type_immediate_max_positive(void) {
	int32_t returnval = get_immediate(FMT_B, 0x7E000F80);
	TEST_ASSERT_EQUAL_INT32(4094, returnval);
}

void test_b_type_immediate_max_negative(void) {
	int32_t returnval = get_immediate(FMT_B, 0x80000000);
	TEST_ASSERT_EQUAL_INT32(-4096, returnval);
}

void test_j_type_immediate_zero(void) {
	int32_t returnval = get_immediate(FMT_J, 0x00000000);
	TEST_ASSERT_EQUAL_INT32(0, returnval);
}

void test_j_type_immediate_max_positive(void) {
	int32_t returnval = get_immediate(FMT_J, 0x7FFFF000);
	TEST_ASSERT_EQUAL_INT32(1048574, returnval);
}

void test_j_type_immediate_max_negative(void) {
	int32_t returnval = get_immediate(FMT_J, 0x80000000);
	TEST_ASSERT_EQUAL_INT32(-1048576, returnval);
}

void test_u_type_immediate_zero(void) {
	int32_t returnval = get_immediate(FMT_U, 0x00000000);
	TEST_ASSERT_EQUAL_INT32(0, returnval);
}

void test_u_type_immediate_all_bits_set(void) {
	int32_t returnval = get_immediate(FMT_U, 0xFFFFFFFF);
	TEST_ASSERT_EQUAL_INT32(-4096, returnval);  /* 0xFFFFF000 as int32_t */
}



int main(void) {
	UNITY_BEGIN();
	RUN_TEST(test_r_type1);
	RUN_TEST(test_r_type2);
	RUN_TEST(test_i_type_immediate);
	RUN_TEST(test_s_type_immediate);
	RUN_TEST(test_b_type_immediate);
	RUN_TEST(test_u_type_immediate);
	RUN_TEST(test_j_type_immediate);
	RUN_TEST(test_opcode_lui);
	RUN_TEST(test_opcode_auipc);
	RUN_TEST(test_opcode_jal);
	RUN_TEST(test_opcode_jalr);
	RUN_TEST(test_opcode_branch);
	RUN_TEST(test_opcode_load);
	RUN_TEST(test_opcode_store);
	RUN_TEST(test_opcode_alu_imm);
	RUN_TEST(test_opcode_alu_reg);
	RUN_TEST(test_opcode_fence);
	RUN_TEST(test_decode_length_is_four);
	RUN_TEST(test_decode_keeps_the_raw_bits);
	RUN_TEST(test_opcode_amo);
	RUN_TEST(test_amo_fields);
	RUN_TEST(test_opcode_system);
	RUN_TEST(test_opcode_unmapped);
	RUN_TEST(test_i_type_immediate_zero);
	RUN_TEST(test_i_type_immediate_max_positive);
	RUN_TEST(test_i_type_immediate_max_negative);
	RUN_TEST(test_s_type_immediate_zero);
	RUN_TEST(test_s_type_immediate_max_positive);
	RUN_TEST(test_s_type_immediate_max_negative);
	RUN_TEST(test_b_type_immediate_zero);
	RUN_TEST(test_b_type_immediate_max_positive);
	RUN_TEST(test_b_type_immediate_max_negative);
	RUN_TEST(test_j_type_immediate_zero);
	RUN_TEST(test_j_type_immediate_max_positive);
	RUN_TEST(test_j_type_immediate_max_negative);
	RUN_TEST(test_u_type_immediate_zero);
	RUN_TEST(test_u_type_immediate_all_bits_set);
	return UNITY_END();
}
