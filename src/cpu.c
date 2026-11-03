#include <stdlib.h>
#include <stdint.h>
#include "log.h"
#include "cpu.h"
uint32_t cpu_read_reg(CPU *cpu, uint8_t index) {
	if(index > 31) {
		LOG_FATAL("CPU read register value larger than 32: %d", index);
	}

	if(index == 0) {
		return (uint32_t) 0;
	}

	return cpu->regs[index];

}

void cpu_write_reg(CPU *cpu, uint8_t index, uint32_t value) {
	if(index > 31) {
		LOG_FATAL("CPU write register value larger than 32: %d", index);
	}

	if(index == 0) {
		return;
	}

	cpu->regs[index] = value;
}

uint32_t cpu_read_csr(CPU *cpu, uint16_t addr){
	if(addr > 0xFFF) {
		LOG_FATAL("CPU CSR register value larger than 0xFFF: 0x%x", addr);
	}
	switch(addr) {
		case CSR_MSTATUS:
			// MPP is WARL and can hold only implemented modes. M-mode is the only one,
			// so MPP is hardwired to M. Forced here so it holds from reset, whatever
			// was written.
			return cpu->csr[addr] | MSTATUS_MPP;
		case CSR_MISA:
			return MISA_VALUE;
		case CSR_MVENDORID:
		case CSR_MARCHID:
		case CSR_MIMPID:
		case CSR_MHARTID:
		case CSR_TSELECT:  // no triggers implemented
		case CSR_TDATA1:
		case CSR_TDATA2:
			return 0;
		default:
			return cpu->csr[addr];
	}
}

void cpu_write_csr(CPU *cpu, uint16_t addr, uint32_t value) {
	if(addr > 0xFFF) {
		LOG_FATAL("CPU CSR register value larger than 0xFFF: 0x%x", addr);
	}
	switch(addr) {
		case CSR_MISA:    // WARL: no extension can be toggled, so writes are ignored
		case CSR_TSELECT: // no triggers implemented: hardwired to 0
		case CSR_TDATA1:
		case CSR_TDATA2:
			return;
		case CSR_MTVEC:
			// MODE is WARL: only Direct (0) is supported, so MODE reads back as 0
			value &= ~0x3u;
			break;
	}
	cpu->csr[addr] = value;
}
