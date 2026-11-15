# boot_fail0.S: a failure whose code is 0 must not look like a pass (the exit status is 1).
#include "boot_common.h"

  .section .text
  .globl _start
_start:
  FAIL_WITH 0
