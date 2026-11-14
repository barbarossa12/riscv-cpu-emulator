# plic_mmio.S
#-----------------------------------------------------------------------------
#
# The PLIC through the real emulator, from M-mode with interrupts off: the UART's THRE interrupt (source 10) is the one source
# there is, and mip shows the PLIC's outputs (MEIP for context 0, SEIP for context 1) without taking anything.
#
#   1. Reset values read 0, then the field masks: priority and threshold keep 3 bits, enable bit 0 does not exist, pending is
#      read-only.
#   2. The bus rules through the real CPU: only aligned 32-bit accesses to registers that exist; every other access faults with
#      the cause for its kind and mtval = the address, a fetch included. Handler as in uart_faults.S (s2 counts, s3 = cause,
#      s4 = address, s5 = faults expected so far).
#   3. The gateway: a raised source is pending and MEIP, a claim returns it and clears the pending bit, nothing more while it is
#      in flight, a complete with the line still high forwards a new request, a complete once the cause is gone does not.
#   4. Context 1 drives SEIP, independently of context 0 (the claim is shared: one request, one claim).
#   5. Every priority 0 to 7 against every threshold 0 to 7: MEIP is set exactly when priority > threshold.
#   6. Priority 0 never claims (the request stays pending); a claim ignores the threshold.
#   7. A complete only counts for a source enabled in the completing context, and ids 0, 32 and 0xFFFFFFFF do nothing.

#include "riscv_test.h"
#include "test_macros.h"
#include "uart_common.h"
#include "plic_common.h"

  .macro LOADFAULT insn, addr            # one more load access fault (5) at addr
  li s3, 5
  li s4, \addr
  \insn t1, 0(s4)
  addi s5, s5, 1
  bne s2, s5, fail
  .endm

  .macro STOREFAULT insn, addr           # one more store access fault (7) at addr
  li s3, 7
  li s4, \addr
  \insn t1, 0(s4)
  addi s5, s5, 1
  bne s2, s5, fail
  .endm

