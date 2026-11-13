# sbi_timer.S
#-----------------------------------------------------------------------------
#
# The flow an OS gets from OpenSBI, through the real emulator: M-mode owns the timer and hands S-mode a
# supervisor timer interrupt.
#
#   1. M-mode delegates STI to S-mode, arms mtimecmp 100 ticks ahead, sets MTIE, and returns to S-mode.
#   2. S-mode enables STIE and SIE and loops on wfi, which jumps time to the target.
#   3. The machine timer interrupt goes to M-mode (from S-mode it is always taken, whatever MIE says, which is 0
#      here, machine.adoc:1343). The
#      handler clears MTIE and sets mip.STIP, which S-mode cannot do (supervisor.adoc:431), and returns.
#   4. S-mode takes the delegated STI: scause = 0x80000005, sepc is the instruction after the wfi, SPP = 1,
#      and sip shows STIP. It then ends the test with an ecall.
#
# Registers: s0 = &mtimecmp, s1 = &mtime, s2 = M interrupts handled, s3 = target, s4, s5, s6 = mtime, mepc, mcause
# in the machine handler. No stvec_handler is defined: the harness would delegate the fetch fault and the user ecall.

#include "riscv_test.h"
#include "test_macros.h"

#define MTIMECMP 0x02004000
#define MTIME    0x0200BFF8

.macro arm
  li t0, -1
  sw t0, 0(s0)
  sw zero, 4(s0)
  sw s3, 0(s0)
.endm

RVTEST_RV32M
RVTEST_CODE_BEGIN

  li s0, MTIMECMP
  li s1, MTIME
  li s2, 0

  li TESTNUM, 2
  li t0, MIP_STIP
  csrw mideleg, t0                        # only STI is delegated
  la t0, s_trap
  csrw stvec, t0
  lw s3, 0(s1)
  addi s3, s3, 100
  arm
  li t0, MIP_MTIP
  csrw mie, t0
  li t0, MSTATUS_MPP
  csrc mstatus, t0
  li t0, (MSTATUS_MPP & -MSTATUS_MPP) * PRV_S
  csrs mstatus, t0                        # MPP = S
  li t0, MSTATUS_MPIE
  csrc mstatus, t0                        # MPIE = 0, so MIE is 0 in S-mode: an M interrupt is taken regardless
  la t0, s_code
  csrw mepc, t0
  mret

  # ---- S-mode ---------------------------------------------------------------------------------------------
  .align 2
s_code:
  li TESTNUM, 3
  li t0, MIP_STIP
  csrs sie, t0                            # STIE
  li t0, SSTATUS_SIE
  csrs sstatus, t0
s_idle:
  wfi
s_wait:
  j s_idle

  # the supervisor timer interrupt, delegated
  .align 2
s_trap:
  li TESTNUM, 4
  csrr t0, scause
  li t1, 0x80000005
  bne t0, t1, fail
  csrr t0, sepc
  la t1, s_wait
  bne t0, t1, fail                        # taken on the instruction after the wfi
  csrr t0, sstatus
  andi t0, t0, SSTATUS_SPP
  beqz t0, fail                           # it came from S-mode
  csrr t0, sip
  andi t0, t0, MIP_STIP
  beqz t0, fail                           # sip shows the delegated STIP
  li t1, 1
  bne s2, t1, fail                        # the machine handler ran exactly once
  li t0, 0x80000007
  bne s6, t0, fail                        # on a machine timer interrupt
  la t0, s_wait
  bne s5, t0, fail                        # which also interrupted at the instruction after the wfi
  sub t0, s4, s3                          # and saw mtime within a few ticks of the target
  bltz t0, fail
  li t2, 3
  bgt t0, t2, fail
  RVTEST_PASS                             # an ecall from S-mode ends the test

  TEST_PASSFAIL

  # the machine timer interrupt: forward it as a supervisor timer interrupt, as OpenSBI does
  .align 2
  .global mtvec_handler
mtvec_handler:
  csrr s6, mcause
  csrr s5, mepc
  lw s4, 0(s1)
  li t0, MIP_MTIP
  csrc mie, t0                            # stop MTIE: the timer is not rearmed
  li t0, MIP_STIP
  csrs mip, t0                            # M-mode may set STIP when Sstc is absent (machine.adoc:1462)
  addi s2, s2, 1
  mret

RVTEST_CODE_END

  .data
RVTEST_DATA_BEGIN

  TEST_DATA

RVTEST_DATA_END
