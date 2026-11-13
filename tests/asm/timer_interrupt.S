# timer_interrupt.S
#-----------------------------------------------------------------------------
#
# The machine timer interrupt through the real emulator, deterministic because mtime is virtual.
#
#   1. mtimecmp is armed 50 ticks ahead with the manual's three-store RV32 sequence, MTIE is set and MIE is
#      clear: the timer passes the target and mip.MTIP reads set, but no interrupt is taken.
#   2. csrs mstatus, MIE: the interrupt is taken before the very next instruction, so mepc is the address
#      after the csrs, and mcause is 0x80000007.
#   3. With MIE set, mtimecmp is armed 40 ticks ahead and the program spins on one instruction. The interrupt
#      lands within a few ticks of the target, with mepc at the spin instruction.
#   4. After the handler disarms the timer it does not fire again, and mret has restored MIE.
#
# Registers: s0 = &mtimecmp, s1 = &mtime, s2 = interrupts handled, s3 = target, s4, s5, s6 = mtime, mepc and
# mcause as the handler saw them. The harness's trap_vector uses t5 and t6 and sends interrupts to mtvec_handler.

#include "riscv_test.h"
#include "test_macros.h"

#define MTIMECMP 0x02004000
#define MTIME    0x0200BFF8

# mtimecmp = (zero:s3): store -1 to the low half first, so it never drops below the old or new value
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

  # ---- phase 1: armed, MTIE set, MIE clear: nothing is taken --------------------------------------------
  li TESTNUM, 2
  lw s3, 0(s1)
  addi s3, s3, 50
  arm
  li t0, MIP_MTIP
  csrs mie, t0

  li TESTNUM, 3
  li t1, 300                              # 600 instructions = 60 ticks, past the target of 50
1:
  addi t1, t1, -1
  bnez t1, 1b
  bnez s2, fail                           # no interrupt yet
  lw t0, 0(s1)
  bltu t0, s3, fail                       # mtime has passed the target
  csrr t0, mip
  andi t0, t0, MIP_MTIP
  beqz t0, fail                           # and the line is visible in mip

  # ---- phase 2: enabling MIE takes it before the next instruction ---------------------------------------
  li TESTNUM, 4
  li t0, MSTATUS_MIE
  csrs mstatus, t0
after_csrs:
  nop
  la t0, after_csrs
  bne s5, t0, fail                        # mepc = the instruction after the csrs
  li t0, 0x80000007
  bne s6, t0, fail                        # an interrupt (bit 31), machine timer (7)
  li t1, 1
  bne s2, t1, fail
  bltu s4, s3, fail                       # the handler saw mtime at or past the target

  # ---- phase 3: armed again with MIE set: the latency is a few ticks ------------------------------------
  li TESTNUM, 5
  lw s3, 0(s1)
  addi s3, s3, 40
  arm
  li t1, 2
spin:
  bne s2, t1, spin                        # waits for the handler to count the second interrupt
  la t0, spin
  bne s5, t0, fail                        # it was taken on the spin instruction
  sub t0, s4, s3                          # ticks between the target and the handler's read of mtime
  bltz t0, fail
  li t2, 3
  bgt t0, t2, fail

  # ---- phase 4: once only, and MIE is back on -------------------------------------------------------------
  li TESTNUM, 6
  li t1, 150
2:
  addi t1, t1, -1
  bnez t1, 2b
  li t1, 2
  bne s2, t1, fail                        # it fired exactly twice in all
  csrr t0, mstatus
  andi t0, t0, MSTATUS_MIE
  beqz t0, fail                           # mret restored MIE from MPIE

  TEST_PASSFAIL

  .align 2
  .global mtvec_handler
mtvec_handler:
  csrr s6, mcause
  csrr s5, mepc
  lw s4, 0(s1)
  li t0, -1
  sw t0, 0(s0)
  sw t0, 4(s0)                            # disarm: mtimecmp is all ones, so MTIP clears
  addi s2, s2, 1
  mret

RVTEST_CODE_END

  .data
RVTEST_DATA_BEGIN

  TEST_DATA

RVTEST_DATA_END
