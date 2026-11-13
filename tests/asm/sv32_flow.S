# sv32_flow.S
#-----------------------------------------------------------------------------
#
# Sv32 end to end: M-mode builds page tables and turns on translation, S-mode runs on an identity
# megapage and drops to U-mode, which runs from a mapped user page. Page faults are delegated to
# S-mode (load and store) or left in M-mode (instruction fetch), and the handlers check the cause
# and the faulting virtual address each time.
#
# Virtual layout (root table = root, one level-0 table = l0):
#   root[0x200]  megapage 0x80000000 -> 0x80000000, R W X, not a user page   (S-mode code and data)
#   root[1]   -> l0
#   0x00400000   user_code, R X U      (U-mode code)
#   0x00401000   user_data, R W U
#   0x00402000   user_ro,   R U        (read-only)
#   0x00403000   unmapped
#   0x00404000   user_ad,   R W U with A = D = 0 (the hardware sets them)
#
# medeleg delegates load (13) and store (15) page faults to S-mode. The instruction page fault (12)
# and the ecall from U-mode (8) are left in M-mode. The harness's own pass/fail ecall is not delegated.
#
# s2 counts the faults handled so far. s3 and s4 hold the scause and stval the next fault must have.

#include "riscv_test.h"
#include "test_macros.h"

#define VA_USER_CODE 0x00400000
#define VA_USER_DATA 0x00401000
#define VA_USER_RO   0x00402000
#define VA_UNMAPPED  0x00403000
#define VA_USER_AD   0x00404000
#define VA_S_PAGE    0x80000000

#define PTE_LEAF_RWX_AD (PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D)

# Store a page table entry: table[index] = value
.macro set_pte table, index, value
  la t3, \table
  li t4, (\index) * 4
  add t3, t3, t4
  sw \value, 0(t3)
.endm

# A leaf entry mapping the 4 KiB page at `target` with the given flags
.macro leaf_pte reg, target, flags
  la \reg, \target
  srli \reg, \reg, 2                      # (pa >> 12) << 10
  ori \reg, \reg, \flags
.endm

RVTEST_RV32M
RVTEST_CODE_BEGIN

  # ---- M-mode: page tables ---------------------------------------------------
  li TESTNUM, 2
  li t0, (0x80000000 >> 2) | PTE_LEAF_RWX_AD
  set_pte root, 0x200, t0                 # identity megapage for S-mode

  la t0, l0
  srli t0, t0, 2
  ori t0, t0, PTE_V                       # root[1] points to l0
  set_pte root, 1, t0

  leaf_pte t0, user_code, PTE_V | PTE_R | PTE_X | PTE_U | PTE_A | PTE_D
  set_pte l0, 0, t0
  leaf_pte t0, user_data, PTE_V | PTE_R | PTE_W | PTE_U | PTE_A | PTE_D
  set_pte l0, 1, t0
  leaf_pte t0, user_ro, PTE_V | PTE_R | PTE_U | PTE_A | PTE_D
  set_pte l0, 2, t0
                                          # l0[3] stays 0: unmapped
  leaf_pte t0, user_ad, PTE_V | PTE_R | PTE_W | PTE_U
  set_pte l0, 4, t0                       # A and D clear

  # ---- M-mode: turn translation on, delegate, enter S-mode --------------------
  la t0, root
  srli t0, t0, 12
  li t1, 0x80000000                       # MODE = Sv32
  or t0, t0, t1
  csrw satp, t0
  sfence.vma

  li t0, (1 << CAUSE_LOAD_PAGE_FAULT) | (1 << CAUSE_STORE_PAGE_FAULT)
  csrw medeleg, t0
  la t0, stvec_entry
  csrw stvec, t0

  li s2, 0
  la t0, s_code
  csrw mepc, t0
  li t0, MSTATUS_MPP
  csrc mstatus, t0
  li t0, (MSTATUS_MPP & -MSTATUS_MPP) * PRV_S
  csrs mstatus, t0                        # MPP = S
  mret

  # ---- S-mode (running on the identity megapage) -------------------------------
  .align 2
