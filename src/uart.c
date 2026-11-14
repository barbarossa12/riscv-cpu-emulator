#include "uart.h"

// The modem inputs. With nothing attached, they read as a connected modem: CTS, DSR and DCD asserted, RI not
// (decision 2026-11-14), so a terminal driver does not wait for a carrier that never comes. In loopback they
// follow the modem control outputs (datasheet 8.6.7, 8.6.8): CTS = RTS, DSR = DTR, RI = OUT1, DCD = OUT2.
static uint8_t modem_inputs(const Uart *u) {
	if (u->mcr & UART_MCR_LOOP) {
		return ((u->mcr & UART_MCR_RTS)  ? UART_MSR_CTS : 0) |
		       ((u->mcr & UART_MCR_DTR)  ? UART_MSR_DSR : 0) |
		       ((u->mcr & UART_MCR_OUT1) ? UART_MSR_RI  : 0) |
		       ((u->mcr & UART_MCR_OUT2) ? UART_MSR_DCD : 0);
	}
	return UART_MSR_CTS | UART_MSR_DSR | UART_MSR_DCD;
}

// Called when the inputs may have changed: sets the change flags (TERI is the trailing edge of RI)
static void update_modem_status(Uart *u) {
	uint8_t now = modem_inputs(u);
	uint8_t changed = now ^ u->msr_inputs;
	if (changed & UART_MSR_CTS) u->msr_delta |= UART_MSR_DCTS;
	if (changed & UART_MSR_DSR) u->msr_delta |= UART_MSR_DDSR;
	if (changed & UART_MSR_DCD) u->msr_delta |= UART_MSR_DDCD;
	if ((u->msr_inputs & UART_MSR_RI) && !(now & UART_MSR_RI)) u->msr_delta |= UART_MSR_TERI;
	u->msr_inputs = now;
}

static unsigned rx_capacity(const Uart *u) { return u->fifos_enabled ? UART_FIFO_SIZE : 1; }
static unsigned trigger_bytes(const Uart *u) { static const uint8_t t[4] = {1, 4, 8, 14}; return t[u->rx_trigger]; }

unsigned uart_rx_space(const Uart *u) { return rx_capacity(u) - u->rx_count; }

// A character arrives (from the host, or looped back from THR). Datasheet 8.6.3 (OE): in FIFO mode it is lost when the FIFO
// is full; with the FIFOs off it destroys the previous character. Either way OE is set.
bool uart_receive(Uart *u, uint8_t byte) {
	u->rx_idle = 0;                                          // a new character restarts the timeout
	if (u->rx_count == rx_capacity(u)) {
		u->rx_overrun = true;
		if (!u->fifos_enabled) u->rx_fifo[u->rx_head] = byte;
		return false;
	}
	u->rx_fifo[(u->rx_head + u->rx_count) % UART_FIFO_SIZE] = byte;
	u->rx_count++;
	return true;
}

void uart_tick(Uart *u) {
	if (u->rx_idle < u->rx_timeout) u->rx_idle++;
}

static void rx_clear(Uart *u) {
	u->rx_head = 0;
	u->rx_count = 0;
	u->rx_idle = 0;
}

// IIR bits 3:0 for the highest-priority pending interrupt, in the datasheet's order (Table 5): line status, then received
// data or timeout (the same rank), then THRE, then modem status. A disabled interrupt is not shown (8.6.6).
static uint8_t iir_cause(const Uart *u) {
	if ((u->ier & UART_IER_ELSI) && u->rx_overrun) return UART_IIR_LINE;
	if (u->ier & UART_IER_ERBFI) {
		if (u->rx_count >= (u->fifos_enabled ? trigger_bytes(u) : 1u)) return UART_IIR_RDA;
		// the timeout needs a character in the FIFO and silence since the last arrival and the last read; FIFO mode only
		if (u->fifos_enabled && u->rx_count && u->rx_idle >= u->rx_timeout) return UART_IIR_TIMEOUT;
	}
	if (u->thre_pending && (u->ier & UART_IER_ETBEI)) return UART_IIR_THRE;
	if ((u->ier & UART_IER_EDSSI) && (u->msr_delta & 0x0F)) return UART_IIR_MODEM;
	return UART_IIR_NO_INT;
}

bool uart_interrupt_pending(const Uart *u) { return !(iir_cause(u) & UART_IIR_NO_INT); }

void uart_set_tx(Uart *u, UartTxFn fn, void *ctx) { u->tx = fn; u->tx_ctx = ctx; }

void uart_set_timeout(Uart *u, uint32_t cycles) { u->rx_timeout = cycles ? cycles : UART_TIMEOUT_CYCLES; }

