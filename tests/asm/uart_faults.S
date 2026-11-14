# uart_faults.S
#-----------------------------------------------------------------------------
#
# The UART's strict bus rules through the real CPU: only byte accesses to offsets 0 to 7 are allowed, every other access
# to the device faults with the access-fault cause for its kind and mtval = the address, and fetching from it faults too.
# The handler checks the expected cause (s3) and address (s4) and counts the faults in s2.

#include "riscv_test.h"
#include "test_macros.h"
#include "uart_common.h"

RVTEST_RV32M
RVTEST_CODE_BEGIN

  li s2, 0
  li s0, UART

  # a word load: load access fault (5)
  li TESTNUM, 2
  li s3, 5
  mv s4, s0
  lw t1, 0(s0)
  li t4, 1
  bne s2, t4, fail

  # a word store: store access fault (7)
  li TESTNUM, 3
  li s3, 7
  sw t1, 0(s0)
  li t4, 2
  bne s2, t4, fail

  # a halfword load: also cause 5
  li TESTNUM, 4
  li s3, 5
  lh t1, 0(s0)
  li t4, 3
  bne s2, t4, fail

  # a byte load past the last register (offset 8)
  li TESTNUM, 5
  li s3, 5
  addi s4, s0, 8
  lb t1, 8(s0)
  li t4, 4
  bne s2, t4, fail

  # a byte store to the same place
  li TESTNUM, 6
  li s3, 7
  sb t1, 8(s0)
  li t4, 5
  bne s2, t4, fail

  # the far end of the 256-byte window faults the same way
  li TESTNUM, 7
  li s3, 5
  addi s4, s0, 0xFF
  lb t1, 0xFF(s0)
  li t4, 6
  bne s2, t4, fail

  # byte accesses to every register 0 to 7 are fine (no trap, the count stays). DLAB is set first, so the stores
  # to offsets 0 and 1 reach the divisor latches and no stray byte is transmitted.
  li TESTNUM, 8
  WR UART_LCR, 0x80
  li t4, 0
1:
  add t5, s0, t4
  lbu t1, 0(t5)
  sb zero, 0(t5)
  addi t4, t4, 1
  li t6, 8
  bne t4, t6, 1b
  li t4, 6
  bne s2, t4, fail

  # fetching from the device: instruction access fault (1), mtval = mepc = the address
  li TESTNUM, 9
  li s3, 1
  mv s4, s0
  jalr ra, 0(s0)
  li t4, 7
  bne s2, t4, fail

  TEST_PASSFAIL

  .align 2
  .global mtvec_handler
mtvec_handler:
  csrr t0, mcause
  bne t0, s3, fail
  csrr t0, mtval
  bne t0, s4, fail
  addi s2, s2, 1
  li t0, 1
  beq s3, t0, fetch_fault
  csrr t0, mepc
  addi t0, t0, 4                         # skip the faulting load or store
  csrw mepc, t0
  mret
fetch_fault:
  csrr t0, mepc
  bne t0, s4, fail                       # mepc = the address we jumped to
  csrw mepc, ra                          # carry on after the jalr
  mret

RVTEST_CODE_END

  .data
RVTEST_DATA_BEGIN

  TEST_DATA

RVTEST_DATA_END
