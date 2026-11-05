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
