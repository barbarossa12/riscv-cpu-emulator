#include <fcntl.h>
#include <unistd.h>
#include "unity.h"
#include "../src/mem.h"
#include "../src/uart.h"
#include "../src/uart_host.h"

#define RAM_BASE 0x80000000u

static Memory mem;
static Uart uart;
static HostInput in;
static int pipe_in[2];            // the host's stdin stand-in: tests write pipe_in[1], the poll reads pipe_in[0]

void setUp(void) {
    mem_init_at(&mem, RAM_BASE, 0x1000);
    uart_reset(&uart);
    mem_add_device(&mem, uart_device(&uart));
    TEST_ASSERT_EQUAL_INT(0, pipe(pipe_in));
    host_input_init(&in, pipe_in[0]);
}

void tearDown(void) {
    if (pipe_in[0] >= 0) close(pipe_in[0]);
    if (pipe_in[1] >= 0) close(pipe_in[1]);
    pipe_in[0] = pipe_in[1] = -1;
    mem_free(&mem);
}

static uint32_t reg(uint32_t offset) {
    uint32_t v = 0xDEADBEEF;
    TEST_ASSERT_TRUE(mem_bus_read(&mem, UART_BASE + offset, 1, &v));
    return v;
}

static void put(uint32_t offset, uint32_t v) {
    TEST_ASSERT_TRUE(mem_bus_write(&mem, UART_BASE + offset, 1, v));
}

static void host_writes(unsigned n, uint8_t first) {
    for (unsigned i = 0; i < n; i++) {
        uint8_t b = (uint8_t)(first + i);
        TEST_ASSERT_EQUAL_INT(1, (int)write(pipe_in[1], &b, 1));
    }
}

/* ---- Output ------------------------------------------------------------------------------------ */

void test_l1_bytes_reach_the_stream_in_order(void) {
    FILE *f = tmpfile();
    TEST_ASSERT_NOT_NULL(f);
    HostOutput out = { f, false };
    for (unsigned i = 0; i < 20; i++) uart_host_tx(&out, (uint8_t)(0x30 + i));
    fflush(f);
    rewind(f);
    for (unsigned i = 0; i < 20; i++) TEST_ASSERT_EQUAL_INT(0x30 + (int)i, fgetc(f));
    TEST_ASSERT_EQUAL_INT(EOF, fgetc(f));
    fclose(f);
}

// A fully buffered stream on a pipe, so only an explicit flush makes a byte visible on the read end
static void with_buffered_pipe(bool flush_each, int *visible_before, int *visible_after) {
    int p[2];
    TEST_ASSERT_EQUAL_INT(0, pipe(p));
    fcntl(p[0], F_SETFL, O_NONBLOCK);
    FILE *w = fdopen(p[1], "w");
    TEST_ASSERT_NOT_NULL(w);
    setvbuf(w, NULL, _IOFBF, 4096);
    HostOutput out = { w, flush_each };
    uart_host_tx(&out, 'x');
    uint8_t b;
    *visible_before = (int)read(p[0], &b, 1);
    fflush(w);
    *visible_after = (*visible_before == 1) ? 1 : (int)read(p[0], &b, 1);
    fclose(w);
    close(p[0]);
}

void test_l2_flush_each_makes_the_byte_visible_at_once(void) {
    int before, after;
    with_buffered_pipe(true, &before, &after);
    TEST_ASSERT_EQUAL_INT(1, before);
}

void test_l3_without_flush_each_the_byte_waits_for_a_flush(void) {
    int before, after;
    with_buffered_pipe(false, &before, &after);
    TEST_ASSERT_EQUAL_INT(-1, before);                       // nothing yet (would block)
    TEST_ASSERT_EQUAL_INT(1, after);
}

void test_l4_a_thr_write_through_the_bus_reaches_the_stream(void) {
    FILE *f = tmpfile();
    HostOutput out = { f, false };
    uart_set_tx(&uart, uart_host_tx, &out);
    put(UART_THR, 'H');
    put(UART_THR, 'i');
    fflush(f);
    rewind(f);
    TEST_ASSERT_EQUAL_INT('H', fgetc(f));
    TEST_ASSERT_EQUAL_INT('i', fgetc(f));
    TEST_ASSERT_EQUAL_INT(EOF, fgetc(f));
    fclose(f);
}

