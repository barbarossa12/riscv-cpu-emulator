# boot_sbi_timer_kernel.S: the timer an OS gets from OpenSBI. sbi_set_timer sets the deadline; OpenSBI takes the machine timer
# interrupt, clears MTIE, sets STIP and returns; S-mode then takes the supervisor timer interrupt (scause 0x80000005). The handler
# disarms the timer with a deadline of all ones, which also clears STIP. While interrupts are on, the main code uses s registers only
# and tight loops, because the handler owns a0, a1, a6, a7 and t4.
#   s1 = interrupts taken, s2 = scause, s3 = time, s4 = sepc, s5 = sip (all as the handler saw them), s8/s9 = deadline low/high
#   exit codes: 3-8 spinning, 9-14 wfi, 15-16 a deadline already past, 21-24 interrupts masked, 30-34 the legacy call, 40 bad sepc
#include "sbi_common.h"

  .macro ARM_TIMER delta, eid                  # the deadline is time + delta, 64-bit; the call's error code is checked by the caller
  csrr s6, time
  csrr s7, timeh
  li t0, \delta
  add s8, s6, t0
  sltu t1, s8, s6                              # carry into the high word
  add s9, s7, t1
  mv a0, s8
  mv a1, s9
  SBI_CALL \eid, 0
  .endm

  .macro ENABLE_TIMER_INTERRUPT
  li t0, SIE_STIE
  csrs sie, t0
  li t0, SSTATUS_SIE
  csrs sstatus, t0
  .endm

  .macro CHECK_TIMER_INTERRUPT code            # exactly one interrupt: the timer one, at or after the deadline, with STIP seen and then cleared
  csrci sstatus, SSTATUS_SIE
  CHECK_EQ s1, 1, \code
  CHECK_EQ s2, 0x80000005, \code + 1
  bgeu s3, s8, 3f                              # time >= deadline (the high word is 0 for the first days of any run)
  FAIL_WITH \code + 2
3:
  andi t0, s5, SIP_STIP
  CHECK_NZ t0, \code + 3                       # sip.STIP was set when the handler ran
  csrr t0, sip
  andi t0, t0, SIP_STIP
  beqz t0, 4f                                  # and the handler's disarm cleared it
  FAIL_WITH \code + 4
4:
  .endm

  .section .text
  .globl _start
_start:
  SET_STVEC handler
  csrr t0, timeh
  CHECK_EQ t0, 0, 1                            # the run is young: timeh is 0, which the deadline arithmetic assumes

  # ---- spin until the interrupt ------------------------------------------------------------------------------
  li s1, 0
  ARM_TIMER 5000, SBI_EXT_TIME
  CHECK_EQ a0, 0, 2
  ENABLE_TIMER_INTERRUPT
spin1:
  beqz s1, spin1
spin1_end:
  CHECK_TIMER_INTERRUPT 3
  la t0, spin1                                 # taken inside the loop
  bltu s4, t0, bad_sepc
  la t0, spin1_end
  bgtu s4, t0, bad_sepc
  li t3, 2000                                  # and it does not repeat
  li t0, SSTATUS_SIE
  csrs sstatus, t0
quiet1:
  addi t3, t3, -1
  bnez t3, quiet1
  csrci sstatus, SSTATUS_SIE
  CHECK_EQ s1, 1, 8

  # ---- wfi: the deadline wakes it ------------------------------------------------------------------------------
  li s1, 0
  ARM_TIMER 5000, SBI_EXT_TIME
  CHECK_EQ a0, 0, 9
  ENABLE_TIMER_INTERRUPT
  li s10, 0                                    # how many times the wfi ran
wait2:
  addi s10, s10, 1
  wfi
after_wfi:
  beqz s1, wait2
  CHECK_TIMER_INTERRUPT 10
  la t0, after_wfi                             # taken on the instruction after the wfi
  bne s4, t0, bad_sepc
  li t0, 3
  bleu s10, t0, 6f                             # the wfi slept until the interrupt: a few turns, not the thousands a busy loop would take
  FAIL_WITH 14
6:

  # ---- a deadline that has already passed fires at once --------------------------------------------------------
  li s1, 0
  ARM_TIMER 0, SBI_EXT_TIME
  CHECK_EQ a0, 0, 15
  ENABLE_TIMER_INTERRUPT
spin3:
  beqz s1, spin3
  CHECK_TIMER_INTERRUPT 16

  # ---- interrupts masked: STIP is visible in sip but nothing is taken, then enabling takes it -----------------------
  li s1, 0
  ARM_TIMER 5000, SBI_EXT_TIME
  CHECK_EQ a0, 0, 21
  li t0, SIE_STIE
  csrs sie, t0                                 # STIE on, sstatus.SIE still off
wait4:
  csrr t0, time
  bltu t0, s8, wait4                           # until the deadline has passed
  li t3, 200000                                # then until STIP shows (a real timer device raises it a little after the deadline)
wait4b:
  csrr t0, sip
  andi t0, t0, SIP_STIP
  bnez t0, 5f
  addi t3, t3, -1
  bnez t3, wait4b
  FAIL_WITH 22                                 # STIP never showed
5:
  CHECK_EQ s1, 0, 23                           # but not taken
  li t0, SSTATUS_SIE
  csrs sstatus, t0
spin4:
  beqz s1, spin4
  CHECK_TIMER_INTERRUPT 24

  # ---- the legacy call does the same ---------------------------------------------------------------------------
  li s1, 0
  ARM_TIMER 5000, SBI_EXT_0_1_SET_TIMER
  ENABLE_TIMER_INTERRUPT
spin5:
  beqz s1, spin5
  CHECK_TIMER_INTERRUPT 30

  PUTS "sbi timer ok\n"
  SHUTDOWN

bad_sepc:
  FAIL_WITH 40

  .align 2
handler:                                       # a supervisor interrupt: record what we can see, disarm the timer
  csrr s2, scause
  csrr s3, time
  csrr s4, sepc
  csrr s5, sip
  addi s1, s1, 1
  li a0, -1
  li a1, -1
  li a7, SBI_EXT_TIME
  li a6, 0
  ecall
  sret