s_code:
  # S1: a supervisor load from a user page with SUM = 0 is a delegated load page fault
  li TESTNUM, 3
  li s3, CAUSE_LOAD_PAGE_FAULT
  li s4, VA_USER_DATA
  li s0, VA_USER_DATA
  lw t1, 0(s0)
  li t4, 1
  bne s2, t4, fail                        # it trapped once, and the handler checked scause and stval

  # S2: with SUM = 1 the same page is accessible
  li TESTNUM, 4
  li t0, SSTATUS_SUM
  csrs sstatus, t0
  li t2, 0xCAFE0001
  sw t2, 0(s0)
  lw t3, 0(s0)
  bne t2, t3, fail
  li t0, SSTATUS_SUM
  csrc sstatus, t0

  # S3: SRET to U-mode at the user code page
  li TESTNUM, 5
  li t0, SSTATUS_SPP
  csrc sstatus, t0                        # SPP = U
  li t0, VA_USER_CODE
  csrw sepc, t0
  sret

  TEST_PASSFAIL

  # ---- S-mode trap handler (delegated page faults, from S or U) ---------------
  .align 2
stvec_entry:
  csrr t0, scause
  bne t0, s3, fail
  csrr t0, stval
  bne t0, s4, fail
  addi s2, s2, 1
  csrr t0, sepc
  addi t0, t0, 4                          # every faulting instruction is 4 bytes (no C extension)
  csrw sepc, t0
  sret

  # ---- M-mode trap handler: only the undelegated instruction page fault arrives here ----
  .align 2
  .global mtvec_handler
mtvec_handler:
  csrr t0, mcause
  li t1, CAUSE_FETCH_PAGE_FAULT
  bne t0, t1, fail
  li t1, VA_USER_DATA
  csrr t0, mtval
  bne t0, t1, fail                        # mtval = the address we jumped to
  csrr t0, mepc
  bne t0, t1, fail                        # and so is mepc

  la t2, user_data                        # M-mode ignores satp: U's store is visible physically
  lw t0, 0(t2)
  li t1, 0x1234ABCD
  bne t0, t1, fail

  la t2, l0                               # the hardware set A and D on the page U stored to
  lw t0, 4 * 4(t2)
  li t1, PTE_A | PTE_D
  and t0, t0, t1
  bne t0, t1, fail

  addi s2, s2, 1
  csrw mepc, ra                           # resume after the jalr
  mret

  # ---- U-mode code, mapped at VA_USER_CODE. It runs at a different address from where it is
  # linked, so it uses no la and no branch outside this page: only absolute addresses and
  # branches within it. The harness's fail and pass sequences are repeated here for that reason.
  .align 12
user_code:
  # U1: store to and load from a user page
  li TESTNUM, 6
  li s0, VA_USER_DATA
  li t0, 0x1234ABCD
  sw t0, 0(s0)
  lw t1, 0(s0)
  bne t0, t1, user_fail

  # U2: a read-only page can be read
  li TESTNUM, 7
  li s0, VA_USER_RO
  lw t1, 0(s0)
  li t2, 0x600DF00D
  bne t1, t2, user_fail

  # U3: and a store to it is a delegated store page fault
  li TESTNUM, 8
  li s3, CAUSE_STORE_PAGE_FAULT
  li s4, VA_USER_RO
  sw t1, 0(s0)
  li t4, 2
  bne s2, t4, user_fail

  # U4: a load from the unmapped page is a delegated load page fault
  li TESTNUM, 9
  li s3, CAUSE_LOAD_PAGE_FAULT
  li s4, VA_UNMAPPED
  li s0, VA_UNMAPPED
  lw t1, 0(s0)
  li t4, 3
  bne s2, t4, user_fail

  # U5: and so is a load from a supervisor page
  li TESTNUM, 10
  li s4, VA_S_PAGE
  li s0, VA_S_PAGE
  lw t1, 0(s0)
  li t4, 4
  bne s2, t4, user_fail

  # U6: a store to a page with A = D = 0 succeeds (the hardware sets them; M-mode checks)
  li TESTNUM, 11
  li s0, VA_USER_AD
  li t0, 0x77
  sw t0, 0(s0)
  lw t1, 0(s0)
  bne t0, t1, user_fail

  # U7: a jump to a page with no execute permission is an instruction page fault, left to M-mode
  li TESTNUM, 12
  li s0, VA_USER_DATA
  jalr ra, 0(s0)
  li t4, 5
  bne s2, t4, user_fail                   # the M handler ran once and resumed here

user_pass:
  RVTEST_PASS
user_fail:
  RVTEST_FAIL

  .align 12

RVTEST_CODE_END

  .data
RVTEST_DATA_BEGIN

  TEST_DATA

  .align 12
root:       .skip 4096
l0:         .skip 4096
user_data:  .skip 4096
user_ro:    .word 0x600DF00D
            .skip 4092
user_ad:    .skip 4096

RVTEST_DATA_END
