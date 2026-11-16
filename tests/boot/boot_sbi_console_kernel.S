# boot_sbi_console_kernel.S: console output through the debug console extension (a byte, a buffer) and the legacy putchar, and
# console_read with nothing typed. The text must come out in order on the emulator's stdout.
#include "sbi_common.h"

  .section .text
  .globl _start
_start:
  li a0, 'A'
  SBI_CALL SBI_EXT_DBCN, 2              # console_write_byte
  CHECK_EQ a0, 0, 1
  li a0, 'B'
  SBI_CALL SBI_EXT_DBCN, 2
  CHECK_EQ a0, 0, 2
  li a0, 10
  SBI_CALL SBI_EXT_DBCN, 2
  CHECK_EQ a0, 0, 3

  PUTS "buffer\n"                       # console_write of a buffer (it also checks the byte count)

  li a0, 4                              # a buffer address above 32 bits is refused on RV32
  la a1, _start
  li a2, 1
  SBI_CALL SBI_EXT_DBCN, 0
  CHECK_EQ a0, -1, 4                    # SBI_ERR_FAILED

  li a0, 'Z'                            # the legacy putchar
  SBI_CALL SBI_EXT_0_1_PUTCHAR, 0
  li a0, 10
  SBI_CALL SBI_EXT_0_1_PUTCHAR, 0

  li a0, 16                             # console_read with nothing typed: it succeeds and reads no bytes
  la a1, readbuf
  li a2, 0
  SBI_CALL SBI_EXT_DBCN, 1
  CHECK_RET 0, 0, 5

  PUTS "sbi console ok\n"
  SHUTDOWN

  .align 2
readbuf:
  .space 16
