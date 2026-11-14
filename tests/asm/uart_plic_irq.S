# uart_plic_irq.S
#-----------------------------------------------------------------------------
#
# The interrupt-driven UART the way Linux's 8250 driver uses it, through the real emulator: the UART's interrupt (PLIC source 10)
# reaches M-mode as MEIP, and the handler claims it from the PLIC (context 0), reads IIR, acts on the cause and completes by
# writing the id back. Registers used with interrupts on are a0 to a3 and the s registers only: the handler owns t0 to t4.
#
#   s0 = UART, s1 = &claim (context 0), s2 = interrupts taken, s3 = IIR the handler saw, s4 = id it claimed, s5 = transmit
#   pointer, s6 = last byte received, s7 = receive interrupts, s8 = mepc of the last interrupt, s9 = 1 means "do not complete".
#
#   1. Transmit: a 30-byte line is queued and ETBEI set. MIE is raised, the THRE interrupt is taken before the next instruction
#      (mepc = the instruction after the csrs). The handler writes 16 bytes, as Linux does; the THRE interrupt is re-armed by
#      those writes and comes once more, the handler writes the last 14, finds the queue empty and clears ETBEI: two interrupts
#      in all, IIR = 0xC2 each time, claimed id 10. The text reaches stdout.
#   2. Receive, trigger level 1: a byte through loopback gives the data interrupt (IIR = 0xC4).
#   3. Receive, trigger level 8: one byte raises nothing until the character timeout (IIR = 0xCC).
#   4. A handler that claims but does not complete is not interrupted again, although the line is high again (the gateway keeps
#      the source in flight); completing by hand lets the waiting interrupt through.
#   5. A spurious claim: the request was latched, then the cause went away; the claim still returns 10, IIR shows no cause, and
#      completing it leaves nothing pending.

#include "riscv_test.h"
#include "test_macros.h"
#include "uart_common.h"
#include "plic_common.h"

RVTEST_RV32M
RVTEST_CODE_BEGIN

  li s0, UART
  li s1, PLIC_CLAIM(0)
  li s2, 0
  li s6, 0
  li s7, 0
  li s9, 0
  la s5, message

  li TESTNUM, 2
  WR UART_FCR, 0x01                        # FIFOs on, trigger level 1
  PW PLIC_PRIORITY(10), 1
  PW PLIC_ENABLE(0), 0x400
  PW PLIC_THRESHOLD(0), 0
  li t0, MEIP
  csrs mie, t0

  # ---- 1. transmit by THRE interrupt ------------------------------------------------------------------------------------------
  li a0, 0x02
  sb a0, UART_IER(s0)                      # ETBEI; interrupts are still off, so nothing is taken yet
  li a0, MSTATUS_MIE
  csrs mstatus, a0                         # taken before the very next instruction
after_csrs:
  nop
  li TESTNUM, 3
  la a0, after_csrs
  bne s8, a0, fail                         # mepc of the last interrupt: still the instruction after the csrs
  li a0, 2
  bne s2, a0, fail                         # two THRE interrupts
  li a0, 0xC2
  bne s3, a0, fail                         # IIR: THRE, FIFOs enabled
  li a0, 10
  bne s4, a0, fail
  lbu a0, 0(s5)
  bnez a0, fail                            # the whole queue was sent
  lbu a0, UART_IER(s0)
  bnez a0, fail                            # and the handler cleared ETBEI

  # ---- 2. receive data interrupt (trigger level 1, loopback) ----------------------------------------------------------------
  li TESTNUM, 4
  li a0, 0x10
  sb a0, UART_MCR(s0)                      # loopback
  li a0, 0x01
  sb a0, UART_IER(s0)                      # ERBFI
  li a0, 0x5A
  sb a0, UART_THR(s0)                      # comes back as received data: the interrupt is taken right after this store
  li a0, 3
  bne s2, a0, fail
  li a0, 1
  bne s7, a0, fail
  li a0, 0x5A
  bne s6, a0, fail                         # the handler read it
  li a0, 0xC4
  bne s3, a0, fail                         # IIR: received data available

  # ---- 3. the character timeout (trigger level 8) -----------------------------------------------------------------------------
  li TESTNUM, 5
  li a0, 0x81
  sb a0, UART_FCR(s0)                      # trigger level 8
  li a0, 0x77
  sb a0, UART_THR(s0)                      # one byte: below the trigger level, nothing yet
  li a0, 3
  bne s2, a0, fail
  li a1, 2
