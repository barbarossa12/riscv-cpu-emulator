# boot_ok.S: a firmware stub for the loader. What a real firmware is promised on entry (the RISC-V boot convention, and
# Documentation/arch/riscv/boot.rst for Linux): a0 = hart id, a1 = the device tree, satp = 0, machine mode. The stub checks
# each one, with a different exit code for each failure, and finishes with a pass.
#include "boot_common.h"

  .section .text
  .globl _start
_start:
  CHECK_EQ a0, 0, 1                     # hart 0
  CHECK_EQ a1, DTB_ADDR, 2              # the device tree is where the machine puts it
  lw t2, 0(a1)
  CHECK_EQ t2, 0xedfe0dd0, 3            # 0xd00dfeed (big-endian magic) as a little-endian word
  csrr t2, satp
  CHECK_EQ t2, 0, 4                     # the MMU is off
  csrr t2, mhartid                      # a read of an M-mode CSR works only in M-mode
  CHECK_EQ t2, 0, 5
  csrr t2, misa
  srli t2, t2, 30
  CHECK_EQ t2, 1, 6                     # MXL = 1: 32 bits
  FINISH FINISHER_PASS