/* ---- Input ------------------------------------------------------------------------------------- */

void test_l5_an_empty_pipe_does_not_block(void) {
    host_input_poll(&in, &uart);                             // the read end is blocking: a read here would hang
    TEST_ASSERT_EQUAL_UINT(0, uart.rx_count);
    TEST_ASSERT_FALSE(in.eof);
}

void test_l6_bytes_arrive_in_order(void) {
    put(UART_FCR, 0x01);
    host_writes(3, 0x41);
    host_input_poll(&in, &uart);
    TEST_ASSERT_EQUAL_UINT(3, uart.rx_count);
    TEST_ASSERT_EQUAL_HEX32(0x41, reg(UART_RBR));
    TEST_ASSERT_EQUAL_HEX32(0x42, reg(UART_RBR));
    TEST_ASSERT_EQUAL_HEX32(0x43, reg(UART_RBR));
}

void test_l7_the_poll_takes_only_what_fits(void) {
    put(UART_FCR, 0x01);
    host_writes(20, 0);
    host_input_poll(&in, &uart);
    TEST_ASSERT_EQUAL_UINT(16, uart.rx_count);
    TEST_ASSERT_EQUAL_HEX32(0, reg(UART_LSR) & UART_LSR_OE);
    for (unsigned i = 0; i < 4; i++) TEST_ASSERT_EQUAL_HEX32(i, reg(UART_RBR));
    host_input_poll(&in, &uart);                             // the last four were waiting in the pipe
    TEST_ASSERT_EQUAL_UINT(16, uart.rx_count);
    for (unsigned i = 4; i < 20; i++) TEST_ASSERT_EQUAL_HEX32(i, reg(UART_RBR));
    TEST_ASSERT_EQUAL_HEX32(0x60, reg(UART_LSR));            // no byte lost, no overrun
}

void test_l8_with_the_fifos_off_one_byte_per_poll(void) {
    host_writes(3, 0x10);
    host_input_poll(&in, &uart);
    host_input_poll(&in, &uart);                             // the holding register is full: nothing is read
    TEST_ASSERT_EQUAL_UINT(1, uart.rx_count);
    TEST_ASSERT_EQUAL_HEX32(0x10, reg(UART_RBR));
    host_input_poll(&in, &uart);
    TEST_ASSERT_EQUAL_HEX32(0x11, reg(UART_RBR));
    host_input_poll(&in, &uart);
    TEST_ASSERT_EQUAL_HEX32(0x12, reg(UART_RBR));
    TEST_ASSERT_EQUAL_HEX32(0x60, reg(UART_LSR));            // never an overrun
}

void test_l9_end_of_input(void) {
    close(pipe_in[1]);
    pipe_in[1] = -1;
    host_input_poll(&in, &uart);
    TEST_ASSERT_TRUE(in.eof);
    TEST_ASSERT_EQUAL_UINT(0, uart.rx_count);
}

void test_l9b_nothing_is_read_after_end_of_input(void) {
    in.eof = true;                                           // the input has ended...
    host_writes(1, 0x41);                                    // ...so even data that shows up later is left alone
    host_input_poll(&in, &uart);
    TEST_ASSERT_EQUAL_UINT(0, uart.rx_count);
}

void test_l10_data_comes_before_end_of_input(void) {
    put(UART_FCR, 0x01);
    host_writes(2, 0x61);
    close(pipe_in[1]);
    pipe_in[1] = -1;
    host_input_poll(&in, &uart);
    TEST_ASSERT_EQUAL_UINT(2, uart.rx_count);
    TEST_ASSERT_FALSE(in.eof);
    host_input_poll(&in, &uart);
    TEST_ASSERT_TRUE(in.eof);
    TEST_ASSERT_EQUAL_HEX32(0x61, reg(UART_RBR));
    TEST_ASSERT_EQUAL_HEX32(0x62, reg(UART_RBR));
}

void test_l11_a_disabled_input_does_nothing(void) {
    HostInput off;
    host_input_init(&off, -1);
    host_writes(1, 0x41);
    host_input_poll(&off, &uart);
    TEST_ASSERT_EQUAL_UINT(0, uart.rx_count);
    TEST_ASSERT_FALSE(off.eof);
}

