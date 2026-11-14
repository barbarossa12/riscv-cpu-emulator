#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "mem.h"

// The core-local interruptor, in the SiFive CLINT layout that QEMU's virt machine and OpenSBI use, for
// one hart. The ISA manual only says these registers are memory-mapped (machine.adoc:1413, 2509); the
// offsets are the platform's choice. The offsets match the ACLINT specification's MSWI and MTIMER
// devices placed next to each other, so the same registers can later be described as ACLINT.
#define CLINT_BASE      0x02000000u
#define CLINT_SIZE      0x10000u
#define CLINT_MSIP      0x0000u   // 32 bits: bit 0 is MSIP, bits 31:1 read as zero
#define CLINT_MTIMECMP  0x4000u   // 64 bits, as two 32-bit halves (low half first)
#define CLINT_MTIME     0xBFF8u   // 64 bits, as two 32-bit halves (low half first)

// mtime is virtual, so a run is reproducible: it advances once per CLINT_CYCLES_PER_TICK cpu cycles and
// never reads the host clock. 10 MHz timebase on a nominal 100 MHz cpu. The device tree must say
// timebase-frequency = <10000000> (Milestone 9).
#define CLINT_TIMEBASE_HZ     10000000u
#define CLINT_CYCLES_PER_TICK 10u

typedef struct Clint {
	uint64_t mtime;
	uint64_t mtimecmp;
	uint32_t msip;
	uint32_t phase;   // cycles since mtime last ticked
} Clint;

// mtime = 0, mtimecmp = all ones (so no timer interrupt is pending until software sets one), msip = 0.
// The manual leaves these reset values unspecified (machine.adoc:2771-2790).
void clint_reset(Clint *c);
void clint_tick(Clint *c);                    // one cpu cycle
// Idle time passes (WFI). The cycles until mtime reaches mtimecmp, or CLINT_NEVER if no timer is armed (mtimecmp all ones)
// or it has been reached already. A far target saturates at CLINT_NEVER - 1, so it still counts as armed.
#define CLINT_NEVER UINT64_MAX
uint64_t clint_timer_horizon(const Clint *c);
// That many cpu cycles pass with no instruction executed: mtime and the tick phase move exactly as clint_tick would.
void clint_advance(Clint *c, uint64_t cycles);
// mtime jumps to mtimecmp and the tick phase restarts (the same as advancing by the horizon, without the cycle
// arithmetic, which overflows for a far target). Returns true if mtime moved.
bool clint_idle_until_timer(Clint *c);
bool clint_timer_pending(const Clint *c);     // MTIP: mtime >= mtimecmp, unsigned (machine.adoc:2521)
bool clint_software_pending(const Clint *c);  // MSIP
Device clint_device(Clint *c);                // for mem_add_device
