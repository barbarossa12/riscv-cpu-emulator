# wfi_uart_timeout.S
#-----------------------------------------------------------------------------
#
# WFI lets idle time pass for the UART as well as the timer (Milestone 8.4), through the real emulator. One byte sits in the
# receive FIFO below the trigger level (8), so the only thing that can raise the UART's interrupt is the character timeout,
# 35000 steps (3500 ticks of the 10 MHz timebase) after the byte arrived. The machine timer is armed 1000000 ticks away, with
# MTIE set, so if WFI only knew the timer it would jump there and the timer interrupt would come first.
#
# What must happen: the first WFI advances time to the UART's timeout and no further, the interrupt that follows is the machine
# external interrupt (mcause 0x8000000B) with mepc at the instruction after the WFI, IIR reads 0xCC (timeout), the claimed id is
# 10, the byte is 0x77, MTIP is still clear, and `time` moved by 3000 to 4000 ticks, not 1000000. With interrupts on, the main
# code uses a registers and s registers only: the handler owns t0 to t4.
#
# Registers: s0 = UART, s1 = &claim (context 0), s3 = time before the WFI, s4 = time in the handler, s5 = mepc, s6 = mcause,
# s7 = id claimed, s8 = IIR, s9 = byte read.

#include "riscv_test.h"
#include "test_macros.h"
#include "uart_common.h"
#include "plic_common.h"

#define MTIMECMP 0x02004000
#define MTIME    0x0200BFF8

RVTEST_RV32M
RVTEST_CODE_BEGIN

  li s0, UART
  li s1, PLIC_CLAIM(0)

  li TESTNUM, 2
  WR UART_FCR, 0x81                        # FIFOs on, trigger level 8
  WR UART_MCR, 0x10                        # loopback: a byte written to THR is received
  WR UART_IER, 0x01                        # ERBFI
  PW PLIC_PRIORITY(10), 1
  PW PLIC_ENABLE(0), 0x400
  PW PLIC_THRESHOLD(0), 0
  li t0, MTIME
  lw t1, 0(t0)
  li t2, 1000000
  add t1, t1, t2                           # 1000000 ticks from now
  li t0, MTIMECMP
  li t2, -1
  sw t2, 0(t0)
  sw zero, 4(t0)
  sw t1, 0(t0)
  li t0, 0x880                             # MEIE and MTIE
  csrw mie, t0
  WR UART_THR, 0x77                        # one byte: below the trigger level, so no interrupt yet

  li TESTNUM, 3
  csrr s3, time
  li a0, MSTATUS_MIE
  csrs mstatus, a0
wait_here:
  wfi
after_wfi:
  j wait_here                              # not reached again: the handler ends the wait

done:
  li TESTNUM, 4
  li t0, 0x8000000B
  bne s6, t0, fail                         # the machine external interrupt, not the timer
  la t0, after_wfi
  bne s5, t0, fail                         # taken on the instruction after the wfi
  li t0, 10
  bne s7, t0, fail
  li t0, 0xCC
  bne s8, t0, fail                         # the character timeout
  li t0, 0x77
  bne s9, t0, fail
  sub t0, s4, s3                           # ticks that passed while the guest waited
  li t1, 3000
  bltu t0, t1, fail
  li t1, 4000
  bgeu t0, t1, fail                        # about 3500: the UART's deadline, nowhere near the timer's 1000000
  csrr t0, mip
  andi t0, t0, 0x80
  bnez t0, fail                            # the timer was never reached

  TEST_PASSFAIL

  .align 2
  .global mtvec_handler
mtvec_handler:
  csrr s6, mcause
  csrr s5, mepc
  csrr s4, time
  lw s7, 0(s1)                             # claim
  lbu s8, UART_IIR(s0)
  lbu s9, UART_RBR(s0)
  sw s7, 0(s1)                             # complete
  la t0, done
  csrw mepc, t0                            # the wait is over
  mret

RVTEST_CODE_END

  .data
RVTEST_DATA_BEGIN

  TEST_DATA

RVTEST_DATA_END
