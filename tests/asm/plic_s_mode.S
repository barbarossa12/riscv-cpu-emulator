# plic_s_mode.S
#-----------------------------------------------------------------------------
#
# The external interrupt an OS gets from OpenSBI, through the real emulator: the UART's interrupt (PLIC source 10) is enabled
# for context 1, which drives SEIP.
#
#   1. M-mode, interrupts off: the SEIP read-modify-write rule (machine.adoc:1436-1443). With the controller signal E high
#      and the software bit B clear, mip.SEIP reads 1; csrrsi mip, 2 (SSIP) must not copy E into B, so once E drops SEIP reads 0
#      and SSIP stays set; csrrci clears SSIP. B set by csrs stays set when E rises and drops, and csrc clears it.
#   2. Not delegated: mideleg is 0 and MEIE is clear, only SEIE is set. The interrupt, raised from M-mode, is taken from S-mode
#      to M-mode with mcause = 0x80000009 (machine.adoc:1343). The machine handler claims from context 1, reads IIR = 0xC2 (FIFOs
#      on), clears ETBEI and completes, then redirects S-mode to the next phase.
#   3. Delegated (mideleg bit 9): S-mode enables SEIE and SIE and then ETBEI. The interrupt goes to S-mode before the next
#      instruction: scause = 0x80000009, sepc = the instruction after the store, SPP = 1, sip shows SEIP, the claim from context 1
#      returns 10, IIR = 0xC2. After the S handler completes it, sip.SEIP reads 0. The machine handler ran only once (phase 2).
#
# Registers: s0 = &UART, s1 = &claim (context 1), s2 = machine interrupts handled, s4, s5, s6 = id claimed, mepc, mcause in the
# machine handler. No stvec_handler is defined for the harness, as in sbi_timer.S.

#include "riscv_test.h"
#include "test_macros.h"
#include "uart_common.h"
#include "plic_common.h"

RVTEST_RV32M
RVTEST_CODE_BEGIN

  li s0, UART
  li s1, PLIC_CLAIM(1)
  li s2, 0

  li TESTNUM, 2
  WR UART_FCR, 0x01
  PW PLIC_PRIORITY(10), 1
  PW PLIC_ENABLE(1), 0x400                 # context 1 only
  PW PLIC_THRESHOLD(1), 0

  # ---- 1. the SEIP read-modify-write rule ---------------------------------------------------------------------------------------
  li TESTNUM, 3
  WR UART_IER, 0x02                        # E high
  MIPEXPECT SEIP, SEIP
  MIPEXPECT SSIP, 0
  csrsi mip, 2                             # csrrsi x0, mip, 2: sets SSIP only
  MIPEXPECT SSIP, SSIP
  RD t2, UART_IIR                          # the cause goes away
  PEXPECT PLIC_CLAIM(1), 10                # the request was latched, so the claim still returns it
  PW PLIC_CLAIM(1), 10                     # complete it
  MIPEXPECT SEIP, 0                        # E dropped and B was never set: E was not copied into B
  MIPEXPECT SSIP, SSIP
  csrci mip, 2                             # csrrci x0, mip, 2: clears SSIP
  MIPEXPECT SSIP, 0
  MIPEXPECT SEIP, 0

  li TESTNUM, 4
  li t0, SEIP
  csrs mip, t0                             # B = 1
  WR UART_IER, 0x00
  WR UART_IER, 0x02                        # E rises
  MIPEXPECT SEIP, SEIP
  csrsi mip, 2                             # E must still not matter for B
  RD t2, UART_IIR
  PEXPECT PLIC_CLAIM(1), 10
  PW PLIC_CLAIM(1), 10                     # E drops
  MIPEXPECT SEIP, SEIP                     # B keeps it
  li t0, SEIP
  csrc mip, t0                             # B = 0
  csrci mip, 2
  MIPEXPECT SEIP, 0
  MIPEXPECT SSIP, 0
  WR UART_IER, 0x00

  # ---- 2. not delegated: the interrupt goes to M-mode with cause 9 ----------------------------------------------------------------
  li TESTNUM, 5
  csrw mideleg, zero
  li t0, SEIP
  csrw mie, t0                             # SEIE only
  la t0, s_trap
  csrw stvec, t0
  WR UART_IER, 0x02                        # raised from M-mode, where S-level interrupts are never taken
  li t0, MSTATUS_MPP
  csrc mstatus, t0
  li t0, (MSTATUS_MPP & -MSTATUS_MPP) * PRV_S
  csrs mstatus, t0                         # MPP = S
  li t0, MSTATUS_MPIE
  csrc mstatus, t0
  la t0, s_spin
  csrw mepc, t0
  mret

  .align 2
s_spin:
  j s_spin                                 # the interrupt arrives here, in S-mode

  # ---- 3. delegated: S-mode takes it ---------------------------------------------------------------------------------------------
  .align 2
s_code:
  li TESTNUM, 6
  li t0, SEIP
  csrs sie, t0                             # SEIE
  li t0, SSTATUS_SIE
  csrs sstatus, t0
  li t0, 0x02
  sb t0, UART_IER(s0)                      # ETBEI: E rises, and the interrupt is taken before the next instruction
s_after:
  j s_after                                # not reached: the handler ends the test

  .align 2
s_trap:
  li TESTNUM, 7
  csrr t0, scause
  li t1, 0x80000009
  bne t0, t1, fail                         # an interrupt (bit 31), supervisor external (9)
  csrr t0, sepc
  la t1, s_after
  bne t0, t1, fail                         # on the instruction after the store
  csrr t0, sstatus
  andi t0, t0, SSTATUS_SPP
  beqz t0, fail                            # from S-mode
  csrr t0, sip
  andi t0, t0, SEIP
  beqz t0, fail                            # sip shows the delegated SEIP
  lw t0, 0(s1)                             # claim from context 1
  li t1, 10
  bne t0, t1, fail
  lbu t0, UART_IIR(s0)
  li t1, 0xC2
  bne t0, t1, fail
  sb zero, UART_IER(s0)
  sw t1, 0(s1)                             # a complete for 0xC2, which is no source: does nothing
  li t1, 10
  sw t1, 0(s1)                             # complete
  csrr t0, sip
  andi t0, t0, SEIP
  bnez t0, fail                            # the controller let go
  li t1, 1
  bne s2, t1, fail                         # the machine handler ran exactly once, in phase 2
  li t0, 0x80000009
  bne s6, t0, fail
  li t0, 10
  bne s4, t0, fail
  la t0, s_spin
  bne s5, t0, fail
  RVTEST_PASS                              # an ecall from S-mode ends the test

  TEST_PASSFAIL

  # phase 2: the machine handler for the undelegated supervisor external interrupt
  .align 2
  .global mtvec_handler
mtvec_handler:
  csrr s6, mcause
  csrr s5, mepc
  lw s4, 0(s1)                             # claim from context 1
  lbu t0, UART_IIR(s0)
  li t1, 0xC2
  bne t0, t1, fail
  sb zero, UART_IER(s0)
  sw s4, 0(s1)                             # complete
  addi s2, s2, 1
  li t0, SEIP
  csrw mideleg, t0                         # now delegate it, and carry on in S-mode at the next phase
  la t0, s_code
  csrw mepc, t0
  mret

RVTEST_CODE_END

  .data
RVTEST_DATA_BEGIN

  TEST_DATA

RVTEST_DATA_END
