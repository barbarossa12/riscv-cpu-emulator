#include <stdio.h>
#include "cpu.h"
#include "decode.h"
#include "mem.h"

int main(){
	CPU cpu = {0};
	Memory mem;
	mem_init(&mem, 16);
	mem_write32(&mem, 0, 0x00500093);  // addi x1, x0, 5
	mem_write32(&mem, 4, 0x00A08093);  // addi x1, x1, 10


	// fetch loop
	for(int i = 0; i < 2; i++) {
		uint32_t raw = mem_read32(&mem, cpu.pc);
		DecodedInstruction instr = decode_instruction(raw);

		// execute will go here.
		printf("%d", instr.raw);
		cpu.pc += 4; // increase the program counter by 4 bytes.
	}
	mem_free(&mem);
	return 0;
}
