#include "mmu.h"
#include "pmp.h"

#define SATP_MODE_SV32 (1u << 31)
#define SATP_PPN_MASK  0x3FFFFFu

#define PTE_V (1u << 0)
#define PTE_R (1u << 1)
#define PTE_W (1u << 2)
#define PTE_X (1u << 3)
#define PTE_U (1u << 4)
#define PTE_A (1u << 6)
#define PTE_D (1u << 7)

XlateResult mmu_translate(CPU *cpu, Memory *mem, uint32_t va, AccessType type,
                          PrivMode priv, uint32_t *pa) {
	uint32_t satp = cpu_read_csr(cpu, CSR_SATP);
	if (priv == PRV_M || !(satp & SATP_MODE_SV32)) { // satp is inactive in M-mode
		*pa = va;
		return XLATE_OK;
	}

	uint32_t mstatus = cpu_read_csr(cpu, CSR_MSTATUS);
	uint64_t a = (uint64_t)(satp & SATP_PPN_MASK) << 12;
	uint32_t pte, pte_addr;
	int i;

	for (i = 1; ; i--) {
		uint64_t addr = a + (uint64_t)((va >> (12 + 10 * i)) & 0x3FF) * 4;
		if (addr > 0xFFFFFFFFu) { // no memory above 4 GiB
			return XLATE_ACCESS_FAULT;
		}
		pte_addr = (uint32_t)addr;
		if (!pmp_check(cpu, pte_addr, 4, PMP_ACCESS_READ, PRV_S) || // implicit accesses are S-mode
		    !mem_try_read32(mem, pte_addr, &pte)) {
			return XLATE_ACCESS_FAULT;
		}
		if (!(pte & PTE_V) || ((pte & PTE_W) && !(pte & PTE_R))) {
			return XLATE_PAGE_FAULT;
		}
		if (pte & (PTE_R | PTE_X)) { // leaf
			break;
		}
		if (i == 0) { // a pointer at the last level
			return XLATE_PAGE_FAULT;
		}
		a = (uint64_t)(pte >> 10) << 12;
	}

	uint32_t ppn = pte >> 10;
	if (i > 0 && (ppn & 0x3FF)) { // misaligned megapage
		return XLATE_PAGE_FAULT;
	}

	bool user_page = pte & PTE_U;
	if (priv == PRV_U && !user_page) {
		return XLATE_PAGE_FAULT;
	}
	if (priv == PRV_S && user_page && (type == ACCESS_FETCH || !(mstatus & MSTATUS_SUM))) {
		return XLATE_PAGE_FAULT; // S-mode never executes U pages, and loads/stores need SUM
	}

	bool allowed;
	switch (type) {
		case ACCESS_FETCH: allowed = pte & PTE_X; break;
		case ACCESS_STORE: allowed = pte & PTE_W; break;
		default:           allowed = (pte & PTE_R) || ((mstatus & MSTATUS_MXR) && (pte & PTE_X)); break;
	}
	if (!allowed) {
		return XLATE_PAGE_FAULT;
	}

	// Hardware A/D update. Single hart, so the read-modify-write is trivially atomic.
	uint32_t want = PTE_A | (type == ACCESS_STORE ? PTE_D : 0);
	if ((pte & want) != want) {
		if (!pmp_check(cpu, pte_addr, 4, PMP_ACCESS_WRITE, PRV_S) ||
		    !mem_try_write32(mem, pte_addr, pte | want)) {
			return XLATE_ACCESS_FAULT;
		}
	}

	// A megapage takes its low PPN bits from the virtual address
	uint32_t out_ppn = (i > 0) ? ((ppn & ~0x3FFu) | ((va >> 12) & 0x3FF)) : ppn;
	uint64_t out = ((uint64_t)out_ppn << 12) | (va & 0xFFF);
	if (out > 0xFFFFFFFFu) {
		return XLATE_ACCESS_FAULT;
	}
	*pa = (uint32_t)out;
	return XLATE_OK;
}
