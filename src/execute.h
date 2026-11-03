#pragma once
#include "decode.h"
#include "cpu.h"
#include "mem.h"

void execute(CPU *cpu, Memory *mem, DecodedInstruction *instr);

// Fetch, decode and execute one instruction. A fetch outside memory raises an
// instruction access fault (cause 1, mtval = pc) instead of aborting.
void cpu_step(CPU *cpu, Memory *mem);
