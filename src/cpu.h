#pragma once
#include <stdint.h>
#include <stdbool.h>

#define CSR_MSTATUS  0x300
#define CSR_MISA      0x301
#define CSR_SSTATUS  0x100
#define CSR_SIE      0x104
#define CSR_STVEC    0x105
#define CSR_SSCRATCH 0x140
#define CSR_SEPC     0x141
#define CSR_SCAUSE   0x142
#define CSR_STVAL    0x143
#define CSR_SIP      0x144
#define CSR_SATP     0x180

#define CSR_PMPCFG0  0x3A0   // pmpcfg0..pmpcfg15 = 0x3A0..0x3AF
#define CSR_PMPADDR0 0x3B0   // pmpaddr0..pmpaddr63 = 0x3B0..0x3EF

#define PMP_ENTRIES 16       // entries 16..63 exist as CSRs but are read-only 0
#define CSR_MEDELEG  0x302
#define CSR_MIDELEG  0x303
#define CSR_MSTATUSH 0x310   // RV32 upper half of mstatus: every field is read-only 0 (little-endian only,
                             // no H, Zicfilp or Smdbltrp), so it always reads 0
#define CSR_MEDELEGH 0x312   // upper half of medeleg on RV32: no bits implemented, reads 0
#define CSR_MIE      0x304
#define CSR_MTVEC    0x305
#define CSR_MSCRATCH 0x340
#define CSR_MEPC     0x341
#define CSR_MCAUSE   0x342
#define CSR_MTVAL    0x343
#define CSR_MIP      0x344
#define CSR_TSELECT   0x7A0
#define CSR_TDATA1    0x7A1
#define CSR_TDATA2    0x7A2
#define CSR_MVENDORID 0xF11
#define CSR_MARCHID   0xF12
#define CSR_MIMPID    0xF13
#define CSR_MHARTID   0xF14

// Machine counters (64-bit, split into low/high halves on RV32) and their
// read-only user-level shadows
#define CSR_MCYCLE    0xB00
#define CSR_MINSTRET  0xB02
#define CSR_MCYCLEH   0xB80
#define CSR_MINSTRETH 0xB82
#define CSR_CYCLE     0xC00
#define CSR_INSTRET   0xC02
#define CSR_CYCLEH    0xC80
#define CSR_INSTRETH  0xC82

// MXL=1 (RV32) in bits 31:30, plus I (bit 8), M (bit 12), S-mode (bit 18) and U-mode (bit 20)
#define MISA_VALUE    0x40141100u

// Counter-enable CSRs. Only the CY and IR bits exist: TM is absent until `time` exists
// (Milestone 6) and there are no hpmcounters, so every other bit is read-only 0.
#define CSR_SCOUNTEREN 0x106
#define CSR_MCOUNTEREN 0x306
#define COUNTEREN_CY (1u << 0)
#define COUNTEREN_IR (1u << 2)
#define COUNTEREN_WRITABLE (COUNTEREN_CY | COUNTEREN_IR)

// Supervisor interrupt bits, the same positions in mip, mie, sip and sie
#define INT_SSI   (1u << 1)
#define INT_STI   (1u << 5)
#define INT_SEI   (1u << 9)
#define INT_S_ALL (INT_SSI | INT_STI | INT_SEI)   // 0x222

#define MSTATUS_SIE  (1u << 1)
#define MSTATUS_MIE  (1u << 3)
#define MSTATUS_SPIE (1u << 5)
#define MSTATUS_MPIE (1u << 7)
#define MSTATUS_SPP  (1u << 8)
#define MSTATUS_MPP  (3u << 11)
#define MSTATUS_MPRV (1u << 17)
#define MSTATUS_TW   (1u << 21)
#define MSTATUS_TSR  (1u << 22)

// Exceptions that can be delegated to S-mode: mcause codes 0-9, and the page faults 12, 13, 15.
// 11 (ECALL from M) and 16 (double trap) can never be delegated.
#define MEDELEG_WRITABLE 0xB3FFu
#define MIDELEG_WRITABLE INT_S_ALL

// The part of mstatus that sstatus shows
#define SSTATUS_MASK (MSTATUS_SIE | MSTATUS_SPIE | MSTATUS_SPP)   // 0x122

// Bits of mstatus that exist. Everything else (UBE, FS, VS, XS, SUM, MXR,
// TVM, SD, ...) is read-only 0 for now.
#define MSTATUS_WRITABLE (MSTATUS_SIE | MSTATUS_MIE | MSTATUS_SPIE | MSTATUS_MPIE | \
                          MSTATUS_SPP | MSTATUS_MPP | MSTATUS_MPRV | MSTATUS_TW | MSTATUS_TSR)

// Privilege modes, encoded the same way as mstatus.MPP (the value 2 is reserved)
typedef enum {
	PRV_U = 0,
	PRV_S = 1,
	PRV_M = 3,
} PrivMode;


typedef struct CPU {
	uint32_t regs[32];
	uint32_t pc;
	uint32_t csr[4096]; // indexed directly by the 12-bit CSR address
	uint8_t  pmpcfg[PMP_ENTRIES];    // one config byte per implemented PMP entry
	uint32_t pmpaddr[PMP_ENTRIES];   // address bits 33:2 of each entry's region
	PrivMode privilege_mode; // current mode. A zeroed CPU is in U-mode, so call cpu_reset()

	uint64_t mcycle;    // behind mcycle/mcycleh and the cycle/cycleh shadows
	uint64_t minstret;  // behind minstret/minstreth and the instret/instreth shadows

	// Per-step bookkeeping for cpu_step: a counter write suppresses that
	// counter's own increment, and an instruction that traps does not retire.
	bool mcycle_written;
	bool minstret_written;
	bool trapped;
} CPU;

// Zero all state and enter M-mode, as at hart reset
void cpu_reset(CPU *cpu);

uint32_t cpu_read_reg(CPU *cpu, uint8_t index);
void cpu_write_reg(CPU *cpu, uint8_t index, uint32_t value);

uint32_t cpu_read_csr(CPU *cpu, uint16_t addr);
void cpu_write_csr(CPU *cpu, uint16_t addr, uint32_t value);

