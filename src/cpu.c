#include <stdlib.h>
#include <stdint.h>
#include "log.h"
#include "cpu.h"
// The modes mstatus.MPP can hold: U, S and M. The encoding 2 is reserved.
static bool mpp_is_legal(uint32_t mpp) {
	return mpp == PRV_U || mpp == PRV_S || mpp == PRV_M;
}

void cpu_reset(CPU *cpu) {
	*cpu = (CPU){0};
	cpu->privilege_mode = PRV_M;
}

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
		case CSR_MISA:
			return MISA_VALUE;
		case CSR_MCYCLE:
		case CSR_CYCLE:
			return (uint32_t)cpu->mcycle;
		case CSR_MCYCLEH:
		case CSR_CYCLEH:
			return (uint32_t)(cpu->mcycle >> 32);
		case CSR_MINSTRET:
		case CSR_INSTRET:
			return (uint32_t)cpu->minstret;
		case CSR_MINSTRETH:
		case CSR_INSTRETH:
			return (uint32_t)(cpu->minstret >> 32);
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
		case CSR_CYCLE:   // read-only shadows of the machine counters
		case CSR_CYCLEH:
		case CSR_INSTRET:
		case CSR_INSTRETH:
			return;
		case CSR_MCYCLE:
			cpu->mcycle = (cpu->mcycle & 0xFFFFFFFF00000000ull) | value;
			cpu->mcycle_written = true;
			return;
		case CSR_MCYCLEH:
			cpu->mcycle = (cpu->mcycle & 0xFFFFFFFFull) | ((uint64_t)value << 32);
			cpu->mcycle_written = true;
			return;
		case CSR_MINSTRET:
			cpu->minstret = (cpu->minstret & 0xFFFFFFFF00000000ull) | value;
			cpu->minstret_written = true;
			return;
		case CSR_MINSTRETH:
			cpu->minstret = (cpu->minstret & 0xFFFFFFFFull) | ((uint64_t)value << 32);
			cpu->minstret_written = true;
			return;
		case CSR_MSTATUS: {
			// MPP is WARL: a value that is not an implemented mode (the reserved 2)
			// leaves MPP unchanged. The other fields of the write still apply.
			uint32_t mpp = (value & MSTATUS_MPP) >> 11;
			uint32_t new_val = value & MSTATUS_WRITABLE;
			if (!mpp_is_legal(mpp)) {
				new_val = (new_val & ~MSTATUS_MPP) | (cpu->csr[addr] & MSTATUS_MPP);
			}
			cpu->csr[addr] = new_val;
			return;
		}
		case CSR_MEPC:
			value &= ~0x3u; // IALIGN=32: the low two bits are always 0
			break;
		case CSR_MTVEC:
			// MODE is WARL: only Direct (0) is supported, so MODE reads back as 0
			value &= ~0x3u;
			break;
	}
	cpu->csr[addr] = value;
}
