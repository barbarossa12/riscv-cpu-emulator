# boot_fail.S: a failure with code 42 must come out as exit status 42.
#include "boot_common.h"

  .section .text
  .globl _start
_start:
  FAIL_WITH 42
