// Shared by the UART assembly tests: register addresses and small helpers. They clobber t0 to t3 and jump to `fail`.
#define UART        0x10000000
#define UART_THR    0
#define UART_RBR    0
#define UART_DLL    0
#define UART_IER    1
#define UART_DLM    1
#define UART_IIR    2
#define UART_FCR    2
#define UART_LCR    3
#define UART_MCR    4
#define UART_LSR    5
#define UART_MSR    6
#define UART_SCR    7

  .macro WR off, val                     # write a byte to a UART register
  li t0, UART
  li t1, \val
  sb t1, \off(t0)
  .endm

  .macro RD reg, off                     # read a byte from a UART register
  li t0, UART
  lbu \reg, \off(t0)
  .endm

  .macro EXPECT off, mask, val           # (register & mask) must equal val
  RD t2, \off
  andi t2, t2, \mask
  li t3, \val
  bne t2, t3, fail
  .endm
