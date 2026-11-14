# uart_hello.S
#-----------------------------------------------------------------------------
#
# A console the way OpenSBI drives it (the init order from lib/utils/serial/uart8250.c as read during the Milestone 7
# research), through the real emulator: set the divisor, 8N1, enable the FIFOs, then print a line by polling the
# transmit-empty bit. The text must come out on the emulator's stdout, which the CTest entry matches.

#include "riscv_test.h"
#include "test_macros.h"
#include "uart_common.h"

RVTEST_RV32M
RVTEST_CODE_BEGIN

  # OpenSBI's init: IER=0, DLAB, divisor, 8N1, FIFOs on, MCR=0, read LSR and RBR, scratch=0
  li TESTNUM, 2
  WR UART_IER, 0x00
  WR UART_LCR, 0x80
  WR UART_DLL, 0x0A
  WR UART_DLM, 0x00
  WR UART_LCR, 0x03
  WR UART_FCR, 0x01
  WR UART_MCR, 0x00
  RD t2, UART_LSR
  RD t2, UART_RBR
  WR UART_SCR, 0x00

  # the state that left behind
  li TESTNUM, 3
  EXPECT UART_LCR, 0xFF, 0x03
  EXPECT UART_IIR, 0xFF, 0xC1            # no interrupt, FIFOs enabled
  EXPECT UART_LSR, 0xFF, 0x60            # transmitter empty, nothing received

  # print the message, waiting for THRE before each byte, which stays ready (transmission is instant)
  li TESTNUM, 4
  la t4, message
1:
  lbu t5, 0(t4)
  beqz t5, 3f
2:
  RD t2, UART_LSR
  andi t2, t2, 0x20
  beqz t2, 2b
  li t0, UART
  sb t5, UART_THR(t0)
  EXPECT UART_LSR, 0xFF, 0x60
  addi t4, t4, 1
  j 1b
3:

  # the divisor survives, and DLAB hides it again
  li TESTNUM, 5
  WR UART_LCR, 0x83
  EXPECT UART_DLL, 0xFF, 0x0A
  EXPECT UART_DLM, 0xFF, 0x00
  WR UART_LCR, 0x03
  EXPECT UART_IER, 0xFF, 0x00

  TEST_PASSFAIL

RVTEST_CODE_END

  .data
RVTEST_DATA_BEGIN

  TEST_DATA

message: .asciz "Hello, UART!\n"

RVTEST_DATA_END
