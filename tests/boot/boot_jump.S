# boot_jump.S: a firmware stub that hands over to the next stage at KERNEL_ADDR with a0 and a1 untouched, as OpenSBI's fw_jump does.
#include "boot_common.h"

  .section .text
  .globl _start
_start:
  li t0, KERNEL_ADDR
  jr t0