void uart_reset(Uart *u) {
	UartTxFn tx = u->tx;
	void *tx_ctx = u->tx_ctx;
	uint32_t timeout = u->rx_timeout;
	*u = (Uart){0};                       // IER, FCR, LCR and MCR reset to 0 (datasheet Table 3)
	u->tx = tx;                           // a reset must not mute the console
	u->tx_ctx = tx_ctx;
	uart_set_timeout(u, timeout);         // host settings survive a reset; 0 (never set) becomes the default
	u->msr_inputs = modem_inputs(u);      // no change flags after reset
}

static bool uart_read(void *ctx, uint32_t offset, uint32_t size, uint32_t *value) {
	Uart *u = ctx;
	(void)size;                           // the device is registered for byte accesses only
	bool dlab = u->lcr & UART_LCR_DLAB;
	switch (offset) {
		case UART_RBR:
			if (dlab) { *value = u->dll; return true; }
			*value = 0;                                              // empty: the datasheet does not say, 0 is our choice
			if (u->rx_count) {
				*value = u->rx_fifo[u->rx_head];
				u->rx_head = (u->rx_head + 1) % UART_FIFO_SIZE;
				u->rx_count--;
			}
			u->rx_idle = 0;                                          // a read restarts the timeout
			return true;
		case UART_IER: *value = dlab ? u->dlm : u->ier;             return true;
		case UART_IIR: {
			uint8_t cause = iir_cause(u);
			*value = cause | (u->fifos_enabled ? UART_IIR_FIFOS : 0);
			if (cause == UART_IIR_THRE) u->thre_pending = false;   // reading IIR clears it when THRE is the source
			return true;
		}
		case UART_LCR: *value = u->lcr;                             return true;
		case UART_MCR: *value = u->mcr;                             return true;
		case UART_LSR:                    // transmitter always empty; no parity, framing or break errors can happen
			*value = UART_LSR_THRE | UART_LSR_TEMT | (u->rx_count ? UART_LSR_DR : 0) | (u->rx_overrun ? UART_LSR_OE : 0);
			u->rx_overrun = false;        // reading LSR clears OE
			return true;
		case UART_MSR: *value = u->msr_inputs | u->msr_delta; u->msr_delta = 0; return true;   // reading clears the flags
		case UART_SCR: *value = u->scr;                             return true;
		default:       return false;          // offsets 8 and up: access fault
	}
}

static bool uart_write(void *ctx, uint32_t offset, uint32_t size, uint32_t value) {
	Uart *u = ctx;
	(void)size;
	uint8_t v = (uint8_t)value;
	bool dlab = u->lcr & UART_LCR_DLAB;
	switch (offset) {
		case UART_THR:
			if (dlab) { u->dll = v; return true; }
			if (u->mcr & UART_MCR_LOOP) uart_receive(u, v);                // loopback: transmitted data is received at once (8.6.7)
			else if (u->tx) u->tx(u->tx_ctx, v);
			u->thre_pending = true;           // transmission is instant, so THRE is set again at once
			return true;
		case UART_IER:
			if (dlab) { u->dlm = v; return true; }
			if (!(u->ier & UART_IER_ETBEI) && (v & UART_IER_ETBEI)) u->thre_pending = true;   // enabling it with THR empty raises it
			u->ier = v & UART_IER_MASK;
			return true;
		case UART_FCR:                    // bit 0 enables the FIFOs; the other bits are only programmed if it is 1
			if ((bool)(v & UART_FCR_ENABLE) != u->fifos_enabled) {
				u->thre_pending = true;   // datasheet: immediate after FCR0 changes
				rx_clear(u);              // and changing the mode clears the FIFOs
			}
			u->fifos_enabled = v & UART_FCR_ENABLE;
			if (u->fifos_enabled) {
				u->rx_trigger = v >> 6;
				if (v & UART_FCR_CLEAR_RX) rx_clear(u);   // the shift register (and OE) are not cleared
			}
			return true;
		case UART_LCR: u->lcr = v;                                  return true;
		case UART_MCR: u->mcr = v & UART_MCR_MASK; update_modem_status(u); return true;
		case UART_LSR:                    // read-only (a write is only for factory testing, datasheet 8.6.3)
		case UART_MSR:                    // read-only
			return true;
		case UART_SCR: u->scr = v;                                  return true;
		default:       return false;
	}
}

static void uart_tick_hook(void *ctx) { uart_tick(ctx); }

Device uart_device(Uart *u) {
	return (Device){ .base = UART_BASE, .size = UART_SIZE, .sizes = DEV_ACCESS_8,
	                 .read = uart_read, .write = uart_write, .ctx = u, .tick = uart_tick_hook };
}