1:
  bne s7, a1, 1b                           # wait for the timeout interrupt (35000 steps by default)
  li a0, 4
  bne s2, a0, fail
  li a0, 0xCC
  bne s3, a0, fail                         # IIR: character timeout
  li a0, 0x77
  bne s6, a0, fail

  # ---- 4. a claim that is not completed keeps the source in flight -----------------------------------------------------------------
  li TESTNUM, 6
  li a0, 0x01
  sb a0, UART_FCR(s0)                      # trigger level 1 again
  li s9, 1                                 # the handler will not complete
  li a0, 0x11
  sb a0, UART_THR(s0)                      # first interrupt: claimed, byte read, not completed
  li a0, 5
  bne s2, a0, fail
  li a0, 0x11
  bne s6, a0, fail
  li a0, 0x22
  sb a0, UART_THR(s0)                      # the line is high again, but the source is in flight
  li a1, 200
2:
  addi a1, a1, -1
  bnez a1, 2b
  li a0, 5
  bne s2, a0, fail                         # no new interrupt
  lbu a0, UART_LSR(s0)
  andi a0, a0, 0x01
  beqz a0, fail                            # the byte is waiting
  li s9, 0
  sw s4, 0(s1)                             # complete by hand: the waiting request is forwarded and taken
  li a0, 6
  bne s2, a0, fail
  li a0, 0x22
  bne s6, a0, fail

  # ---- 5. a spurious claim ------------------------------------------------------------------------------------------------------
  li TESTNUM, 7
  li a0, MSTATUS_MIE
  csrc mstatus, a0                         # interrupts off, so the claim is ours
  li a0, 0x33
  sb a0, UART_THR(s0)                      # the line goes high
  csrr a0, mip                             # sampling the gateway latches the request
  sb zero, UART_IER(s0)                    # and the cause goes away before the claim
  lbu a0, UART_RBR(s0)                     # drain the byte
  lw a0, 0(s1)                             # the claim still returns the latched request
  li a1, 10
  bne a0, a1, fail
  lbu a2, UART_IIR(s0)
  li a1, 0xC1
  bne a2, a1, fail                         # no cause: what Linux's handler sees on a spurious claim
  sw a0, 0(s1)                             # complete
  li a1, PLIC_PENDING
  lw a2, 0(a1)
  bnez a2, fail
  lw a2, 0(s1)
  bnez a2, fail                            # nothing left to claim
  li a0, MEIP
  csrr a2, mip
  and a2, a2, a0
  bnez a2, fail
  sb zero, UART_MCR(s0)

  TEST_PASSFAIL

  # the interrupt handler: what Linux's 8250 does (claim, read IIR, act on the cause, complete)
  .align 2
  .global mtvec_handler
mtvec_handler:
  csrr t0, mcause
  li t1, 0x8000000B
  bne t0, t1, fail                         # only the machine external interrupt is expected
  csrr s8, mepc
  addi s2, s2, 1
  lw s4, 0(s1)                             # claim
  lbu s3, UART_IIR(s0)
  andi t0, s3, 0x0F
  li t1, 0x02
  beq t0, t1, h_thre
  li t1, 0x04
  beq t0, t1, h_data
  li t1, 0x0C
  beq t0, t1, h_data
  j h_done                                 # no cause (a spurious claim)
h_thre:
  li t2, 16                                # up to 16 bytes per interrupt, as Linux does
1:
  lbu t3, 0(s5)
  beqz t3, h_empty
  sb t3, UART_THR(s0)
  addi s5, s5, 1
  addi t2, t2, -1
  bnez t2, 1b
  j h_done
h_empty:
  sb zero, UART_IER(s0)                    # nothing left to send: stop the THRE interrupt
  j h_done
h_data:
  lbu s6, UART_RBR(s0)
  addi s7, s7, 1
h_done:
  bnez s9, h_skip
  sw s4, 0(s1)                             # complete: write the claimed id back
h_skip:
  mret

RVTEST_CODE_END

  .data
RVTEST_DATA_BEGIN

  TEST_DATA

message: .asciz "IRQ OK, transmit by interrupt\n"

RVTEST_DATA_END
