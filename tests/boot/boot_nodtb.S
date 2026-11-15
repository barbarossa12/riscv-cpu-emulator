# boot_nodtb.S: without --dtb the hart still gets a0 = 0 and a1 = 0.
#include "boot_common.h"

  .section .text
  .globl _start
_start:
  CHECK_EQ a0, 0, 1
  CHECK_EQ a1, 0, 2
  FINISH FINISHER_PASS