RVTEST_RV32M
RVTEST_CODE_BEGIN

  li s2, 0
  li s5, 0

  # ---- 1. reset values and field masks ---------------------------------------------------------------------------------
  li TESTNUM, 2
  PEXPECT PLIC_PRIORITY(1), 0
  PEXPECT PLIC_PRIORITY(10), 0
  PEXPECT PLIC_PRIORITY(31), 0
  PEXPECT PLIC_PENDING, 0
  PEXPECT PLIC_ENABLE(0), 0
  PEXPECT PLIC_ENABLE(1), 0
  PEXPECT PLIC_THRESHOLD(0), 0
  PEXPECT PLIC_THRESHOLD(1), 0
  PEXPECT PLIC_CLAIM(0), 0
  PEXPECT PLIC_CLAIM(1), 0

  li TESTNUM, 3
  PW PLIC_PRIORITY(10), 0xFFFFFFFF
  PEXPECT PLIC_PRIORITY(10), 7
  PW PLIC_PRIORITY(31), 9
  PEXPECT PLIC_PRIORITY(31), 1
  PEXPECT PLIC_PRIORITY(1), 0              # the neighbours did not move
  PW PLIC_ENABLE(0), 0xFFFFFFFF
  PEXPECT PLIC_ENABLE(0), 0xFFFFFFFE       # source 0 does not exist
  PEXPECT PLIC_ENABLE(1), 0
  PW PLIC_ENABLE(1), 0xFFFFFFFF
  PEXPECT PLIC_ENABLE(1), 0xFFFFFFFE
  PW PLIC_THRESHOLD(0), 0xFFFFFFFF
  PEXPECT PLIC_THRESHOLD(0), 7
  PW PLIC_THRESHOLD(1), 9
  PEXPECT PLIC_THRESHOLD(1), 1
  PW PLIC_PENDING, 0xFFFFFFFF
  PEXPECT PLIC_PENDING, 0                  # read-only
  PW PLIC_PRIORITY(10), 0
  PW PLIC_PRIORITY(31), 0
  PW PLIC_ENABLE(0), 0
  PW PLIC_ENABLE(1), 0
  PW PLIC_THRESHOLD(0), 0
  PW PLIC_THRESHOLD(1), 0

  # ---- 2. the bus rules ---------------------------------------------------------------------------------------------------
  li TESTNUM, 4
  LOADFAULT lb, PLIC_PRIORITY(1)           # narrow accesses
  LOADFAULT lbu, PLIC_PENDING
  LOADFAULT lh, PLIC_PRIORITY(1)
  LOADFAULT lhu, PLIC_THRESHOLD(0)
  STOREFAULT sb, PLIC_PRIORITY(1)
  STOREFAULT sh, PLIC_ENABLE(0)
  LOADFAULT lw, PLIC+2                     # misaligned
  LOADFAULT lw, PLIC_PRIORITY(1)+2
  STOREFAULT sw, PLIC_THRESHOLD(0)+1

  li TESTNUM, 5
  LOADFAULT lw, PLIC                       # source 0 does not exist
  STOREFAULT sw, PLIC
  LOADFAULT lw, PLIC_PRIORITY(32)          # one past source 31
  STOREFAULT sw, PLIC_PRIORITY(32)
  LOADFAULT lw, PLIC_PENDING+4             # gaps between the blocks
  STOREFAULT sw, PLIC_PENDING+4
  LOADFAULT lw, PLIC_PENDING-4
  LOADFAULT lw, PLIC_ENABLE(0)+4           # enable word 1
  STOREFAULT sw, PLIC_ENABLE(1)+4
  LOADFAULT lw, PLIC_ENABLE(2)             # context 2
  STOREFAULT sw, PLIC_ENABLE(2)
  LOADFAULT lw, PLIC_THRESHOLD(0)+8        # between the threshold/claim pair and the next context
  LOADFAULT lw, PLIC_THRESHOLD(1)+0xFFC
  STOREFAULT sw, PLIC_THRESHOLD(1)+8
  LOADFAULT lw, PLIC_THRESHOLD(2)
  STOREFAULT sw, PLIC_CLAIM(2)
  LOADFAULT lw, PLIC_THRESHOLD(0)-4        # just below the context block
  LOADFAULT lw, PLIC+0x3FFFFFC             # the last word of the region

  li TESTNUM, 6                            # a word access to every register that exists is fine: no new fault
  li t4, PLIC_PRIORITY(1)
  lw t1, 0(t4)
  sw t1, 0(t4)
  li t4, PLIC_THRESHOLD(1)
  lw t1, 0(t4)
  sw t1, 0(t4)
  bne s2, s5, fail

  li TESTNUM, 7                            # fetching from the device: instruction access fault (1), mtval = mepc = the address
  li s3, 1
  li s4, PLIC
  addi s5, s5, 1
  jalr ra, 0(s4)
  bne s2, s5, fail

  # ---- 3. the gateway ---------------------------------------------------------------------------------------------------
  li TESTNUM, 8
  PW PLIC_PRIORITY(10), 1
  PW PLIC_ENABLE(0), 0x400                 # source 10, context 0
  WR UART_IER, 0x02                        # ETBEI: the THRE interrupt is raised
  PEXPECT PLIC_PENDING, 0x400
  MIPEXPECT MEIP, MEIP
  MIPEXPECT SEIP, 0                        # context 1 has not enabled it

  li TESTNUM, 9
  PEXPECT PLIC_CLAIM(0), 10
  PEXPECT PLIC_PENDING, 0                  # a claim clears the pending bit
  MIPEXPECT MEIP, 0
  PEXPECT PLIC_CLAIM(0), 0                 # and the source is in flight: nothing more, although the line is still high
  PEXPECT PLIC_PENDING, 0

  li TESTNUM, 10
  PW PLIC_CLAIM(0), 10                     # complete with the line still high: a new request
  PEXPECT PLIC_PENDING, 0x400
  PEXPECT PLIC_CLAIM(0), 10
  RD t2, UART_IIR                          # reading IIR clears the THRE cause: the line drops
  PW PLIC_CLAIM(0), 10                     # complete: no new request
  PEXPECT PLIC_PENDING, 0
  PEXPECT PLIC_CLAIM(0), 0
  WR UART_IER, 0x00
  WR UART_IER, 0x02                        # a new rising edge is a new request
  PEXPECT PLIC_PENDING, 0x400

  # ---- 4. context 1 ------------------------------------------------------------------------------------------------------
  li TESTNUM, 11
  MIPEXPECT SEIP, 0
  PW PLIC_ENABLE(1), 0x400
  MIPEXPECT SEIP, SEIP
  MIPEXPECT MEIP, MEIP                     # both contexts see the one pending request
  PEXPECT PLIC_CLAIM(1), 10                # whichever claims takes it
  PEXPECT PLIC_CLAIM(0), 0
  MIPEXPECT MEIP, 0
  MIPEXPECT SEIP, 0
  RD t2, UART_IIR
  PW PLIC_CLAIM(1), 10                     # context 1 completes it
  PEXPECT PLIC_PENDING, 0
  PW PLIC_ENABLE(1), 0

  # ---- 5. priority against threshold ---------------------------------------------------------------------------------------
  li TESTNUM, 12
  WR UART_IER, 0x00
  WR UART_IER, 0x02
  li s7, 0                                 # priority
