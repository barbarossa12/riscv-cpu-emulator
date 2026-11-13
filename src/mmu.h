#pragma once
#include <stdint.h>
#include "cpu.h"
#include "mem.h"

typedef enum {
	ACCESS_FETCH,
	ACCESS_LOAD,
	ACCESS_STORE,
} AccessType;

typedef enum {
	XLATE_OK,
	XLATE_PAGE_FAULT,
	XLATE_ACCESS_FAULT,
} XlateResult;

// Sv32 translation of one virtual address. `priv` is the effective privilege the access runs at
// (the caller applies MPRV for loads and stores). In M-mode, or with satp.MODE = Bare, pa = va.
// The walk's own PTE reads and writes are PMP-checked as S-mode accesses.
XlateResult mmu_translate(CPU *cpu, Memory *mem, uint32_t va, AccessType type,
                          PrivMode priv, uint32_t *pa);

// True when an access at this privilege goes through the page tables: satp.MODE is Sv32 and the
// privilege is S or U.
bool mmu_enabled(const CPU *cpu, PrivMode priv);

// SFENCE.VMA: drops cached translations. all_addresses is rs1 = x0 (else only the page holding va),
// all_asids is rs2 = x0 (else only that ASID, and global entries stay). "x0" is the register number,
// not a value of 0.
void mmu_flush(CPU *cpu, uint32_t va, uint32_t asid, bool all_addresses, bool all_asids);
