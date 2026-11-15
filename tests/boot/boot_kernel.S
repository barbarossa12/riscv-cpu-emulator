# boot_kernel.S: the next stage, linked at KERNEL_ADDR (the build script does that for names ending in _kernel). It must run at its
# link address and still see a0 = 0 and a1 = the device tree, as the firmware received them.
#include "boot_common.h"

  .section .text
  .globl _start
_start:
  auipc t2, 0
  CHECK_EQ t2, KERNEL_ADDR, 1
  CHECK_EQ a0, 0, 2
  CHECK_EQ a1, DTB_ADDR, 3
  lw t2, 0(a1)
  CHECK_EQ t2, 0xedfe0dd0, 4
  FINISH FINISHER_PASS
