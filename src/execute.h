#pragma once
#include "decode.h"
#include "cpu.h"
#include "mem.h"

void execute(CPU *cpu, Memory *mem, DecodedInstruction *instr);
