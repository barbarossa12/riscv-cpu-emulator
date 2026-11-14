#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "mem.h"

// A 16550-style UART (the PC16550D, which Linux and OpenSBI drive as "ns16550a"), memory-mapped like QEMU's virt
// machine: byte-wide registers one byte apart (reg-shift 0, reg-io-width 1). The ISA manual does not define it; the
// register map, reset values and bit meanings are the TI PC16550D datasheet (SNLS378C, Tables 1 to 3 and section 8.6).
// Offsets 8 and up are an access fault (decision 2026-11-14).
#define UART_BASE 0x10000000u
#define UART_SIZE 0x100u

// Register offsets. Offsets 0 and 1 mean different things with the divisor latch access bit (LCR bit 7).
#define UART_RBR 0   // receive buffer (read), DLAB = 0
#define UART_THR 0   // transmit holding (write), DLAB = 0
#define UART_DLL 0   // divisor latch low, DLAB = 1
#define UART_IER 1   // interrupt enable, DLAB = 0
#define UART_DLM 1   // divisor latch high, DLAB = 1
#define UART_IIR 2   // interrupt identification (read)
#define UART_FCR 2   // FIFO control (write)
#define UART_LCR 3
#define UART_MCR 4
#define UART_LSR 5
#define UART_MSR 6
#define UART_SCR 7

#define UART_IER_MASK     0x0Fu   // bits 7:4 always read 0
#define UART_IER_ERBFI    0x01u   // received data available (and timeout) interrupt enable
#define UART_IER_ETBEI    0x02u   // transmitter holding register empty interrupt enable
#define UART_IER_ELSI     0x04u   // receiver line status interrupt enable
#define UART_IER_EDSSI    0x08u   // modem status interrupt enable
#define UART_LCR_DLAB     0x80u
#define UART_MCR_DTR      0x01u
#define UART_MCR_RTS      0x02u
#define UART_MCR_OUT1     0x04u
#define UART_MCR_OUT2     0x08u
#define UART_MCR_LOOP     0x10u
#define UART_MCR_MASK     0x1Fu   // bits 7:5 always read 0
#define UART_FCR_ENABLE   0x01u
#define UART_FCR_CLEAR_RX 0x02u
#define UART_FCR_CLEAR_TX 0x04u
#define UART_IIR_NO_INT   0x01u
#define UART_IIR_THRE     0x02u   // cause: transmitter holding register empty
#define UART_IIR_RDA      0x04u   // cause: received data available (trigger level reached)
#define UART_IIR_LINE     0x06u   // cause: receiver line status
#define UART_IIR_TIMEOUT  0x0Cu   // cause: character timeout
#define UART_IIR_MODEM    0x00u   // cause: modem status
#define UART_LSR_OE       0x02u

#define UART_FIFO_SIZE    16
// How long a character may sit unread with nothing arriving before the timeout interrupt, in cpu steps. The datasheet says
// 4 character times; with the baud rate ignored this is 4 characters of 10 bits at 115200 baud on the nominal 100 MHz
// clock. A default: the host can change it with uart_set_timeout.
#define UART_TIMEOUT_CYCLES 35000u
#define UART_IIR_FIFOS    0xC0u   // bits 7:6 read 1 while the FIFOs are enabled
#define UART_LSR_DR       0x01u
#define UART_LSR_THRE     0x20u
#define UART_LSR_TEMT     0x40u
#define UART_MSR_DCTS     0x01u   // delta bits, cleared by reading MSR
#define UART_MSR_DDSR     0x02u
#define UART_MSR_TERI     0x04u
#define UART_MSR_DDCD     0x08u
#define UART_MSR_CTS      0x10u
#define UART_MSR_DSR      0x20u
#define UART_MSR_RI       0x40u
#define UART_MSR_DCD      0x80u

// Called with each byte the program transmits (not in loopback)
typedef void (*UartTxFn)(void *ctx, uint8_t byte);

typedef struct Uart {
	uint8_t ier;
	uint8_t lcr;
	uint8_t mcr;
	uint8_t scr;
	uint8_t dll;
	uint8_t dlm;
	bool    fifos_enabled;   // FCR bit 0
	uint8_t rx_trigger;      // FCR bits 7:6: 0, 1, 2, 3 for 1, 4, 8, 14 bytes
	uint8_t msr_inputs;      // the status bits 7:4 as last seen
	uint8_t msr_delta;       // the change flags, bits 3:0
	bool    thre_pending;    // the THRE interrupt condition (latched; shown in IIR only while ETBEI is set)
	uint8_t  rx_fifo[UART_FIFO_SIZE];   // received characters (capacity 1 with the FIFOs off)
	uint8_t  rx_head, rx_count;
	bool     rx_overrun;     // LSR.OE, cleared by reading LSR
	uint32_t rx_idle;        // steps since a character arrived or was read; stops at rx_timeout
	uint32_t rx_timeout;     // steps before the timeout interrupt (0 is replaced by the default)
	UartTxFn tx;             // host output for transmitted bytes; NULL drops them
	void   *tx_ctx;
} Uart;

void uart_reset(Uart *u);                      // keeps the host output hook
void uart_set_tx(Uart *u, UartTxFn fn, void *ctx);
void uart_set_timeout(Uart *u, uint32_t cycles);   // 0 restores the default; survives uart_reset
bool uart_receive(Uart *u, uint8_t byte);          // a character arrives; false if it was lost to an overrun
unsigned uart_rx_space(const Uart *u);             // how many more characters fit
void uart_tick(Uart *u);                           // one cpu step (the device tick hook)
bool uart_interrupt_pending(const Uart *u);    // the interrupt output line (THRE only until the receive side exists)
Device uart_device(Uart *u);   // for mem_add_device
