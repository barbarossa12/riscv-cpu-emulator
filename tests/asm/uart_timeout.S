# uart_timeout.S
#-----------------------------------------------------------------------------
#
# The receive timeout on real cpu steps. Run with RISCV_EMU_UART_TIMEOUT=200: a character that sits unread with
# nothing arriving for 200 steps raises the timeout interrupt (IIR 0xC), a read of RBR restarts the timer. The windows
# are loose (about 100 steps against 200, then 300 against 200) so no boundary has to be exact. Without the environment
# variable the timeout is 35000 steps and this program fails at step 3, which the CTest entry for the default relies on.

#include "riscv_test.h"
#include "test_macros.h"
#include "uart_common.h"

  .macro SPIN n                          # about 2*n steps
  li t4, \n
9:
  addi t4, t4, -1
  bnez t4, 9b
  .endm

RVTEST_RV32M
RVTEST_CODE_BEGIN

  # loopback, FIFOs on with a trigger level of 8 (so one character cannot raise the data interrupt), ERBFI on
  li TESTNUM, 2
  WR UART_MCR, 0x10
  WR UART_FCR, 0x81
  WR UART_IER, 0x01
  WR UART_THR, 0x41
  SPIN 50
  EXPECT UART_IIR, 0x0F, 0x01            # about 100 steps: not yet

  li TESTNUM, 3
  SPIN 150
  EXPECT UART_IIR, 0x0F, 0x0C            # about 400 steps: the timeout

  # a second character restarts the timer, and with two characters unread the timeout comes again
  li TESTNUM, 4
  WR UART_THR, 0x42
  EXPECT UART_IIR, 0x0F, 0x01            # the arrival dropped the timeout at once
  SPIN 150
  EXPECT UART_IIR, 0x0F, 0x0C

  # reading one character clears the timeout and restarts the timer (no character arrives meanwhile, so only the read can)
  li TESTNUM, 5
  EXPECT UART_RBR, 0xFF, 0x41
  EXPECT UART_IIR, 0x0F, 0x01

  li TESTNUM, 6
  SPIN 50
  EXPECT UART_IIR, 0x0F, 0x01            # about 100 steps after the read: still not

  li TESTNUM, 7
  SPIN 150
  EXPECT UART_IIR, 0x0F, 0x0C            # the character left behind times out in turn

  TEST_PASSFAIL

RVTEST_CODE_END

  .data
RVTEST_DATA_BEGIN

  TEST_DATA

RVTEST_DATA_END
