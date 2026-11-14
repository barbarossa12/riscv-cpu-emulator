# uart_echo.S
#-----------------------------------------------------------------------------
#
# Reads characters the host feeds in (RISCV_EMU_UART_STDIN=1) and writes each one back, until a newline. With the FIFOs on
# and a line longer than the 16-byte FIFO this exercises the host feed end to end: the poll must hand over only what fits,
# so nothing is lost. Without the environment variable no character ever arrives and the program spins, which the CTest
# entry for "stdin is off by default" relies on (it stops it with a small instruction limit).

#include "riscv_test.h"
#include "test_macros.h"
#include "uart_common.h"

RVTEST_RV32M
RVTEST_CODE_BEGIN

  li TESTNUM, 2
  WR UART_FCR, 0x01
1:
  RD t2, UART_LSR
  andi t2, t2, 0x01
  beqz t2, 1b                            # wait for a character
  RD t2, UART_RBR
  li t0, UART
  sb t2, UART_THR(t0)                    # echo it
  li t3, 10
  bne t2, t3, 1b                         # until a newline

  TEST_PASSFAIL

RVTEST_CODE_END

  .data
RVTEST_DATA_BEGIN

  TEST_DATA

RVTEST_DATA_END
