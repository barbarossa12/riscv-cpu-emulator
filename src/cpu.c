#include <stdlib.h>
#include <stdint.h>
#include "log.h"
#include "cpu.h"
#include "pmp.h"
// The modes mstatus.MPP can hold: U, S and M. The encoding 2 is reserved.
static bool mpp_is_legal(uint32_t mpp) {
	return mpp == PRV_U || mpp == PRV_S || mpp == PRV_M;
}

void cpu_reset(CPU *cpu) {
	*cpu = (CPU){0};
	cpu->privilege_mode = PRV_M;
	clint_reset(&cpu->clint);
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

// The machine-level interrupts the platform is asserting. MEIP stays 0 until the PLIC exists (Milestone 8),
// and then the supervisor external interrupt read of mip must also OR in the controller's signal
// (machine.adoc:1436-1441).
static uint32_t cpu_platform_pending(const CPU *cpu) {
	uint32_t lines = 0;
	if (clint_timer_pending(&cpu->clint)) {
		lines |= INT_MTI;
	}
	if (clint_software_pending(&cpu->clint)) {
		lines |= INT_MSI;
	}
	return lines;
}

uint32_t cpu_read_csr(CPU *cpu, uint16_t addr){
	if(addr > 0xFFF) {
		LOG_FATAL("CPU CSR register value larger than 0xFFF: 0x%x", addr);
	}
	if(pmp_csr_exists(addr)) {
		return pmp_csr_read(cpu, addr);
	}
	switch(addr) {
		case CSR_SSTATUS:
			return cpu->csr[CSR_MSTATUS] & SSTATUS_MASK;
		case CSR_SIE:   // only delegated interrupts are visible
			return cpu->csr[CSR_MIE] & cpu->csr[CSR_MIDELEG] & INT_S_ALL;
		case CSR_SIP:
			return cpu->csr[CSR_MIP] & cpu->csr[CSR_MIDELEG] & INT_S_ALL;
		case CSR_MIP:   // the stored supervisor bits, and the machine bits as the platform drives them
			return cpu->csr[CSR_MIP] | cpu_platform_pending(cpu);
		case CSR_MSTATUSH:
		case CSR_MEDELEGH:
			return 0;
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
	if(pmp_csr_exists(addr)) {
		pmp_csr_write(cpu, addr, value);
		return;
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
		case CSR_MIE:
			cpu->csr[addr] = value & (INT_S_ALL | INT_M_ALL);
			return;
		case CSR_MIP:   // MSIP/MTIP/MEIP are read-only: the platform sets them
			cpu->csr[addr] = value & INT_S_ALL;
			return;
		case CSR_SSTATUS:
			cpu_write_csr(cpu, CSR_MSTATUS,
			              (cpu->csr[CSR_MSTATUS] & ~SSTATUS_MASK) | (value & SSTATUS_MASK));
			return;
		case CSR_SIE: {
			uint32_t m = cpu->csr[CSR_MIDELEG] & INT_S_ALL;      // delegated bits only
			cpu->csr[CSR_MIE] = (cpu->csr[CSR_MIE] & ~m) | (value & m);
			return;
		}
		case CSR_SIP: {
			uint32_t m = cpu->csr[CSR_MIDELEG] & INT_SSI;        // only SSIP is writable in sip
			cpu->csr[CSR_MIP] = (cpu->csr[CSR_MIP] & ~m) | (value & m);
			return;
		}
		case CSR_MEDELEG:
			cpu->csr[addr] = value & MEDELEG_WRITABLE;
			return;
		case CSR_MIDELEG:
			cpu->csr[addr] = value & MIDELEG_WRITABLE;
			return;
		case CSR_MSTATUSH:
		case CSR_MEDELEGH:
			return;
		case CSR_MCOUNTEREN:
		case CSR_SCOUNTEREN:
			cpu->csr[addr] = value & COUNTEREN_WRITABLE; // WARL: other bits read-only 0
			return;
		case CSR_MEPC:
		case CSR_SEPC:
			value &= ~0x1u; // IALIGN=16: only bit 0 is always 0
			break;
		case CSR_MTVEC:
		case CSR_STVEC:
			// MODE is WARL: only Direct (0) is supported, so MODE reads back as 0
			value &= ~0x3u;
			break;
	}
	cpu->csr[addr] = value;
}
