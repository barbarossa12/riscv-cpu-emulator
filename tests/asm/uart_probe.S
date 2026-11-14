# uart_probe.S
#-----------------------------------------------------------------------------
#
# The probes Linux's 8250 driver runs to recognise a 16550A (the order is from the Milestone 7 research, the expected
# values from the PC16550D datasheet), then the loopback data path, overrun and interrupt priority with a real program.
# The interrupt line reaches no CPU interrupt before the PLIC (Milestone 8), so every interrupt is observed by polling IIR.

#include "riscv_test.h"
#include "test_macros.h"
#include "uart_common.h"

RVTEST_RV32M
RVTEST_CODE_BEGIN

  # IER: bits 3:0 hold, nothing more
  li TESTNUM, 2
  WR UART_IER, 0x00
  EXPECT UART_IER, 0x0F, 0x00
  WR UART_IER, 0x0F
  EXPECT UART_IER, 0x0F, 0x0F
  WR UART_IER, 0x00

  # loopback self-test: MCR = LOOP | OUT2 | RTS reads back as DCD and CTS
  li TESTNUM, 3
  WR UART_MCR, 0x1A
  EXPECT UART_MSR, 0xF0, 0x90
  WR UART_MCR, 0x03
  RD t2, UART_MSR                        # drop the change flags
  EXPECT UART_MSR, 0xF0, 0xB0            # a connected modem again

  # FIFO detect: the LCR 0xBF dance, then FCR=1 must show IIR bits 7:6
  li TESTNUM, 4
  WR UART_LCR, 0xBF
  WR UART_FCR, 0x00
  WR UART_LCR, 0x00
  WR UART_FCR, 0x01
  EXPECT UART_IIR, 0xC0, 0xC0

  # scratch register
  li TESTNUM, 5
  WR UART_SCR, 0xA5
  EXPECT UART_SCR, 0xFF, 0xA5
  WR UART_SCR, 0x5A
  EXPECT UART_SCR, 0xFF, 0x5A

  # startup: FCR 0x01, 0x07, 0x00, then read LSR, RBR, IIR and MSR; LSR must not read 0xFF
  li TESTNUM, 6
  WR UART_FCR, 0x01
  WR UART_FCR, 0x07
  WR UART_FCR, 0x00
  RD t2, UART_LSR
  li t3, 0xFF
  beq t2, t3, fail
  RD t2, UART_RBR
  RD t2, UART_IIR
  RD t2, UART_MSR

  # THRE test: enabling ETBEI with THR empty must raise the THRE interrupt, every time
  li TESTNUM, 7
  li t4, 3
1:
  WR UART_IER, 0x02
  EXPECT UART_IIR, 0x0F, 0x02
  WR UART_IER, 0x00
  addi t4, t4, -1
  bnez t4, 1b

  # loopback data path: a byte written to THR comes back through RBR
  li TESTNUM, 8
  WR UART_MCR, 0x10
  WR UART_FCR, 0x01
  WR UART_THR, 0x5A
  EXPECT UART_LSR, 0x01, 0x01            # data ready
  EXPECT UART_RBR, 0xFF, 0x5A
  EXPECT UART_LSR, 0x01, 0x00            # and empty again

  # overrun: 17 bytes into a 16-byte FIFO lose the last one and set OE, which an LSR read clears
  li TESTNUM, 9
  li t4, 1
2:
  li t0, UART
  sb t4, UART_THR(t0)
  addi t4, t4, 1
  li t5, 18
  bne t4, t5, 2b
  EXPECT UART_LSR, 0x02, 0x02
  EXPECT UART_LSR, 0x02, 0x00
  EXPECT UART_RBR, 0xFF, 0x01            # the first byte survived
  WR UART_FCR, 0x03                      # clear the rest

  # priority with everything pending: line status, then data, then THRE, then modem status
  li TESTNUM, 10
  WR UART_MCR, 0x00                      # leaving loopback sets the modem change flags
  WR UART_MCR, 0x10
  li t4, 1
3:
  li t0, UART
  sb t4, UART_THR(t0)
  addi t4, t4, 1
  li t5, 18
  bne t4, t5, 3b                         # 16 bytes and an overrun, and THRE re-armed
  WR UART_IER, 0x0F
  EXPECT UART_IIR, 0x0F, 0x06            # line status
  RD t2, UART_LSR                        # reading LSR clears the overrun
  EXPECT UART_IIR, 0x0F, 0x04            # received data
  WR UART_FCR, 0x03                      # drop the data
  EXPECT UART_IIR, 0x0F, 0x02            # THRE (reading IIR clears it)
  EXPECT UART_IIR, 0x0F, 0x00            # modem status
  RD t2, UART_MSR
  EXPECT UART_IIR, 0x0F, 0x01            # nothing left

  TEST_PASSFAIL

RVTEST_CODE_END

  .data
RVTEST_DATA_BEGIN

  TEST_DATA

RVTEST_DATA_END
