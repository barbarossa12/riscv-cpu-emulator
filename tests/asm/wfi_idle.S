# wfi_idle.S
#-----------------------------------------------------------------------------
#
# WFI lets idle time pass through the real emulator.
#
#   1. mtimecmp is 100000 ticks ahead, MTIE set, MIE clear: wfi returns after 3 cycles, but time has jumped to
#      the target, and mip.MTIP is set. cycle did not move: a sleeping hart executes no cycles.
#   2. With MIE set and the timer armed 500 ticks ahead, wfi jumps time, and the interrupt is taken on the
#      instruction after it: mepc is the address after the wfi (machine.adoc:2694).
#   3. With nothing armed (mtimecmp all ones), wfi returns at once and time barely moves.
#
# Registers: s0 = &mtimecmp, s1 = &mtime, s2 = interrupts handled, s3 = target, s4, s5, s6 = mtime, mepc, mcause
# in the handler, s7 s8 = cycle and time before the wfi, s9 s10 = after.

#include "riscv_test.h"
#include "test_macros.h"

#define MTIMECMP 0x02004000
#define MTIME    0x0200BFF8

.macro arm
  li t0, -1
  sw t0, 0(s0)
  sw zero, 4(s0)
  sw s3, 0(s0)
.endm

RVTEST_RV32M
RVTEST_CODE_BEGIN

  li s0, MTIMECMP
  li s1, MTIME
  li s2, 0

  # ---- 1. time jumps, cycles do not --------------------------------------------------------------------
  li TESTNUM, 2
  lw s3, 0(s1)
  li t0, 100000
  add s3, s3, t0
  arm
  li t0, MIP_MTIP
  csrs mie, t0
  csrr s7, cycle
  csrr s8, time
  wfi
  csrr s9, cycle
  csrr s10, time
  sub t0, s9, s7                          # cycles: the three instructions from the first csrr to the wfi
  li t1, 3
  bne t0, t1, fail
  sub t0, s10, s8                         # time: the whole 100000 ticks, less the few spent arming
  li t1, 99990
  bltu t0, t1, fail
  lw t0, 0(s1)
  bltu t0, s3, fail                       # mtime is at the target
  csrr t0, mip
  andi t0, t0, MIP_MTIP
  beqz t0, fail                           # MTIP is set, but MIE is clear: nothing was taken
  bnez s2, fail

  # ---- 2. the interrupt is taken after the wfi ---------------------------------------------------------
  li TESTNUM, 3
  li t0, -1
  sw t0, 0(s0)
  sw t0, 4(s0)                            # disarm: clears MTIP
  lw s3, 0(s1)
  addi s3, s3, 500
  arm
  li t0, MSTATUS_MIE
  csrs mstatus, t0
  wfi
after_wfi:
  nop
  li t1, 1
  bne s2, t1, fail
  la t0, after_wfi
  bne s5, t0, fail                        # mepc = pc of the wfi + 4
  li t0, 0x80000007
  bne s6, t0, fail
  bltu s4, s3, fail                       # the jump took time to the target

  # ---- 3. nothing armed: wfi does not wait --------------------------------------------------------------
  li TESTNUM, 4
  li t0, MSTATUS_MIE
  csrc mstatus, t0
  csrr s8, time
  wfi
  csrr s10, time
  sub t0, s10, s8
  li t1, 2
  bgtu t0, t1, fail                       # a couple of instructions: at most a tick or so

  TEST_PASSFAIL

  .align 2
  .global mtvec_handler
mtvec_handler:
  csrr s6, mcause
  csrr s5, mepc
  lw s4, 0(s1)
  li t0, -1
  sw t0, 0(s0)
  sw t0, 4(s0)
  addi s2, s2, 1
  mret

RVTEST_CODE_END

  .data
RVTEST_DATA_BEGIN

  TEST_DATA

RVTEST_DATA_END
