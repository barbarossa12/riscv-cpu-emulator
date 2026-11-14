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

void uart_reset(Uart *u) {
	*u = (Uart){0};                       // IER, FCR, LCR and MCR reset to 0 (datasheet Table 3)
	u->msr_inputs = modem_inputs(u);      // no change flags after reset
}

static bool uart_read(void *ctx, uint32_t offset, uint32_t size, uint32_t *value) {
	Uart *u = ctx;
	(void)size;                           // the device is registered for byte accesses only
	bool dlab = u->lcr & UART_LCR_DLAB;
	switch (offset) {
		case UART_RBR: *value = dlab ? u->dll : 0;                  return true;   // no data path yet
		case UART_IER: *value = dlab ? u->dlm : u->ier;             return true;
		case UART_IIR: *value = UART_IIR_NO_INT | (u->fifos_enabled ? UART_IIR_FIFOS : 0); return true;
		case UART_LCR: *value = u->lcr;                             return true;
		case UART_MCR: *value = u->mcr;                             return true;
		case UART_LSR: *value = UART_LSR_THRE | UART_LSR_TEMT;      return true;   // transmitter empty, nothing received
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
		case UART_THR:                                              // no data path yet: the byte is dropped
			if (dlab) u->dll = v;
			return true;
		case UART_IER:
			if (dlab) u->dlm = v; else u->ier = v & UART_IER_MASK;
			return true;
		case UART_FCR:                    // bit 0 enables the FIFOs; the other bits are only programmed if it is 1
			u->fifos_enabled = v & UART_FCR_ENABLE;
			if (u->fifos_enabled) {
				u->rx_trigger = v >> 6;   // bits 1 and 2 (clear the FIFOs) have nothing to clear yet
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

Device uart_device(Uart *u) {
	return (Device){ .base = UART_BASE, .size = UART_SIZE, .sizes = DEV_ACCESS_8,
	                 .read = uart_read, .write = uart_write, .ctx = u };
}
