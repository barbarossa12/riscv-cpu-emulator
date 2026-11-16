# boot_sbi_handoff_kernel.S: the next stage after OpenSBI's fw_jump, linked at KERNEL_ADDR. What OpenSBI promises on entry
# (sbi_hart_switch_mode, and Documentation/arch/riscv/boot.rst for Linux), checked one item at a time with its own exit code:
#   1  entered at KERNEL_ADDR               5  satp = 0 (the MMU is off)
#   2  a0 = hart id 0                        6  sie = 0 and 7 sscratch = 0
#   3  a1 = OpenSBI's copy of the device     8  stvec = KERNEL_ADDR
#      tree (FW_JUMP_FDT_OFFSET 0x2200000)   9  we are in S-mode: reading mhartid raises illegal instruction, which OpenSBI
#   4  the 0xd00dfeed magic is there            redirects to our stvec handler with scause 2 and sepc at the instruction
# Then it asks OpenSBI to shut the machine down (SBI System Reset, shutdown). That runs OpenSBI's syscon-poweroff driver, its
# read-modify-write on the finisher, and ends the emulator with exit status 0 and no help from this stub. Exit 10 if the call returns.
#include "boot_common.h"

#define FW_FDT_ADDR     0x82200000     // MACHINE_RAM_BASE + OpenSBI's FW_JUMP_FDT_OFFSET
#define SBI_EXT_SRST    0x53525354     // "SRST"
#define SRST_SYSTEM_RESET 0
#define SRST_SHUTDOWN   0
#define SRST_COLD_REBOOT 1

#ifndef RESET_TYPE                     // the reboot twin (boot_sbi_reboot_kernel.S) defines it as SRST_COLD_REBOOT
#define RESET_TYPE SRST_SHUTDOWN
#endif

  .section .text
  .globl _start
_start:
  auipc t2, 0
  CHECK_EQ t2, KERNEL_ADDR, 1
  CHECK_EQ a0, 0, 2
  CHECK_EQ a1, FW_FDT_ADDR, 3
  lw t2, 0(a1)
  CHECK_EQ t2, 0xedfe0dd0, 4            # 0xd00dfeed as a little-endian word
  csrr t2, satp
  CHECK_EQ t2, 0, 5
  csrr t2, sie
  CHECK_EQ t2, 0, 6
  csrr t2, sscratch
  CHECK_EQ t2, 0, 7
  csrr t2, stvec
  CHECK_EQ t2, KERNEL_ADDR, 8

  la t2, trap
  csrw stvec, t2
  li s1, 0                              # traps taken
  csrr t3, mhartid                      # an M-mode CSR: illegal in S-mode
  CHECK_EQ s1, 1, 9

  li a7, SBI_EXT_SRST
  li a6, SRST_SYSTEM_RESET
  li a0, RESET_TYPE
  li a1, 0                              # no reason
  ecall
  FAIL_WITH 10                          # the call must not return

  .align 2
trap:                                   # a redirected illegal instruction: scause 2, then step over the instruction
  csrr t4, scause
  li t5, 2
  bne t4, t5, bad
  csrr t4, sepc
  addi t4, t4, 4
  csrw sepc, t4
  addi s1, s1, 1
  sret
bad:
  FAIL_WITH 11
