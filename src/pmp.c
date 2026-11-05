#include "pmp.h"

#define PMP_CFG_CSRS  16   // pmpcfg0..pmpcfg15
#define PMP_ADDR_CSRS 64   // pmpaddr0..pmpaddr63

static bool is_cfg_csr(uint16_t a)  { return a >= CSR_PMPCFG0  && a < CSR_PMPCFG0  + PMP_CFG_CSRS; }
static bool is_addr_csr(uint16_t a) { return a >= CSR_PMPADDR0 && a < CSR_PMPADDR0 + PMP_ADDR_CSRS; }

bool pmp_csr_exists(uint16_t csr_addr) {
    return is_cfg_csr(csr_addr) || is_addr_csr(csr_addr);
}

uint32_t pmp_csr_read(const CPU *cpu, uint16_t csr_addr) {
    if (is_cfg_csr(csr_addr)) {
        unsigned first = (csr_addr - CSR_PMPCFG0) * 4;       // four config bytes per CSR, little-endian
        uint32_t value = 0;
        for (unsigned j = 0; j < 4; j++) {
            if (first + j < PMP_ENTRIES) {
                value |= (uint32_t)cpu->pmpcfg[first + j] << (8 * j);
            }                                                 // entries 16..63: read-only 0
        }
        return value;
    }
    unsigned n = csr_addr - CSR_PMPADDR0;
    return n < PMP_ENTRIES ? cpu->pmpaddr[n] : 0;
}

// One config byte. A locked entry ignores the write. R, W and X are one WARL field: the
// reserved value R=0, W=1 leaves RWX as it was, while A and L still take the written values.
static void write_cfg_byte(CPU *cpu, unsigned i, uint8_t byte) {
    uint8_t old = cpu->pmpcfg[i];
    if (old & PMP_L) {
        return;
    }
    uint8_t value = byte & PMP_CFG_IMPLEMENTED;
    if ((value & PMP_W) && !(value & PMP_R)) {
        value = (value & ~PMP_RWX) | (old & PMP_RWX);
    }
    cpu->pmpcfg[i] = value;
}

// A locked entry ignores writes to its pmpaddr, and so does the entry below a locked TOR entry.
static void write_addr(CPU *cpu, unsigned n, uint32_t value) {
    if (n >= PMP_ENTRIES || (cpu->pmpcfg[n] & PMP_L)) {
        return;
    }
    if (n + 1 < PMP_ENTRIES && (cpu->pmpcfg[n + 1] & PMP_L) &&
        ((cpu->pmpcfg[n + 1] & PMP_A_MASK) >> PMP_A_SHIFT) == PMP_A_TOR) {
        return;
    }
    cpu->pmpaddr[n] = value;
}

void pmp_csr_write(CPU *cpu, uint16_t csr_addr, uint32_t value) {
    if (is_cfg_csr(csr_addr)) {
        unsigned first = (csr_addr - CSR_PMPCFG0) * 4;
        for (unsigned j = 0; j < 4 && first + j < PMP_ENTRIES; j++) {
            write_cfg_byte(cpu, first + j, (uint8_t)(value >> (8 * j)));
        }
        return;
    }
    write_addr(cpu, csr_addr - CSR_PMPADDR0, value);
}

bool pmp_range(uint8_t cfg, uint32_t addr, uint32_t prev_addr, PmpRange *out) {
    switch ((cfg & PMP_A_MASK) >> PMP_A_SHIFT) {
        case PMP_A_TOR:
            out->lo = (uint64_t)prev_addr << 2;
            out->hi = (uint64_t)addr << 2;
            return out->lo < out->hi;                 // lo >= hi matches nothing
        case PMP_A_NA4:
            out->lo = (uint64_t)addr << 2;
            out->hi = out->lo + 4;
            return true;
        case PMP_A_NAPOT: {
            // t trailing ones select a region of 2^(t+3) bytes. The ones and the zero above them
            // are the size bits; the bits above those are the base.
            unsigned t = 0;
            while (t < 32 && ((addr >> t) & 1u)) {
                t++;
            }
            uint64_t base_words = (uint64_t)addr & ~((1ull << (t + 1)) - 1);
            out->lo = base_words << 2;
            out->hi = out->lo + (1ull << (t + 3));
            return true;
        }
        default:                                       // PMP_A_OFF
            return false;
    }
}

bool pmp_entry_range(const CPU *cpu, unsigned i, PmpRange *out) {
    if (i >= PMP_ENTRIES) {
        return false;
    }
    uint32_t prev = i == 0 ? 0 : cpu->pmpaddr[i - 1];   // the previous entry's config is irrelevant
    return pmp_range(cpu->pmpcfg[i], cpu->pmpaddr[i], prev, out);
}

bool pmp_check(const CPU *cpu, uint32_t addr, uint32_t size, PmpAccess access, PrivMode mode) {
    uint64_t lo = addr;
    uint64_t hi = (uint64_t)addr + size;            // 64-bit: the physical space is 34 bits wide

    for (unsigned i = 0; i < PMP_ENTRIES; i++) {
        PmpRange r;
        if (!pmp_entry_range(cpu, i, &r)) {
            continue;                                // OFF, or an empty TOR
        }
        if (hi <= r.lo || lo >= r.hi) {
            continue;                                // no byte in common
        }
        // The lowest-numbered entry that matches any byte decides, and it must match all of them.
        if (lo < r.lo || hi > r.hi) {
            return false;
        }
        uint8_t cfg = cpu->pmpcfg[i];
        if (mode == PRV_M && !(cfg & PMP_L)) {
            return true;                             // L clear: M-mode ignores the permissions
        }
        switch (access) {
            case PMP_ACCESS_READ:  return (cfg & PMP_R) != 0;
            case PMP_ACCESS_WRITE: return (cfg & PMP_W) != 0;
            default:               return (cfg & PMP_X) != 0;
        }
    }
    // No entry matched. At least one entry is implemented, so S and U fail; M-mode succeeds.
    return mode == PRV_M;
}
