# boot_spin.S: never finishes, so only an instruction limit ends it.
  .section .text
  .globl _start
_start:
1:
  j 1b
