# boot_reboot.S: a reboot request ends the run with its own status.
#include "boot_common.h"

  .section .text
  .globl _start
_start:
  FINISH FINISHER_RESET
