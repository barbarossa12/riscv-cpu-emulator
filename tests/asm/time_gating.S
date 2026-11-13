# time_gating.S
#-----------------------------------------------------------------------------
#
# The time CSR and its counter-enable gate, through the real emulator, from U-mode.
#
#   1. M-mode sets mcounteren.TM only (scounteren = 0) and returns to U-mode.
#   2. U-mode rdtime is an illegal instruction (supervisor.adoc:510). The handler sets scounteren.TM and
#      returns to the same instruction, which now works.
#   3. Two rdtime 100 steps apart differ by exactly 10 ticks, and rdtimeh is 0.
#   4. rdcycle is still illegal (CY is clear in both registers): the TM bit is independent. The handler skips it.
#
# s2 counts the illegal instructions the handler has seen.

#include "riscv_test.h"
#include "test_macros.h"

RVTEST_RV32M
RVTEST_CODE_BEGIN

  li s2, 0
  li TESTNUM, 2
  li t0, 2                                # COUNTEREN_TM
  csrw mcounteren, t0
  csrw scounteren, zero
  li t0, MSTATUS_MPP
  csrc mstatus, t0                        # MPP = U
  la t0, u_code
  csrw mepc, t0
  mret

  .align 2
u_code:
  li TESTNUM, 3
u_first:
  rdtime t1                               # traps once (scounteren.TM clear), then runs again
  li t4, 1
  bne s2, t4, fail

  li TESTNUM, 4
  rdtime t1
  .rept 99
  nop
  .endr
  rdtime t2
  sub t3, t2, t1
  li t4, 10
  bne t3, t4, fail                        # 100 steps: exactly 10 ticks
  rdtimeh t1
  bnez t1, fail

  li TESTNUM, 5
u_cycle:
  rdcycle t1                              # traps again: CY is clear
  li t4, 2
  bne s2, t4, fail

  RVTEST_PASS                             # an ecall from U-mode ends the test

  TEST_PASSFAIL

  .align 2
  .global mtvec_handler
mtvec_handler:
  csrr t0, mcause
  li t1, 2
  bne t0, t1, fail                        # only illegal instructions are expected
  csrr t0, mepc
  bnez s2, handle_second
  la t1, u_first
  bne t0, t1, fail                        # the first trap is the rdtime
  li t1, 2
  csrw scounteren, t1                     # now U-mode may read time; mepc stays: run the rdtime again
  addi s2, s2, 1
  mret
handle_second:
  la t1, u_cycle
  bne t0, t1, fail                        # the second trap is the rdcycle
  addi t0, t0, 4
  csrw mepc, t0                           # skip it
  addi s2, s2, 1
  mret

RVTEST_CODE_END

  .data
RVTEST_DATA_BEGIN

  TEST_DATA

RVTEST_DATA_END
