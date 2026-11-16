# boot_sbi_base_kernel.S: the base extension, error handling and the services with a single hart (HSM, remote fences).
#include "sbi_common.h"

  .section .text
  .globl _start
_start:
  SBI_CALL SBI_EXT_BASE, 0              # spec version
  CHECK_RET 0, 0x03000000, 1            # SBI 3.0, as OpenSBI's banner says
  SBI_CALL SBI_EXT_BASE, 1              # implementation id: 1 is OpenSBI
  CHECK_RET 0, 1, 3
  SBI_CALL SBI_EXT_BASE, 2              # implementation version
  CHECK_EQ a0, 0, 5
  CHECK_NZ a1, 6
  SBI_CALL SBI_EXT_BASE, 4              # machine ids: the call must succeed (the values are the machine's)
  CHECK_EQ a0, 0, 7
  SBI_CALL SBI_EXT_BASE, 5
  CHECK_EQ a0, 0, 8
  SBI_CALL SBI_EXT_BASE, 6
  CHECK_EQ a0, 0, 9

  # probe: every extension a kernel uses is there, and an unknown one is not
  li a0, SBI_EXT_TIME
  SBI_CALL SBI_EXT_BASE, 3
  CHECK_RET 0, 1, 10
  li a0, SBI_EXT_IPI
  SBI_CALL SBI_EXT_BASE, 3
  CHECK_RET 0, 1, 12
  li a0, SBI_EXT_RFENCE
  SBI_CALL SBI_EXT_BASE, 3
  CHECK_RET 0, 1, 14
  li a0, SBI_EXT_HSM
  SBI_CALL SBI_EXT_BASE, 3
  CHECK_RET 0, 1, 16
  li a0, SBI_EXT_SRST
  SBI_CALL SBI_EXT_BASE, 3
  CHECK_RET 0, 1, 18
  li a0, SBI_EXT_DBCN
  SBI_CALL SBI_EXT_BASE, 3
  CHECK_RET 0, 1, 20
  li a0, 0x12345678
  SBI_CALL SBI_EXT_BASE, 3
  CHECK_RET 0, 0, 22

  # not supported: an unknown extension, and an unknown function of a known one, return an error without trapping
  li a0, 0
  li a7, 0x12345678
  li a6, 0
  ecall
  CHECK_EQ a0, SBI_ERR_NOT_SUPPORTED, 24
  SBI_CALL SBI_EXT_BASE, 0x7f
  CHECK_EQ a0, SBI_ERR_NOT_SUPPORTED, 25

  # one hart: hart 0 is started, starting it again is an error, and there is no hart 1
  li a0, 0
  SBI_CALL SBI_EXT_HSM, 2               # hart_get_status
  CHECK_RET 0, 0, 26                    # STARTED
  li a0, 0
  la a1, _start
  li a2, 0
  SBI_CALL SBI_EXT_HSM, 0               # hart_start of a hart that is already running
  CHECK_EQ a0, SBI_ERR_ALREADY_AVAILABLE, 28
  li a0, 1
  SBI_CALL SBI_EXT_HSM, 2
  CHECK_EQ a0, SBI_ERR_INVALID_PARAM, 29

  # remote fences to this hart: fence.i, and sfence.vma of the whole address space
  li a0, 1                              # hart mask: hart 0
  li a1, 0                              # mask base
  SBI_CALL SBI_EXT_RFENCE, 0
  CHECK_EQ a0, 0, 30
  li a0, 1
  li a1, 0
  li a2, 0                              # start 0 and size 0: everything
  li a3, 0
  SBI_CALL SBI_EXT_RFENCE, 1
  CHECK_EQ a0, 0, 31

  PUTS "sbi base ok\n"
  SHUTDOWN
