#include "mmu.h"
#include "pmp.h"

#define SATP_MODE_SV32 (1u << 31)
#define SATP_PPN_MASK  0x3FFFFFu
#define SATP_ASID_SHIFT 22
#define ASID_MASK      0x1FFu   // ASIDMAX is 9 for Sv32

#define PTE_V (1u << 0)
#define PTE_R (1u << 1)
#define PTE_W (1u << 2)
#define PTE_X (1u << 3)
#define PTE_U (1u << 4)
#define PTE_G (1u << 5)
#define PTE_A (1u << 6)
#define PTE_D (1u << 7)

bool mmu_enabled(const CPU *cpu, PrivMode priv) {
	return priv != PRV_M && (cpu->csr[CSR_SATP] & SATP_MODE_SV32);
}

/* ---- The translation cache -------------------------------------------------------------------
 * It holds leaf PTEs the walk has read (and, for a store or a first access, updated), keyed by
 * virtual page and ASID. Two direct-mapped arrays: 4 KiB pages by VPN, megapages by VPN1.
 * An entry keeps only the raw PTE: the permission checks run again on every hit, so SUM, MXR and
 * the privilege take effect immediately. PMP is never cached either. */

static bool tlb_match(const TlbEntry *e, uint32_t vpn, uint32_t asid) {
	return e->valid && e->vpn == vpn && ((e->pte & PTE_G) || e->asid == asid);   // global: every ASID
}

// The entry for va, or NULL. *level is 0 for a 4 KiB page and 1 for a megapage.
static const TlbEntry *tlb_lookup(const CPU *cpu, uint32_t va, uint32_t asid, int *level) {
	uint32_t vpn = va >> 12;
	const TlbEntry *e = &cpu->tlb_4k[vpn % TLB_4K_ENTRIES];
	if (tlb_match(e, vpn, asid)) {
		*level = 0;
		return e;
	}
	uint32_t vpn1 = va >> 22;
	e = &cpu->tlb_mega[vpn1 % TLB_MEGA_ENTRIES];
	if (tlb_match(e, vpn1, asid)) {
		*level = 1;
		return e;
	}
	return NULL;
}

static void tlb_insert(CPU *cpu, uint32_t va, uint32_t asid, uint32_t pte, int level) {
	uint32_t vpn = level ? va >> 22 : va >> 12;
	TlbEntry *e = level ? &cpu->tlb_mega[vpn % TLB_MEGA_ENTRIES] : &cpu->tlb_4k[vpn % TLB_4K_ENTRIES];
	*e = (TlbEntry){ .valid = true, .asid = (uint16_t)asid, .vpn = vpn, .pte = pte };
}

void mmu_flush(CPU *cpu, uint32_t va, uint32_t asid, bool all_addresses, bool all_asids) {
	for (int level = 0; level < 2; level++) {
		TlbEntry *entries = level ? cpu->tlb_mega : cpu->tlb_4k;
		int count = level ? TLB_MEGA_ENTRIES : TLB_4K_ENTRIES;
		uint32_t vpn = level ? va >> 22 : va >> 12;
		for (int i = 0; i < count; i++) {
			TlbEntry *e = &entries[i];
			bool address_ok = all_addresses || e->vpn == vpn;
			bool asid_ok = all_asids || (!(e->pte & PTE_G) && e->asid == asid);   // global entries survive an ASID fence
			if (e->valid && address_ok && asid_ok) {
				e->valid = false;
			}
		}
	}
}

/* ---- Translation -------------------------------------------------------------------------- */

// Steps 6 to 8 of the walk: is the access allowed by the leaf's U, R, W and X bits?
static bool leaf_allows(uint32_t mstatus, uint32_t pte, AccessType type, PrivMode priv) {
	bool user_page = pte & PTE_U;
	if (priv == PRV_U && !user_page) {
		return false;
	}
	if (priv == PRV_S && user_page && (type == ACCESS_FETCH || !(mstatus & MSTATUS_SUM))) {
		return false; // S-mode never executes U pages, and loads/stores need SUM
	}
	switch (type) {
		case ACCESS_FETCH: return pte & PTE_X;
		case ACCESS_STORE: return pte & PTE_W;
		default:           return (pte & PTE_R) || ((mstatus & MSTATUS_MXR) && (pte & PTE_X));
	}
}

// Step 11: the physical address of va through a leaf at the given level. False if it is above 4 GiB.
static bool leaf_pa(uint32_t pte, int level, uint32_t va, uint32_t *pa) {
	uint32_t ppn = pte >> 10;
	// A megapage takes its low PPN bits from the virtual address
	uint32_t out_ppn = level ? ((ppn & ~0x3FFu) | ((va >> 12) & 0x3FF)) : ppn;
	uint64_t out = ((uint64_t)out_ppn << 12) | (va & 0xFFF);
	if (out > 0xFFFFFFFFu) {
		return false;
	}
	*pa = (uint32_t)out;
	return true;
}

XlateResult mmu_translate(CPU *cpu, Memory *mem, uint32_t va, AccessType type,
                          PrivMode priv, uint32_t *pa) {
	uint32_t satp = cpu_read_csr(cpu, CSR_SATP);
	if (priv == PRV_M || !(satp & SATP_MODE_SV32)) { // satp is inactive in M-mode
		*pa = va;
		return XLATE_OK;
	}

	uint32_t mstatus = cpu_read_csr(cpu, CSR_MSTATUS);
	uint32_t asid = (satp >> SATP_ASID_SHIFT) & ASID_MASK;
	uint32_t want = PTE_A | (type == ACCESS_STORE ? PTE_D : 0);   // what the PTE must have, else the walk updates it

	int level;
	const TlbEntry *hit = tlb_lookup(cpu, va, asid, &level);
	if (hit) {
		uint32_t cached = hit->pte;
		if (!leaf_allows(mstatus, cached, type, priv)) {
			return XLATE_PAGE_FAULT;
		}
		if ((cached & want) == want) {
			return leaf_pa(cached, level, va, pa) ? XLATE_OK : XLATE_ACCESS_FAULT;
		}
		// A or D must change, and that update can only be made in memory: walk
	}

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

	level = i;
	if (level > 0 && ((pte >> 10) & 0x3FF)) { // misaligned megapage
		return XLATE_PAGE_FAULT;
	}
	if (!leaf_allows(mstatus, pte, type, priv)) {
		return XLATE_PAGE_FAULT;
	}

	// Hardware A/D update. Single hart, so the read-modify-write is trivially atomic.
	if ((pte & want) != want) {
		if (!pmp_check(cpu, pte_addr, 4, PMP_ACCESS_WRITE, PRV_S) ||
		    !mem_try_write32(mem, pte_addr, pte | want)) {
			return XLATE_ACCESS_FAULT;
		}
		pte |= want;
	}

	if (!leaf_pa(pte, level, va, pa)) {
		return XLATE_ACCESS_FAULT;
	}
	tlb_insert(cpu, va, asid, pte, level);
	return XLATE_OK;
}
