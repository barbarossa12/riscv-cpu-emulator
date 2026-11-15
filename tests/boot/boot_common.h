// Shared by the boot stubs: the finisher and the boot contract of src/machine.h, written out as numbers because the assembler cannot
// read the C header. If the machine changes, these stubs fail (that is the point of running them).
#define FINISHER        0x00100000
#define FINISHER_PASS   0x5555
#define FINISHER_FAIL   0x3333
#define FINISHER_RESET  0x7777
#define DTB_ADDR        0x87C00000     // MACHINE_DTB_ADDR
#define KERNEL_ADDR     0x80400000     // MACHINE_KERNEL_ADDR

  .macro FINISH value                   # write a finisher value and stop
  li t0, FINISHER
  li t1, \value
  sw t1, 0(t0)
1:
  j 1b
  .endm

  .macro FAIL_WITH code                 # failure, with the code as the exit status
  li t0, FINISHER
  li t1, (\code << 16) | FINISHER_FAIL
  sw t1, 0(t0)
1:
  j 1b
  .endm

  .macro CHECK_EQ reg, val, code        # reg must equal val, otherwise fail with code
  li t6, \val
  beq \reg, t6, 2f
  FAIL_WITH \code
2:
  .endm
