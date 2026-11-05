#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "cpu.h"

// pmpNcfg byte
#define PMP_R   (1u << 0)
#define PMP_W   (1u << 1)
#define PMP_X   (1u << 2)
#define PMP_RWX (PMP_R | PMP_W | PMP_X)
#define PMP_A_SHIFT 3
#define PMP_A_MASK  (3u << PMP_A_SHIFT)
#define PMP_L   (1u << 7)
#define PMP_CFG_IMPLEMENTED (PMP_RWX | PMP_A_MASK | PMP_L)   // 0x9F: bits 6:5 read as 0

// The A field: address-matching mode
#define PMP_A_OFF   0u
#define PMP_A_TOR   1u
#define PMP_A_NA4   2u
#define PMP_A_NAPOT 3u

bool pmp_csr_exists(uint16_t csr_addr);                          // pmpcfg0-15, pmpaddr0-63
uint32_t pmp_csr_read(const CPU *cpu, uint16_t csr_addr);
void pmp_csr_write(CPU *cpu, uint16_t csr_addr, uint32_t value);

// A PMP region as a half-open byte range [lo, hi). 64-bit, because a NAPOT region can span 2^35 bytes.
typedef struct {
    uint64_t lo;
    uint64_t hi;
} PmpRange;

// The range an entry matches, from its config byte, its pmpaddr and the previous entry's pmpaddr
// (only TOR uses that one). Returns false when the entry matches nothing: OFF, or an empty TOR.
bool pmp_range(uint8_t cfg, uint32_t addr, uint32_t prev_addr, PmpRange *out);

// The same for entry i of a CPU, with prev_addr = pmpaddr[i-1] (0 for entry 0).
// Returns false when it matches nothing, and for i >= PMP_ENTRIES.
bool pmp_entry_range(const CPU *cpu, unsigned i, PmpRange *out);
