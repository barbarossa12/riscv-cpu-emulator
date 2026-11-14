# uart_autoconfig_variants.S
#-----------------------------------------------------------------------------
#
# Linux's whole autoconfig sequence for a 16550A, in the order the kernel runs it (drivers/tty/serial/8250/8250_port.c in Linux
# 7.4-rc2): the existence tests (:1135-1180), the type test (:1191-1205), and the extra probes that
# CONFIG_SERIAL_8250_16550A_VARIANTS enables by default on RISC-V (autoconfig_16550a, :939-1098): the EFR checks, the National
# Semiconductor check, the 16750 check, the XScale check and size_fifo (:754-784). Every probe must fall through, so the port
# stays a plain 16550A with a 16-byte FIFO. Offset 2 is IIR when read and FCR (or EFR) when written.

#include "riscv_test.h"
#include "test_macros.h"
#include "uart_common.h"

  .macro WRR off, reg                    # write a register's value to a UART register
  li t0, UART
  sb \reg, \off(t0)
  .endm

RVTEST_RV32M
RVTEST_CODE_BEGIN

  # existence test: IER bits 3:0 must clear to 0 and set to 0xF, then IER is restored
  li TESTNUM, 2
  RD s2, UART_IER                        # scratch = IER
  WR UART_IER, 0x00
  EXPECT UART_IER, 0x0F, 0x00
  WR UART_IER, 0x0F
  EXPECT UART_IER, 0x0F, 0x0F
  WRR UART_IER, s2

  # loopback test: MCR = LOOP | OUT2 | RTS must read back as DCD | CTS, then MCR is restored
  li TESTNUM, 3
  RD s3, UART_MCR                        # save_mcr
  RD s4, UART_LCR                        # save_lcr
  WR UART_MCR, 0x1A
  EXPECT UART_MSR, 0xF0, 0x90
  WRR UART_MCR, s3

  # type test: EFR = 0 through LCR 0xBF, FIFOs on, the IIR top bits give a 16550A
  li TESTNUM, 4
  WR UART_LCR, 0xBF
  WR UART_FCR, 0x00
  WR UART_LCR, 0x00
  WR UART_FCR, 0x01
  EXPECT UART_IIR, 0xC0, 0xC0

  # autoconfig_16550a, EFR with DLAB set (LCR 0x80): reading offset 2 must not give 0, or it would look like an EFR
  li TESTNUM, 5
  WR UART_LCR, 0x80
  RD t4, UART_IIR
  beqz t4, fail

  # and with LCR 0xBF
  li TESTNUM, 6
  WR UART_LCR, 0xBF
  RD t4, UART_IIR
  beqz t4, fail

  # National Semiconductor check: the LOOP bit of MCR must not show up in offset 2 (bank 2, LCR 0xE0)
  li TESTNUM, 7
  WR UART_LCR, 0x00
  RD s5, UART_MCR                        # status1
  WR UART_LCR, 0xE0
  RD s6, UART_IIR                        # status2
  xor t4, s5, s6
  andi t4, t4, 0x10
  bnez t4, 1f                            # the kernel only tries to flip LOOP if the bit matches
  WR UART_LCR, 0x00
  xori t4, s5, 0x10
  WRR UART_MCR, t4                       # flip LOOP
  WR UART_LCR, 0xE0
  RD s6, UART_IIR                        # status2 again
  WR UART_LCR, 0x00
  WRR UART_MCR, s5
  xor t4, s5, s6
  andi t4, t4, 0x10
  bnez t4, fail                          # LOOP followed MCR: it would be taken for a National Semiconductor chip
1:

  # 16750 check: FCR bit 5 (64-byte FIFOs) must not set IIR bit 5, with or without DLAB
  li TESTNUM, 8
  WR UART_LCR, 0x00
  WR UART_FCR, 0x21
  EXPECT UART_IIR, 0xE0, 0xC0
  WR UART_FCR, 0x01
  WR UART_LCR, 0x80
  WR UART_FCR, 0x21
  EXPECT UART_IIR, 0xE0, 0xC0            # a 16750 would read 0xE0 here
  WR UART_FCR, 0x01
  WR UART_LCR, 0x00

  # XScale check: the IER bit 6 (UUE) must not be writable
  li TESTNUM, 9
  RD s7, UART_IER
  andi t4, s7, 0xBF                      # iersave & ~UUE
  WRR UART_IER, t4
  EXPECT UART_IER, 0x40, 0x00
  ori t4, s7, 0x40                       # iersave | UUE
  WRR UART_IER, t4
  EXPECT UART_IER, 0x40, 0x00            # ours must not hold it, or it would look like an XScale UART
  WRR UART_IER, s7

  # size_fifo: 256 bytes through loopback with the FIFOs on, then count how many come back. A 16550A holds 16; 64 would
  # make the kernel take it for a U6 16550A.
  li TESTNUM, 10
  RD s8, UART_LCR                        # old_lcr
  WR UART_LCR, 0x00
  RD s9, UART_IIR                        # old_fcr (the kernel reads offset 2 and writes it back as an FCR)
  RD s10, UART_MCR                       # old_mcr
  WR UART_FCR, 0x07                      # ENABLE | CLEAR_RCVR | CLEAR_XMIT
  WR UART_MCR, 0x10                      # LOOP
  WR UART_LCR, 0x80
  RD s11, UART_DLL                       # old_dl
  WR UART_DLL, 0x01
  WR UART_DLM, 0x00
  WR UART_LCR, 0x03
  li t4, 0
2:
  li t0, UART
  sb t4, UART_THR(t0)
  addi t4, t4, 1
  li t5, 256
  bne t4, t5, 2b
  li t4, 0
3:
  RD t2, UART_LSR
  andi t2, t2, 0x01
  beqz t2, 4f
  RD t2, UART_RBR
  addi t4, t4, 1
  li t5, 256
  bne t4, t5, 3b
4:
  li t5, 16
  bne t4, t5, fail                       # 16 bytes, so it stays a plain 16550A
  WRR UART_FCR, s9
  WRR UART_MCR, s10
  WR UART_LCR, 0x80
  WRR UART_DLL, s11
  WR UART_DLM, 0x00
  WRR UART_LCR, s8

  # the end of autoconfig: restore, clear the FIFOs, read RBR, clear IER, then startup's LSR safety check
  li TESTNUM, 11
  WRR UART_LCR, s4
  WRR UART_MCR, s3
  WR UART_FCR, 0x01
  WR UART_FCR, 0x07
  WR UART_FCR, 0x00
  RD t2, UART_RBR
  WR UART_IER, 0x00
  RD t2, UART_LSR
  li t3, 0xFF
  beq t2, t3, fail                       # LSR must not read 0xFF
  EXPECT UART_LSR, 0x03, 0x00            # the overrun from size_fifo was cleared by the read, and nothing is waiting

  TEST_PASSFAIL

RVTEST_CODE_END

  .data
RVTEST_DATA_BEGIN

  TEST_DATA

RVTEST_DATA_END