void test_l12_a_delivered_byte_raises_the_line(void) {
    put(UART_IER, UART_IER_ERBFI);
    TEST_ASSERT_FALSE(uart_interrupt_pending(&uart));
    host_writes(1, 0x41);
    host_input_poll(&in, &uart);
    TEST_ASSERT_TRUE(uart_interrupt_pending(&uart));
}

void test_l13_the_poll_leaves_the_registers_alone(void) {
    put(UART_IER, 0x05);
    put(UART_LCR, 0x03);
    put(UART_MCR, 0x03);
    put(UART_SCR, 0x77);
    host_writes(1, 0x41);
    host_input_poll(&in, &uart);
    TEST_ASSERT_EQUAL_HEX32(0x05, reg(UART_IER));
    TEST_ASSERT_EQUAL_HEX32(0x03, reg(UART_LCR));
    TEST_ASSERT_EQUAL_HEX32(0x03, reg(UART_MCR));
    TEST_ASSERT_EQUAL_HEX32(0x77, reg(UART_SCR));
}

/* ---- The environment option parser ----------------------------------------------------------------- */

void test_l14_parse_u32(void) {
    uint32_t v = 123;
    TEST_ASSERT_TRUE(uart_host_parse_u32("100", &v));        TEST_ASSERT_EQUAL_UINT32(100, v);
    TEST_ASSERT_TRUE(uart_host_parse_u32("0", &v));          TEST_ASSERT_EQUAL_UINT32(0, v);
    TEST_ASSERT_TRUE(uart_host_parse_u32("4294967295", &v)); TEST_ASSERT_EQUAL_UINT32(0xFFFFFFFFu, v);
    v = 123;
    const char *bad[] = { "", "abc", "-5", "+5", "10x", " 7", "7 ", "4294967296", "99999999999999999999" };
    for (unsigned i = 0; i < sizeof bad / sizeof bad[0]; i++) {
        TEST_ASSERT_FALSE_MESSAGE(uart_host_parse_u32(bad[i], &v), bad[i]);
        TEST_ASSERT_EQUAL_UINT32(123, v);                    // a rejected value leaves the output alone
    }
}

void test_l7b_the_poll_returns_how_many_bytes_it_delivered(void) {
    put(UART_FCR, 0x01);
    TEST_ASSERT_EQUAL_UINT(0, host_input_poll(&in, &uart));                  // nothing waiting
    host_writes(3, 0x41);
    TEST_ASSERT_EQUAL_UINT(3, host_input_poll(&in, &uart));
    TEST_ASSERT_EQUAL_UINT(0, host_input_poll(&in, &uart));                  // already taken
    host_writes(20, 0);
    TEST_ASSERT_EQUAL_UINT(13, host_input_poll(&in, &uart));                 // only what fits: 16 - 3
    HostInput off;
    host_input_init(&off, -1);
    TEST_ASSERT_EQUAL_UINT(0, host_input_poll(&off, &uart));                 // disabled
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_l1_bytes_reach_the_stream_in_order);
    RUN_TEST(test_l2_flush_each_makes_the_byte_visible_at_once);
    RUN_TEST(test_l3_without_flush_each_the_byte_waits_for_a_flush);
    RUN_TEST(test_l4_a_thr_write_through_the_bus_reaches_the_stream);
    RUN_TEST(test_l5_an_empty_pipe_does_not_block);
    RUN_TEST(test_l6_bytes_arrive_in_order);
    RUN_TEST(test_l7_the_poll_takes_only_what_fits);
    RUN_TEST(test_l8_with_the_fifos_off_one_byte_per_poll);
    RUN_TEST(test_l9_end_of_input);
    RUN_TEST(test_l9b_nothing_is_read_after_end_of_input);
    RUN_TEST(test_l10_data_comes_before_end_of_input);
    RUN_TEST(test_l11_a_disabled_input_does_nothing);
    RUN_TEST(test_l12_a_delivered_byte_raises_the_line);
    RUN_TEST(test_l13_the_poll_leaves_the_registers_alone);
    RUN_TEST(test_l14_parse_u32);
    RUN_TEST(test_l7b_the_poll_returns_how_many_bytes_it_delivered);
    return UNITY_END();
}
