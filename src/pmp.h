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
