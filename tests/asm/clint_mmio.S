# clint_mmio.S
#-----------------------------------------------------------------------------
#
# The CLINT through the real emulator (main.c registers it): reset values, and mtime counting cycles
# deterministically. mtime ticks once per 10 cpu cycles, one cycle per instruction step, so between two
# loads of mtime that are a multiple of 10 steps apart it advances by exactly steps / 10, whatever the
# phase was at the first load.

#include "riscv_test.h"
#include "test_macros.h"

#define CLINT_MTIMECMP 0x02004000
#define CLINT_MTIME    0x0200BFF8
#define CLINT_MSIP     0x02000000

RVTEST_RV32M
RVTEST_CODE_BEGIN

  # reset values: mtimecmp is all ones (no timer pending), msip is 0
  li TESTNUM, 2
  li t0, CLINT_MTIMECMP
  lw t1, 0(t0)
  li t2, -1
  bne t1, t2, fail
  lw t1, 4(t0)
  bne t1, t2, fail
  li t0, CLINT_MSIP
  lw t1, 0(t0)
  bnez t1, fail

  # msip holds only bit 0
  li TESTNUM, 3
  li t2, 0xFFFFFFFE
  sw t2, 0(t0)
  lw t1, 0(t0)
  bnez t1, fail
  li t2, 1
  sw t2, 0(t0)
  lw t1, 0(t0)
  bne t1, t2, fail
  sw zero, 0(t0)

  # 100 steps apart: exactly 10 ticks. The window is the lw, 99 nops, and ends at the next lw.
  li TESTNUM, 4
  li t0, CLINT_MTIME
  lw t1, 0(t0)
  .rept 99
  nop
  .endr
  lw t2, 0(t0)
  sub t3, t2, t1
  li t4, 10
  bne t3, t4, fail

  # 500 steps apart through a loop: the lw, li, 249 iterations of two instructions = 500 steps, 50 ticks
  li TESTNUM, 5
  lw t1, 0(t0)
  li t5, 249
1:
  addi t5, t5, -1
  bnez t5, 1b
  lw t2, 0(t0)
  sub t3, t2, t1
  li t4, 50
  bne t3, t4, fail

  # the high half has not moved
  li TESTNUM, 6
  lw t1, 4(t0)
  bnez t1, fail

  TEST_PASSFAIL

RVTEST_CODE_END

  .data
RVTEST_DATA_BEGIN

  TEST_DATA

RVTEST_DATA_END
