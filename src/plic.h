#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "mem.h"

// The platform-level interrupt controller, in the SiFive/QEMU virt layout that Linux and OpenSBI use, for one
// hart. Register map and rules: RISC-V PLIC specification 1.0.0 (riscv/riscv-plic-spec, not part of the ISA
// manual). Which context belongs to which hart and mode is the platform's choice; here context 0 is hart 0 in
// M-mode (it drives MEIP) and context 1 is hart 0 in S-mode (it drives SEIP), as OpenSBI's
// fdt_irqchip_plic.c expects.
#define PLIC_BASE       0x0C000000u
#define PLIC_SIZE       0x4000000u
#define PLIC_NSOURCES   31u          // IDs 1 to 31; ID 0 means "no interrupt" and has no registers
#define PLIC_NCONTEXTS  2u
#define PLIC_PRIO_MASK  0x7u         // priority and threshold are WARL fields with 3 bits (0 to 7)
#define PLIC_PENDING    0x1000u      // one 32-bit word: bit n is source n
#define PLIC_ENABLE     0x2000u      // one word per context, 0x80 apart
#define PLIC_ENABLE_STRIDE  0x80u
#define PLIC_CONTEXT    0x200000u    // 0x1000 per context: +0 threshold, +4 claim/complete
#define PLIC_CONTEXT_STRIDE 0x1000u

typedef struct Plic {
	uint32_t priority[PLIC_NSOURCES + 1];   // index 0 unused
	uint32_t pending;                        // bit 0 always 0
	uint32_t enable[PLIC_NCONTEXTS];         // bit 0 always 0
	uint32_t threshold[PLIC_NCONTEXTS];
} Plic;

// The PLIC spec leaves reset values open; everything is 0 (no source can interrupt).
void plic_reset(Plic *p);
// 32-bit accesses only. Every offset that is not a register below is an access fault: priority words 1 to 31,
// the pending word, one enable word and the threshold and claim words for each context. Pending is read-only
// (a write is ignored). Claim reads 0 and complete is ignored until the gateway exists (Milestone 8.2).
Device plic_device(Plic *p);   // for mem_add_device
