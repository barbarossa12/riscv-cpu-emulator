#pragma once
#include <stdint.h>

#define CSR_MSTATUS  0x300
#define CSR_MISA      0x301
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

// MXL=1 (RV32) in bits 31:30, and the I extension at bit 8
#define MISA_VALUE    0x40000100u

#define MSTATUS_MIE  (1u << 3)
#define MSTATUS_MPIE (1u << 7)
#define MSTATUS_MPP  (3u << 11)


typedef struct CPU {
	uint32_t regs[32];
	uint32_t pc;
	uint32_t csr[4096]; // indexed directly by the 12-bit CSR address
} CPU;

uint32_t cpu_read_reg(CPU *cpu, uint8_t index);
void cpu_write_reg(CPU *cpu, uint8_t index, uint32_t value);

uint32_t cpu_read_csr(CPU *cpu, uint16_t addr);
void cpu_write_csr(CPU *cpu, uint16_t addr, uint32_t value);

