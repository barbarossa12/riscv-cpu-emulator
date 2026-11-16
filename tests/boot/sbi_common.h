// Shared by the SBI service stubs (boot_sbi_*_kernel.S): the SBI calling convention and the extension numbers from OpenSBI's
// include/sbi/sbi_ecall_interface.h. A call takes the extension in a7, the function in a6 and arguments in a0 to a5, and returns an
// error code in a0 and a value in a1; every other register is preserved. Each stub is the next stage after OpenSBI's fw_jump
// (linked at KERNEL_ADDR), runs in S-mode, checks one area of the SBI with its own exit code per check, and ends with a shutdown.
// The same stubs run unchanged on QEMU's virt machine with the same fw_jump, as an oracle for what the calls must do.
#include "boot_common.h"

#define SBI_EXT_0_1_SET_TIMER      0x0
#define SBI_EXT_0_1_PUTCHAR        0x1
#define SBI_EXT_BASE               0x10
#define SBI_EXT_TIME               0x54494D45
#define SBI_EXT_IPI                0x735049
#define SBI_EXT_RFENCE             0x52464E43
#define SBI_EXT_HSM                0x48534D
#define SBI_EXT_SRST               0x53525354
#define SBI_EXT_DBCN               0x4442434E

#define SBI_ERR_NOT_SUPPORTED      -2
#define SBI_ERR_INVALID_PARAM      -3
#define SBI_ERR_ALREADY_AVAILABLE  -6

// CSR bits
#define SSTATUS_SIE   0x2
#define SIE_SSIE      0x2
#define SIE_STIE      0x20
#define SIP_SSIP      0x2
#define SIP_STIP      0x20

  .macro SBI_CALL eid, fid
  li a7, \eid
  li a6, \fid
  ecall
  .endm

  .macro CHECK_NZ reg, code             # reg must not be 0
  bnez \reg, 2f
  FAIL_WITH \code
2:
  .endm

  .macro CHECK_EQ_REG reg1, reg2, code  # two registers must be equal
  beq \reg1, \reg2, 2f
  FAIL_WITH \code
2:
  .endm

  .macro CHECK_RET err, val, code       # after an SBI call: a0 = err and a1 = val
  CHECK_EQ a0, \err, \code
  CHECK_EQ a1, \val, \code + 1
  .endm

  .macro SET_STVEC label
  la t0, \label
  csrw stvec, t0
  .endm

  .macro PUTS text                      # write a string through the debug console, checking that all of it was taken
  j .Lskip\@
.Lstr\@:
  .ascii "\text"
.Lend\@:
  .align 2
.Llen\@:
  .word .Lend\@ - .Lstr\@              # the length, as data: the assembler cannot use a label difference as an immediate
.Lskip\@:
  lw a0, .Llen\@
  la a1, .Lstr\@
  li a2, 0
  SBI_CALL SBI_EXT_DBCN, 0
  CHECK_EQ a0, 0, 90
  lw t5, .Llen\@
  beq a1, t5, .Lok\@
  FAIL_WITH 91                          # not all the bytes were taken
.Lok\@:
  .endm

  .macro SHUTDOWN                       # the stub passed: ask OpenSBI to power off, which ends the run with exit status 0
  li a7, SBI_EXT_SRST
  li a6, 0
  li a0, 0
  li a1, 0
  ecall
  FAIL_WITH 99                          # the call must not return
  .endm
