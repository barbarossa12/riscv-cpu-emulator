# boot_sbi_ipi_kernel.S: a supervisor software interrupt from sbi_send_ipi. OpenSBI raises the machine software interrupt, takes it,
# sets SSIP and returns; S-mode takes the supervisor software interrupt (scause 0x80000001) and clears SSIP itself, as Linux does.
# With sstatus.SIE clear it stays pending in sip until interrupts are enabled.
#   s1 = interrupts taken, s2 = scause, s5 = sip as the handler saw it
#include "sbi_common.h"

  .macro SEND_IPI
  li a0, 1                                     # hart mask 1, base 0: hart 0
  li a1, 0
  SBI_CALL SBI_EXT_IPI, 0
  .endm

  .section .text
  .globl _start
_start:
  SET_STVEC handler
  li t0, SIE_SSIE
  csrs sie, t0

  # ---- enabled: taken, and SSIP is clear after the handler -------------------------------------------------------
  li s1, 0
  li t0, SSTATUS_SIE
  csrs sstatus, t0
  SEND_IPI                                     # the interrupt may arrive before or after the ecall returns
  CHECK_EQ a0, 0, 1
spin1:
  beqz s1, spin1
  csrci sstatus, SSTATUS_SIE
  CHECK_EQ s1, 1, 2
  CHECK_EQ s2, 0x80000001, 3
  andi t0, s5, SIP_SSIP
  CHECK_NZ t0, 4                               # SSIP was set when the handler ran
  csrr t0, sip
  andi t0, t0, SIP_SSIP
  beqz t0, 5f                                  # the handler cleared it
  FAIL_WITH 5
5:
  li t3, 2000                                  # and it does not repeat
  li t0, SSTATUS_SIE
  csrs sstatus, t0
quiet1:
  addi t3, t3, -1
  bnez t3, quiet1
  csrci sstatus, SSTATUS_SIE
  CHECK_EQ s1, 1, 6

  # ---- masked: pending in sip, taken when enabled ---------------------------------------------------------------
  li s1, 0
  SEND_IPI
  CHECK_EQ a0, 0, 10
  li t3, 2000
wait2:
  addi t3, t3, -1
  bnez t3, wait2
  csrr t0, sip
  andi t0, t0, SIP_SSIP
  CHECK_NZ t0, 11                              # pending
  CHECK_EQ s1, 0, 12                           # not taken
  li t0, SSTATUS_SIE
  csrs sstatus, t0
spin2:
  beqz s1, spin2
  csrci sstatus, SSTATUS_SIE
  CHECK_EQ s1, 1, 13
  CHECK_EQ s2, 0x80000001, 14

  PUTS "sbi ipi ok\n"
  SHUTDOWN

  .align 2
handler:
  csrr s2, scause
  csrr s5, sip
  li t4, SIP_SSIP
  csrc sip, t4                                 # clear the pending bit, or the interrupt would repeat
  addi s1, s1, 1
  sret
