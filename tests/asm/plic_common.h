// Shared by the PLIC assembly tests: register addresses and small helpers. The definitions have no spaces on purpose, so
// they can be passed as macro arguments. The macros clobber t0 to t3 and jump to `fail`; they are for code that runs with
// interrupts off, because a handler may use the same registers.
#define PLIC              0x0C000000
#define PLIC_PRIORITY(n)  (0x0C000000+4*(n))
#define PLIC_PENDING      (0x0C000000+0x1000)
#define PLIC_ENABLE(c)    (0x0C000000+0x2000+0x80*(c))
#define PLIC_THRESHOLD(c) (0x0C000000+0x200000+0x1000*(c))
#define PLIC_CLAIM(c)     (0x0C000000+0x200000+0x1000*(c)+4)

#define MEIP 0x800
#define SEIP 0x200
#define SSIP 0x002

  .macro PW addr, val                    # sw val to addr
  li t0, \addr
  li t1, \val
  sw t1, 0(t0)
  .endm

  .macro PEXPECT addr, val               # the word at addr must equal val
  li t0, \addr
  lw t2, 0(t0)
  li t3, \val
  bne t2, t3, fail
  .endm

  .macro MIPEXPECT mask, val             # (mip & mask) must equal val
  csrr t2, mip
  li t3, \mask
  and t2, t2, t3
  li t3, \val
  bne t2, t3, fail
  .endm
