#pragma once
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "uart.h"

// Host side of the UART: where transmitted bytes go and where received bytes come from. Platform I/O, so neither the
// ISA manual nor the 16550 datasheet says anything about it.

// Output: a UartTxFn (pass uart_host_tx to uart_set_tx, with a HostOutput as the context).
typedef struct {
	FILE *file;
	bool  flush_each;   // flush after every byte (wanted on a terminal, so prompts without a newline show up)
} HostOutput;

void uart_host_tx(void *ctx, uint8_t byte);

// Input: a non-blocking feed from a file descriptor into the receive FIFO. fd < 0 means disabled.
typedef struct {
	int  fd;
	bool eof;           // the descriptor reached end of input; it is not read again
} HostInput;

void host_input_init(HostInput *in, int fd);
// Moves at most uart_rx_space() bytes, so the host never causes an overrun, and never waits for data.
unsigned host_input_poll(HostInput *in, Uart *u);   // returns how many bytes it delivered

// Strict unsigned decimal parse for the environment options: digits only, no sign, no spaces, must fit 32 bits.
bool uart_host_parse_u32(const char *s, uint32_t *out);
