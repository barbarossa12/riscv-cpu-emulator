#include <errno.h>
#include <poll.h>
#include <unistd.h>
#include "uart_host.h"

void uart_host_tx(void *ctx, uint8_t byte) {
	HostOutput *o = ctx;
	fputc(byte, o->file);
	if (o->flush_each) fflush(o->file);
}

void host_input_init(HostInput *in, int fd) {
	in->fd = fd;
	in->eof = false;
}

void host_input_poll(HostInput *in, Uart *u) {
	if (in->fd < 0 || in->eof) return;
	unsigned room = uart_rx_space(u);
	if (room == 0) return;                      // leave the bytes in the pipe or terminal: no overrun
	struct pollfd p = { .fd = in->fd, .events = POLLIN };
	if (poll(&p, 1, 0) <= 0) return;            // nothing to read right now (timeout 0: never waits)
	uint8_t buf[UART_FIFO_SIZE];
	ssize_t n = read(in->fd, buf, room < sizeof buf ? room : sizeof buf);
	if (n == 0) in->eof = true;                 // end of input: stop polling
	for (ssize_t i = 0; i < n; i++) uart_receive(u, buf[i]);
}

bool uart_host_parse_u32(const char *s, uint32_t *out) {
	if (!*s) return false;
	uint64_t v = 0;
	for (; *s; s++) {
		if (*s < '0' || *s > '9') return false;
		v = v * 10 + (uint64_t)(*s - '0');
		if (v > 0xFFFFFFFFull) return false;
	}
	*out = (uint32_t)v;
	return true;
}
