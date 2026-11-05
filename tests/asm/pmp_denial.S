# pmp_denial.S
#-----------------------------------------------------------------------------
#
# PMP denial, end to end: in U-mode, a load, a store and an instruction fetch that hit a 4-byte
# region with no permissions must trap with the right cause and mtval, while a neighbouring word
# stays accessible. M-mode is not restricted by the region (its L bit is clear).
#
# The harness leaves PMP entry 0 as an allow-all NAPOT region. Entries are matched lowest first,
# so this test moves the allow-all region to entry 1 and puts the no-permission region in entry 0.

#include "riscv_test.h"
#include "test_macros.h"

RVTEST_RV32M
RVTEST_CODE_BEGIN

  # ---- M-mode: set up PMP --------------------------------------------------
  li t0, 0x7FFFFFFF
  csrw pmpaddr1, t0                       # entry 1: NAPOT over the whole space
  la t0, forbidden
  srli t0, t0, PMP_SHIFT
  csrw pmpaddr0, t0                       # entry 0: the 4 bytes at `forbidden`
  li t0, ((PMP_NAPOT | PMP_R | PMP_W | PMP_X) << 8) | PMP_NA4
  csrw pmpcfg0, t0                        # entry 0: NA4, no permissions. entry 1: NAPOT, RWX.

  # M-mode ignores a region whose L bit is clear, so this load must not trap.
  li TESTNUM, 2
  la s0, forbidden
  lw t1, 0(s0)

  # ---- drop to U-mode ------------------------------------------------------
  li s2, 0                                # the handler counts the faults it handles in s2
  la t0, user_code
  csrw mepc, t0
  li t0, MSTATUS_MPP
  csrc mstatus, t0                        # MPP = U
  mret

  .align 2
user_code:
  # load from the forbidden word: load access fault (cause 5), mtval = its address
  li TESTNUM, 3
  la s0, forbidden
  lw t1, 0(s0)
  li t4, 1
  bne s2, t4, fail                        # the load must have trapped, once

  # store to it: store access fault (cause 7)
  li TESTNUM, 4
  sw t1, 0(s0)
  li t4, 2
  bne s2, t4, fail

  # the neighbouring word is allowed by entry 1
  li TESTNUM, 5
  la s1, allowed
  li t2, 0x1234
  sw t2, 0(s1)
  lw t3, 0(s1)
  bne t2, t3, fail

  # jump to the forbidden word: instruction access fault (cause 1), mtval = mepc = its address
  li TESTNUM, 6
  jalr ra, 0(s0)
after_fetch:
  li t4, 3
  bne s2, t4, fail                        # and so must the fetch

  TEST_PASSFAIL

  # ---- M-mode trap handler -------------------------------------------------
  .align 2
  .global mtvec_handler
mtvec_handler:
  csrr t0, mcause
  csrr t1, mtval
  csrr t2, mepc
  la t3, forbidden

  li t4, 5
  beq t0, t4, expect_load_fault
  li t4, 7
  beq t0, t4, expect_store_fault
  li t4, 1
  beq t0, t4, expect_fetch_fault
  j fail                                  # any other trap is a failure

expect_load_fault:
  li t4, 3
  bne TESTNUM, t4, fail
  bne t1, t3, fail
  j skip_instruction

expect_store_fault:
  li t4, 4
  bne TESTNUM, t4, fail
  bne t1, t3, fail
  j skip_instruction

expect_fetch_fault:
  li t4, 6
  bne TESTNUM, t4, fail
  bne t1, t3, fail                        # mtval = the address we jumped to
  bne t2, t3, fail                        # mepc = the same address
  addi s2, s2, 1
  la t4, after_fetch
  csrw mepc, t4                           # carry on after the jalr
  mret

skip_instruction:
  addi s2, s2, 1
  addi t2, t2, 4
  csrw mepc, t2
  mret

RVTEST_CODE_END

  .data
RVTEST_DATA_BEGIN

  TEST_DATA

  .align 4
forbidden: .word 0xdeadbeef
allowed:   .word 0

RVTEST_DATA_END
