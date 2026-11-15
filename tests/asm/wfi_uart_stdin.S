# wfi_uart_stdin.S
#-----------------------------------------------------------------------------
#
# Host input reaches a guest that sleeps in WFI (Milestone 8.4, option B2), through the real emulator. The guest enables the
# receive interrupt at trigger level 1 and loops on WFI with only MEIE set (MTIE clear, so a WFI never jumps time), until the
# data interrupt arrives. Run with RISCV_EMU_UART_STDIN=1 and stdin redirected from a file holding one byte, `x`: a file is
# readable at once, so the first WFI's poll delivers it, and the test does not depend on how fast a writer process is.
#
# The count of WFIs that ran (s2) must be small: with the hook the byte arrives at the first WFI, without it the byte could only
# arrive through the poll that main makes every 1024 steps, which is hundreds of loop turns away. Without RISCV_EMU_UART_STDIN
# no byte ever arrives and the guest waits until the instruction limit, which the CTest entry for "stdin is off by default" uses.
#
# Registers: s0 = UART, s1 = &claim (context 0), s2 = WFIs executed, s6 = byte read, s7 = id claimed, s8 = IIR, s9 = mcause.

#include "riscv_test.h"
#include "test_macros.h"
#include "uart_common.h"
#include "plic_common.h"

RVTEST_RV32M
RVTEST_CODE_BEGIN

  li s0, UART
  li s1, PLIC_CLAIM(0)
  li s2, 0

  li TESTNUM, 2
  WR UART_FCR, 0x01                        # FIFOs on, trigger level 1
  WR UART_IER, 0x01                        # ERBFI
  PW PLIC_PRIORITY(10), 1
  PW PLIC_ENABLE(0), 0x400
  PW PLIC_THRESHOLD(0), 0
  li t0, MEIP
  csrw mie, t0                             # MEIE only
  li a0, MSTATUS_MIE
  csrs mstatus, a0

  li TESTNUM, 3
wait_here:
  addi s2, s2, 1
  wfi
  j wait_here                              # the interrupt, when it comes, ends the loop

done:
  li TESTNUM, 4
  li t0, 0x8000000B
  bne s9, t0, fail
  li t0, 10
  bne s7, t0, fail
  li t0, 0xC4
  bne s8, t0, fail                         # received data available
  li t0, 0x78
  bne s6, t0, fail                         # the byte the host sent
  li t0, 2
  bgtu s2, t0, fail                        # it arrived at the first WFI (or the next), not after hundreds

  TEST_PASSFAIL

  .align 2
  .global mtvec_handler
mtvec_handler:
  csrr s9, mcause
  lw s7, 0(s1)                             # claim
  lbu s8, UART_IIR(s0)
  lbu s6, UART_RBR(s0)
  sw s7, 0(s1)                             # complete
  la t0, done
  csrw mepc, t0
  mret

RVTEST_CODE_END

  .data
RVTEST_DATA_BEGIN

  TEST_DATA

RVTEST_DATA_END