1:
  li s8, 0                                 # threshold
2:
  li t0, PLIC_PRIORITY(10)
  sw s7, 0(t0)
  li t0, PLIC_THRESHOLD(0)
  sw s8, 0(t0)
  csrr t2, mip
  li t3, MEIP
  and t2, t2, t3
  li t4, 0
  ble s7, s8, 3f
  li t4, MEIP                              # priority > threshold: the interrupt is signalled
3:
  bne t2, t4, fail
  addi s8, s8, 1
  li t0, 8
  bne s8, t0, 2b
  addi s7, s7, 1
  bne s7, t0, 1b

  # ---- 6. priority 0, and the threshold does not stop a claim --------------------------------------------------------------
  li TESTNUM, 13
  PW PLIC_PRIORITY(10), 0
  PW PLIC_THRESHOLD(0), 0
  PEXPECT PLIC_CLAIM(0), 0                 # priority 0 means never
  PEXPECT PLIC_PENDING, 0x400              # still pending
  PW PLIC_PRIORITY(10), 2
  PW PLIC_THRESHOLD(0), 7
  MIPEXPECT MEIP, 0                        # masked by the threshold
  PEXPECT PLIC_CLAIM(0), 10                # but a claim works
  PW PLIC_THRESHOLD(0), 0

  # ---- 7. complete rules ----------------------------------------------------------------------------------------------
  li TESTNUM, 14
  PW PLIC_CLAIM(1), 10                     # context 1 has not enabled source 10: ignored
  PEXPECT PLIC_PENDING, 0                  # still in flight (the line is high, so a re-arm would show here)
  PW PLIC_CLAIM(0), 0                      # ids 0, 32 and 0xFFFFFFFF are no source
  PW PLIC_CLAIM(0), 32
  PW PLIC_CLAIM(0), 0xFFFFFFFF
  PEXPECT PLIC_PENDING, 0
  PW PLIC_CLAIM(0), 10                     # the real one
  PEXPECT PLIC_PENDING, 0x400

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
  addi t0, t0, 4                           # skip the faulting load or store
  csrw mepc, t0
  mret
fetch_fault:
  csrr t0, mepc
  bne t0, s4, fail                         # mepc = the address we jumped to
  csrw mepc, ra                            # carry on after the jalr
  mret

RVTEST_CODE_END

  .data
RVTEST_DATA_BEGIN

  TEST_DATA

RVTEST_DATA_END
