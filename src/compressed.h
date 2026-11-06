#pragma once
#include <stdint.h>
#include <stdbool.h>

// Expand a 16-bit compressed instruction (Zca, RV32) to the 32-bit instruction it stands for.
// Returns false, leaving *word untouched, if `half` is not a legal compressed instruction: the
// reserved code points, the floating-point and RV64-only forms, and anything with bits [1:0] = 11
// (which is the start of a 32-bit or longer instruction, not a compressed one).
bool expand_compressed(uint16_t half, uint32_t *word);
