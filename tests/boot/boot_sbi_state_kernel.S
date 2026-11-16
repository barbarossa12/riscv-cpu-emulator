# boot_sbi_state_kernel.S: what S-mode sees of the machine after OpenSBI set it up: the counters (mcounteren is all ones), the
# delegated interrupt bits in sie and sip, and instructions that must not trap in S-mode.
#include "sbi_common.h"

  .section .text
  .globl _start
_start:
  # the counters count, and time moves forward
  csrr s1, cycle
  csrr s2, instret
  csrr s3, time
  nop
  nop
  csrr t0, cycle
  csrr t1, instret
  csrr t2, time
  bgtu s1, t0, bad_cycle
  beq s1, t0, bad_cycle                        # cycle moved
  bgtu s2, t1, bad_instret
  beq s2, t1, bad_instret                      # instret moved
  bgtu s3, t2, bad_time                        # time did not go back
  csrr t0, timeh
  CHECK_EQ t0, 0, 4

  # sie holds the three delegated bits: SSIE, STIE and SEIE
  li t0, -1
  csrw sie, t0
  csrr t1, sie
  andi t1, t1, 0x222
  CHECK_EQ t1, 0x222, 5
  csrw sie, zero
  csrr t1, sie
  CHECK_EQ t1, 0, 6

  # sip: only SSIP can be written from S-mode. STIP and SEIP are set by the firmware and the platform, so a write must leave them as
  # they were (whatever they were) and set SSIP
  csrr t2, sip
  andi t2, t2, 0x220                           # STIP and SEIP before
  li t0, -1
  csrw sip, t0
  csrr t1, sip
  andi t1, t1, 0x222
  ori t2, t2, 0x002                            # expected: the same STIP and SEIP, and SSIP now set
  CHECK_EQ_REG t1, t2, 7
  li t0, SIP_SSIP
  csrc sip, t0
  csrr t1, sip
  andi t1, t1, SIP_SSIP
  CHECK_EQ t1, 0, 8                            # SSIP is writable both ways

  # these must run without trapping in S-mode (a trap would reach OpenSBI and end in a failure)
  sfence.vma zero, zero
  fence.i
  fence
  csrr t0, sstatus
  csrr t0, scounteren
  csrr t0, stvec

  PUTS "sbi state ok\n"
  SHUTDOWN

bad_cycle:
  FAIL_WITH 1
bad_instret:
  FAIL_WITH 2
bad_time:
  FAIL_WITH 3
