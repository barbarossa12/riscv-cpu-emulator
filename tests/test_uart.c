#include "unity.h"
#include "../src/mem.h"
#include "../src/uart.h"
#include "cpu_test_util.h"
#include "../src/execute.h"

#define RAM_BASE 0x80000000u

static Memory mem;
static Uart uart;

void setUp(void) {
    mem_init_at(&mem, RAM_BASE, 0x1000);
    uart_reset(&uart);
    mem_add_device(&mem, uart_device(&uart));
}

void tearDown(void) {
    mem_free(&mem);
}

static bool rd(uint32_t offset, uint32_t size, uint32_t *v) {
    return mem_bus_read(&mem, UART_BASE + offset, size, v);
}

static bool wr(uint32_t offset, uint32_t size, uint32_t v) {
    return mem_bus_write(&mem, UART_BASE + offset, size, v);
}

static uint32_t reg(uint32_t offset) {
    uint32_t v = 0xDEADBEEF;
    TEST_ASSERT_TRUE(rd(offset, 1, &v));
    return v;
}

static void put(uint32_t offset, uint32_t v) {
    TEST_ASSERT_TRUE(wr(offset, 1, v));
}

/* ---- A. Reset values (datasheet Table 3) ------------------------------------------------------- */

void test_a1_reset_values(void) {
    TEST_ASSERT_EQUAL_HEX32(0, reg(UART_IER));
    TEST_ASSERT_EQUAL_HEX32(0, reg(UART_LCR));
    TEST_ASSERT_EQUAL_HEX32(0, reg(UART_MCR));
    TEST_ASSERT_FALSE(uart.fifos_enabled);
    TEST_ASSERT_EQUAL_UINT8(0, uart.rx_trigger);
}

void test_a2_iir(void)  { TEST_ASSERT_EQUAL_HEX32(0x01, reg(UART_IIR)); }
void test_a3_lsr(void)  { TEST_ASSERT_EQUAL_HEX32(0x60, reg(UART_LSR)); }
void test_a4_msr(void)  {
    TEST_ASSERT_EQUAL_HEX32(0xB0, reg(UART_MSR));
    TEST_ASSERT_EQUAL_HEX32(0xB0, reg(UART_MSR));       // no change flags after reset
}
void test_a5_others(void) {
    TEST_ASSERT_EQUAL_HEX32(0, reg(UART_SCR));
    TEST_ASSERT_EQUAL_HEX32(0, reg(UART_RBR));
    put(UART_LCR, UART_LCR_DLAB);
    TEST_ASSERT_EQUAL_HEX32(0, reg(UART_DLL));
    TEST_ASSERT_EQUAL_HEX32(0, reg(UART_DLM));
}

/* ---- B. Plain registers ------------------------------------------------------------------------ */

void test_b1_ier(void) {
    put(UART_IER, 0xFF);
    TEST_ASSERT_EQUAL_HEX32(0x0F, reg(UART_IER));
    for (int bit = 0; bit < 8; bit++) {
        put(UART_IER, 1u << bit);
        TEST_ASSERT_EQUAL_HEX32(bit < 4 ? (1u << bit) : 0, reg(UART_IER));
    }
}

void test_b2_lcr(void) {
    put(UART_LCR, 0xFF);
    TEST_ASSERT_EQUAL_HEX32(0xFF, reg(UART_LCR));
    put(UART_LCR, 0x03);
    TEST_ASSERT_EQUAL_HEX32(0x03, reg(UART_LCR));
}

void test_b3_mcr(void) {
    put(UART_MCR, 0xFF);
    TEST_ASSERT_EQUAL_HEX32(0x1F, reg(UART_MCR));
    put(UART_MCR, 0x00);
    TEST_ASSERT_EQUAL_HEX32(0x00, reg(UART_MCR));
}

void test_b4_scr(void) {
    put(UART_SCR, 0xA5);
    TEST_ASSERT_EQUAL_HEX32(0xA5, reg(UART_SCR));
    put(UART_SCR, 0x5A);
    TEST_ASSERT_EQUAL_HEX32(0x5A, reg(UART_SCR));
}

/* ---- C. DLAB ----------------------------------------------------------------------------------- */

void test_c1_divisor_latches(void) {
    put(UART_LCR, UART_LCR_DLAB);
    put(UART_DLL, 0x34);
    put(UART_DLM, 0x12);
    TEST_ASSERT_EQUAL_HEX32(0x34, reg(UART_DLL));
    TEST_ASSERT_EQUAL_HEX32(0x12, reg(UART_DLM));
}

void test_c2_dlab_clear_shows_rbr_and_ier(void) {
    put(UART_IER, 0x05);
    put(UART_LCR, UART_LCR_DLAB);
    put(UART_DLL, 0x34);
    put(UART_DLM, 0x12);
    put(UART_LCR, 0x03);
    TEST_ASSERT_EQUAL_HEX32(0, reg(UART_RBR));
    TEST_ASSERT_EQUAL_HEX32(0x05, reg(UART_IER));
}

void test_c3_divisor_write_touches_nothing_else(void) {
    put(UART_IER, 0x05);
    put(UART_LCR, UART_LCR_DLAB);
    put(UART_DLL, 0xFF);
    put(UART_DLM, 0xFF);
    put(UART_LCR, 0);
    TEST_ASSERT_EQUAL_HEX32(0x05, reg(UART_IER));
    TEST_ASSERT_EQUAL_HEX32(0x60, reg(UART_LSR));
}

void test_c4_ier_survives_dlab(void) {
    put(UART_IER, 0x05);
    put(UART_LCR, UART_LCR_DLAB);
    put(UART_LCR, 0);
    TEST_ASSERT_EQUAL_HEX32(0x05, reg(UART_IER));
}

void test_c5_dlab_does_not_affect_other_offsets(void) {
    put(UART_MCR, 0x03);
    put(UART_SCR, 0x77);
    uint32_t before[8];
    for (int o = UART_IIR; o <= UART_SCR; o++) {
        if (o != UART_MSR) before[o] = reg(o);       // reading MSR clears flags, leave it out
    }
    put(UART_LCR, UART_LCR_DLAB);
    TEST_ASSERT_EQUAL_HEX32(before[UART_IIR], reg(UART_IIR));
    TEST_ASSERT_EQUAL_HEX32(UART_LCR_DLAB, reg(UART_LCR));
    TEST_ASSERT_EQUAL_HEX32(0x03, reg(UART_MCR));
    TEST_ASSERT_EQUAL_HEX32(0x60, reg(UART_LSR));
    TEST_ASSERT_EQUAL_HEX32(0x77, reg(UART_SCR));
}

/* ---- D. FIFO control and IIR ------------------------------------------------------------------- */

void test_d1_fifo_enable_shows_in_iir(void) {
    put(UART_FCR, 0x01);
    TEST_ASSERT_EQUAL_HEX32(0xC1, reg(UART_IIR));
}

void test_d2_fifo_disable(void) {
    put(UART_FCR, 0x01);
    put(UART_FCR, 0x00);
    TEST_ASSERT_EQUAL_HEX32(0x01, reg(UART_IIR));
}

void test_d3_trigger_not_programmed_without_enable(void) {
    put(UART_FCR, 0x81);
    TEST_ASSERT_EQUAL_UINT8(2, uart.rx_trigger);
    put(UART_FCR, 0xC0);
    TEST_ASSERT_EQUAL_HEX32(0x01, reg(UART_IIR));
    TEST_ASSERT_EQUAL_UINT8(2, uart.rx_trigger);
}

void test_d4_trigger_levels(void) {
    put(UART_FCR, 0x81); TEST_ASSERT_EQUAL_UINT8(2, uart.rx_trigger);
    put(UART_FCR, 0x41); TEST_ASSERT_EQUAL_UINT8(1, uart.rx_trigger);
    put(UART_FCR, 0xC1); TEST_ASSERT_EQUAL_UINT8(3, uart.rx_trigger);
    put(UART_FCR, 0x01); TEST_ASSERT_EQUAL_UINT8(0, uart.rx_trigger);
}

void test_d5_clear_bits_are_accepted(void) {
    put(UART_FCR, 0x07);
    TEST_ASSERT_EQUAL_HEX32(0xC1, reg(UART_IIR));
}

void test_d6_ier_without_etbei_raises_nothing(void) {
    put(UART_IER, 0x0D);                       // everything but the THRE interrupt
    TEST_ASSERT_EQUAL_HEX32(0x01, reg(UART_IIR));
    put(UART_FCR, 0x01);
    TEST_ASSERT_EQUAL_HEX32(0xC1, reg(UART_IIR));
}

void test_d7_offset2_reads_iir_not_fcr(void) {
    put(UART_FCR, 0x07);
    put(UART_FCR, 0x00);
    TEST_ASSERT_EQUAL_HEX32(0x01, reg(UART_IIR));
}

/* ---- E. LSR ------------------------------------------------------------------------------------ */

void test_e1_lsr_constant(void) {
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT_EQUAL_HEX32(0x60, reg(UART_LSR));
    }
}

void test_e2_lsr_write_ignored(void) {
    put(UART_LSR, 0xFF);
    TEST_ASSERT_EQUAL_HEX32(0x60, reg(UART_LSR));
}

/* ---- F. MSR and loopback ----------------------------------------------------------------------- */

void test_f1_loopback_1a(void) {
    put(UART_MCR, 0x1A);
    TEST_ASSERT_EQUAL_HEX32(0x90, reg(UART_MSR) & 0xF0);
}

void test_f2_each_output_alone(void) {
    put(UART_MCR, UART_MCR_LOOP | UART_MCR_RTS);  TEST_ASSERT_EQUAL_HEX32(UART_MSR_CTS, reg(UART_MSR) & 0xF0);
    put(UART_MCR, UART_MCR_LOOP | UART_MCR_DTR);  TEST_ASSERT_EQUAL_HEX32(UART_MSR_DSR, reg(UART_MSR) & 0xF0);
    put(UART_MCR, UART_MCR_LOOP | UART_MCR_OUT1); TEST_ASSERT_EQUAL_HEX32(UART_MSR_RI,  reg(UART_MSR) & 0xF0);
    put(UART_MCR, UART_MCR_LOOP | UART_MCR_OUT2); TEST_ASSERT_EQUAL_HEX32(UART_MSR_DCD, reg(UART_MSR) & 0xF0);
}

void test_f3_entering_loopback_sets_change_flags(void) {
    put(UART_MCR, UART_MCR_LOOP);                  // CTS, DSR and DCD all drop
    TEST_ASSERT_EQUAL_HEX32(0x0B, reg(UART_MSR));  // DCTS | DDSR | DDCD, inputs 0
}

void test_f4_read_clears_flags(void) {
    put(UART_MCR, UART_MCR_LOOP);
    (void)reg(UART_MSR);
    TEST_ASSERT_EQUAL_HEX32(0x00, reg(UART_MSR));
}

void test_f5_leaving_loopback(void) {
    put(UART_MCR, UART_MCR_LOOP);
    (void)reg(UART_MSR);
    put(UART_MCR, 0);
    TEST_ASSERT_EQUAL_HEX32(0xBB, reg(UART_MSR));  // connected again, with the three change flags
}

void test_f6_teri_on_trailing_edge_only(void) {
    put(UART_MCR, UART_MCR_LOOP | UART_MCR_OUT1);  // RI rises
    uint32_t m = reg(UART_MSR);
    TEST_ASSERT_EQUAL_HEX32(UART_MSR_RI, m & UART_MSR_RI);
    TEST_ASSERT_EQUAL_HEX32(0, m & UART_MSR_TERI);
    put(UART_MCR, UART_MCR_LOOP);                  // RI falls
    TEST_ASSERT_EQUAL_HEX32(UART_MSR_TERI, reg(UART_MSR) & 0x0F);
}

void test_f7_msr_write_ignored(void) {
    put(UART_MSR, 0x00);
    TEST_ASSERT_EQUAL_HEX32(0xB0, reg(UART_MSR));
}

void test_f8_unchanged_mcr_sets_no_flags(void) {
    put(UART_MCR, 0x00);
    put(UART_MCR, 0x03);                           // DTR and RTS, not in loopback: inputs do not move
    TEST_ASSERT_EQUAL_HEX32(0xB0, reg(UART_MSR));
}

/* ---- G. The probes Linux and OpenSBI run -------------------------------------------------------- */

void test_g1_ier_probe(void) {
    put(UART_IER, 0x00);
    TEST_ASSERT_EQUAL_HEX32(0x00, reg(UART_IER) & 0x0F);
    put(UART_IER, 0x0F);
    TEST_ASSERT_EQUAL_HEX32(0x0F, reg(UART_IER) & 0x0F);
    put(UART_IER, 0x00);
}

void test_g2_loopback_probe(void) {
    put(UART_MCR, 0x1A);
    TEST_ASSERT_EQUAL_HEX32(0x90, reg(UART_MSR) & 0xF0);
    put(UART_MCR, 0x03);
    TEST_ASSERT_EQUAL_HEX32(0xB0, reg(UART_MSR) & 0xF0);
}

void test_g3_fifo_probe(void) {
    put(UART_LCR, 0xBF);
    put(UART_FCR, 0x00);
    put(UART_LCR, 0x00);
    put(UART_FCR, 0x01);
    TEST_ASSERT_EQUAL_HEX32(0xC0, reg(UART_IIR) & 0xC0);
}

void test_g4_scratch_probe(void) {
    put(UART_SCR, 0xA5); TEST_ASSERT_EQUAL_HEX32(0xA5, reg(UART_SCR));
    put(UART_SCR, 0x5A); TEST_ASSERT_EQUAL_HEX32(0x5A, reg(UART_SCR));
}

void test_g5_startup_sequence(void) {
    put(UART_FCR, 0x01);
    put(UART_FCR, 0x07);
    put(UART_FCR, 0x00);
    TEST_ASSERT_NOT_EQUAL(0xFF, reg(UART_LSR));
    (void)reg(UART_RBR);
    (void)reg(UART_IIR);
    (void)reg(UART_MSR);
}

void test_g6_opensbi_init(void) {
    put(UART_IER, 0x00);
    put(UART_LCR, 0x80);
    put(UART_DLL, 0x0A);
    put(UART_DLM, 0x00);
    put(UART_LCR, 0x03);
    put(UART_FCR, 0x01);
    put(UART_MCR, 0x00);
    (void)reg(UART_LSR);
    put(UART_SCR, 0x00);
    TEST_ASSERT_EQUAL_HEX32(0x03, reg(UART_LCR));
    TEST_ASSERT_EQUAL_HEX32(0xC1, reg(UART_IIR));
}

/* ---- H. The bus view --------------------------------------------------------------------------- */

void test_h1_offsets_8_and_up_fault(void) {
    uint32_t v;
    for (uint32_t o = 8; o < UART_SIZE; o++) {
        TEST_ASSERT_FALSE(rd(o, 1, &v));
        TEST_ASSERT_FALSE(wr(o, 1, 0x55));
    }
    TEST_ASSERT_EQUAL_HEX32(0, reg(UART_SCR));
}

void test_h2_wide_accesses_fault(void) {
    uint32_t v;
    for (uint32_t o = 0; o < 8; o++) {
        TEST_ASSERT_FALSE(rd(o, 2, &v));
        TEST_ASSERT_FALSE(rd(o, 4, &v));
        TEST_ASSERT_FALSE(wr(o, 2, 0));
        TEST_ASSERT_FALSE(wr(o, 4, 0));
    }
}

void test_h3_byte_accesses_succeed(void) {
    uint32_t v;
    for (uint32_t o = 0; o < 8; o++) {
        TEST_ASSERT_TRUE(rd(o, 1, &v));
        TEST_ASSERT_TRUE(wr(o, 1, 0));
    }
}

void test_h4_thr_write_dropped(void) {
    put(UART_IER, 0x03);
    put(UART_THR, 0x41);
    TEST_ASSERT_EQUAL_HEX32(0x03, reg(UART_IER));
    TEST_ASSERT_EQUAL_HEX32(0x60, reg(UART_LSR));
    TEST_ASSERT_EQUAL_HEX32(0, reg(UART_RBR));
}

void test_h5_device_description(void) {
    Device d = uart_device(&uart);
    TEST_ASSERT_EQUAL_HEX32(0x10000000, d.base);
    TEST_ASSERT_EQUAL_HEX32(0x100, d.size);
}

/* ---- I. Transmit and the THRE interrupt --------------------------------------------------------- */

static uint8_t sent[64];
static unsigned sent_n;

static void capture(void *ctx, uint8_t byte) {
    (void)ctx;
    if (sent_n < sizeof sent) sent[sent_n] = byte;
    sent_n++;
}

static void hook_up(void) {
    sent_n = 0;
    uart_set_tx(&uart, capture, NULL);
}

void test_i1_byte_reaches_host(void) {
    hook_up();
    put(UART_THR, 0x41);
    TEST_ASSERT_EQUAL_UINT(1, sent_n);
    TEST_ASSERT_EQUAL_HEX8(0x41, sent[0]);
    TEST_ASSERT_EQUAL_HEX32(0x60, reg(UART_LSR));
}

void test_i2_many_bytes_in_order(void) {
    hook_up();
    const char *text = "hello, uart: twenty b";
    unsigned n = 0;
    for (; text[n]; n++) put(UART_THR, (uint8_t)text[n]);
    TEST_ASSERT_EQUAL_UINT(n, sent_n);
    TEST_ASSERT_EQUAL_MEMORY(text, sent, n);
}

void test_i3_dlab_write_is_not_transmitted(void) {
    hook_up();
    put(UART_LCR, UART_LCR_DLAB);
    put(UART_DLL, 0x34);
    TEST_ASSERT_EQUAL_UINT(0, sent_n);
    TEST_ASSERT_EQUAL_HEX32(0x34, reg(UART_DLL));
}

void test_i4_no_hook_is_fine(void) {
    put(UART_THR, 0x41);
    TEST_ASSERT_EQUAL_HEX32(0x60, reg(UART_LSR));
}

void test_i5_loopback_stays_inside(void) {
    hook_up();
    put(UART_MCR, UART_MCR_LOOP);
    put(UART_THR, 0x41);
    TEST_ASSERT_EQUAL_UINT(0, sent_n);
}

void test_i6_enabling_etbei_raises_thre(void) {
    put(UART_IER, UART_IER_ETBEI);
    TEST_ASSERT_TRUE(uart_interrupt_pending(&uart));
    TEST_ASSERT_EQUAL_HEX32(0x02, reg(UART_IIR));
}

void test_i7_iir_read_clears_it(void) {
    put(UART_IER, UART_IER_ETBEI);
    (void)reg(UART_IIR);
    TEST_ASSERT_EQUAL_HEX32(0x01, reg(UART_IIR));
    TEST_ASSERT_FALSE(uart_interrupt_pending(&uart));
}

void test_i8_thr_write_rearms(void) {
    hook_up();
    put(UART_IER, UART_IER_ETBEI);
    (void)reg(UART_IIR);
    put(UART_THR, 0x41);
    TEST_ASSERT_TRUE(uart_interrupt_pending(&uart));
    TEST_ASSERT_EQUAL_HEX32(0x02, reg(UART_IIR));
}

void test_i9_disabling_etbei_hides_it(void) {
    put(UART_IER, UART_IER_ETBEI);
    put(UART_IER, 0x00);
    TEST_ASSERT_FALSE(uart_interrupt_pending(&uart));
    TEST_ASSERT_EQUAL_HEX32(0x01, reg(UART_IIR));
}

void test_i10_every_rising_edge_rearms(void) {
    for (int i = 0; i < 3; i++) {
        put(UART_IER, UART_IER_ETBEI);
        TEST_ASSERT_EQUAL_HEX32(0x02, reg(UART_IIR));
        put(UART_IER, 0x00);
    }
}

void test_i11_staying_enabled_does_not_rearm(void) {
    put(UART_IER, UART_IER_ETBEI);
    (void)reg(UART_IIR);
    put(UART_IER, UART_IER_ETBEI | 0x01);
    TEST_ASSERT_EQUAL_HEX32(0x01, reg(UART_IIR));
}

void test_i12_fcr_bit0_change_rearms(void) {
    put(UART_IER, UART_IER_ETBEI);
    (void)reg(UART_IIR);
    put(UART_FCR, 0x01);                           // bit 0 changes 0 -> 1
    TEST_ASSERT_EQUAL_HEX32(0xC2, reg(UART_IIR));
    put(UART_FCR, 0x01);                           // unchanged
    TEST_ASSERT_EQUAL_HEX32(0xC1, reg(UART_IIR));
    put(UART_FCR, 0x00);                           // 1 -> 0
    TEST_ASSERT_EQUAL_HEX32(0x02, reg(UART_IIR));
}

void test_i13_iir_with_fifos(void) {
    put(UART_FCR, 0x01);
    put(UART_IER, UART_IER_ETBEI);
    TEST_ASSERT_EQUAL_HEX32(0xC2, reg(UART_IIR));
}

void test_i14_latch_survives_while_disabled(void) {
    hook_up();
    put(UART_THR, 0x41);                           // sets the latch with ETBEI clear
    TEST_ASSERT_EQUAL_HEX32(0x01, reg(UART_IIR));
    TEST_ASSERT_FALSE(uart_interrupt_pending(&uart));
    put(UART_IER, UART_IER_ETBEI);
    TEST_ASSERT_EQUAL_HEX32(0x02, reg(UART_IIR));
}

void test_i15_reset_keeps_hook_and_clears_latch(void) {
    hook_up();
    put(UART_IER, UART_IER_ETBEI);
    uart_reset(&uart);
    TEST_ASSERT_FALSE(uart.thre_pending);
    put(UART_THR, 0x42);
    TEST_ASSERT_EQUAL_UINT(1, sent_n);
    TEST_ASSERT_EQUAL_HEX8(0x42, sent[0]);
}

void test_i16_linux_thre_probe(void) {
    for (int i = 0; i < 3; i++) {                  // enable, read IIR, disable: every time it must show THRE
        put(UART_IER, UART_IER_ETBEI);
        TEST_ASSERT_EQUAL_HEX32(0x02, reg(UART_IIR) & 0x0F);
        put(UART_IER, 0x00);
    }
}

void test_i17_polled_print_loop(void) {
    hook_up();
    const char *text = "Hi\n";
    for (const char *c = text; *c; c++) {
        while (!(reg(UART_LSR) & UART_LSR_THRE)) { }
        put(UART_THR, (uint8_t)*c);
    }
    TEST_ASSERT_EQUAL_UINT(3, sent_n);
    TEST_ASSERT_EQUAL_MEMORY(text, sent, 3);
}

/* ---- J. Receive ---------------------------------------------------------------------------------- */

static void ticks(unsigned n) { while (n--) uart_tick(&uart); }
static void feed(unsigned n, uint8_t first) { for (unsigned i = 0; i < n; i++) uart_receive(&uart, (uint8_t)(first + i)); }
static void fifo_on(uint8_t trigger_bits) { put(UART_FCR, 0x01 | (trigger_bits << 6)); }

void test_j1_one_byte_fifo_off(void) {
    TEST_ASSERT_TRUE(uart_receive(&uart, 0x41));
    TEST_ASSERT_EQUAL_HEX32(0x61, reg(UART_LSR));
    TEST_ASSERT_EQUAL_HEX32(0x41, reg(UART_RBR));
    TEST_ASSERT_EQUAL_HEX32(0x60, reg(UART_LSR));
}

void test_j2_fifo_order_and_dr(void) {
    fifo_on(0);
    feed(3, 0x10);
    TEST_ASSERT_EQUAL_HEX32(0x10, reg(UART_RBR));
    TEST_ASSERT_EQUAL_HEX32(0x11, reg(UART_RBR));
    TEST_ASSERT_EQUAL_HEX32(0x61, reg(UART_LSR));
    TEST_ASSERT_EQUAL_HEX32(0x12, reg(UART_RBR));
    TEST_ASSERT_EQUAL_HEX32(0x60, reg(UART_LSR));
}

void test_j3_empty_rbr_read(void) {
    TEST_ASSERT_EQUAL_HEX32(0, reg(UART_RBR));
    TEST_ASSERT_EQUAL_HEX32(0x60, reg(UART_LSR));
}

void test_j4_overrun_fifo_off(void) {
    uart_receive(&uart, 0x11);
    TEST_ASSERT_FALSE(uart_receive(&uart, 0x22));
    TEST_ASSERT_EQUAL_HEX32(0x63, reg(UART_LSR));
    TEST_ASSERT_EQUAL_HEX32(0x22, reg(UART_RBR));
    TEST_ASSERT_EQUAL_HEX32(0x60, reg(UART_LSR));
}

void test_j5_overrun_fifo_on(void) {
    fifo_on(0);
    TEST_ASSERT_EQUAL_UINT(16, uart_rx_space(&uart));
    for (unsigned i = 0; i < 16; i++) TEST_ASSERT_TRUE(uart_receive(&uart, (uint8_t)i));
    TEST_ASSERT_EQUAL_UINT(0, uart_rx_space(&uart));
    TEST_ASSERT_FALSE(uart_receive(&uart, 0xEE));
    for (unsigned i = 0; i < 16; i++) TEST_ASSERT_EQUAL_HEX32(i, reg(UART_RBR));
    TEST_ASSERT_EQUAL_HEX32(0x62, reg(UART_LSR));            // empty again, OE still set
}

void test_j6_lsr_read_clears_oe(void) {
    uart_receive(&uart, 1);
    uart_receive(&uart, 2);
    TEST_ASSERT_EQUAL_HEX32(0x02, reg(UART_LSR) & 0x02);
    TEST_ASSERT_EQUAL_HEX32(0x00, reg(UART_LSR) & 0x02);
}

void test_j7_dr_and_oe_together(void) {
    uart_receive(&uart, 1);
    uart_receive(&uart, 2);
    TEST_ASSERT_EQUAL_HEX32(0x63, reg(UART_LSR));
}

void test_j8_trigger_levels(void) {
    static const unsigned level[4] = {1, 4, 8, 14};
    for (int t = 0; t < 4; t++) {
        uart_reset(&uart);
        put(UART_IER, UART_IER_ERBFI);
        fifo_on((uint8_t)t);
        feed(level[t] - 1, 0);
        if (level[t] > 1) TEST_ASSERT_EQUAL_HEX32(0xC1, reg(UART_IIR));
        uart_receive(&uart, 0);
        TEST_ASSERT_EQUAL_HEX32(0xC4, reg(UART_IIR));
    }
}

void test_j9_rda_drops_below_trigger(void) {
    put(UART_IER, UART_IER_ERBFI);
    fifo_on(1);                                              // 4 bytes
    feed(4, 0);
    TEST_ASSERT_EQUAL_HEX32(0xC4, reg(UART_IIR));
    (void)reg(UART_RBR);
    TEST_ASSERT_EQUAL_HEX32(0xC1, reg(UART_IIR));
}

void test_j10_rda_fifo_off(void) {
    put(UART_IER, UART_IER_ERBFI);
    uart_receive(&uart, 7);
    TEST_ASSERT_EQUAL_HEX32(0x04, reg(UART_IIR));
}

void test_j11_erbfi_clear_hides_rda_and_timeout(void) {
    fifo_on(2);
    uart_receive(&uart, 7);
    ticks(UART_TIMEOUT_CYCLES);
    TEST_ASSERT_EQUAL_HEX32(0xC1, reg(UART_IIR));
    feed(8, 0);
    TEST_ASSERT_EQUAL_HEX32(0xC1, reg(UART_IIR));
}

void test_j12_timeout_after_exact_idle(void) {
    put(UART_IER, UART_IER_ERBFI);
    fifo_on(2);                                              // trigger 8
    uart_receive(&uart, 7);
    ticks(UART_TIMEOUT_CYCLES - 1);
    TEST_ASSERT_EQUAL_HEX32(0xC1, reg(UART_IIR));
    ticks(1);
    TEST_ASSERT_EQUAL_HEX32(0xCC, reg(UART_IIR));
}

void test_j13_read_clears_timeout(void) {
    put(UART_IER, UART_IER_ERBFI);
    fifo_on(2);
    uart_receive(&uart, 7);
    ticks(UART_TIMEOUT_CYCLES);
    (void)reg(UART_RBR);
    TEST_ASSERT_EQUAL_HEX32(0xC1, reg(UART_IIR));
}

void test_j14_timeout_returns_after_a_read(void) {
    put(UART_IER, UART_IER_ERBFI);
    fifo_on(2);
    feed(2, 0);
    ticks(UART_TIMEOUT_CYCLES);
    (void)reg(UART_RBR);
    TEST_ASSERT_EQUAL_HEX32(0xC1, reg(UART_IIR));
    ticks(UART_TIMEOUT_CYCLES - 1);
    TEST_ASSERT_EQUAL_HEX32(0xC1, reg(UART_IIR));
    ticks(1);
    TEST_ASSERT_EQUAL_HEX32(0xCC, reg(UART_IIR));
}

void test_j15_arrival_restarts_timeout(void) {
    put(UART_IER, UART_IER_ERBFI);
    fifo_on(2);
    uart_receive(&uart, 1);
    ticks(UART_TIMEOUT_CYCLES);
    uart_receive(&uart, 2);
    TEST_ASSERT_EQUAL_HEX32(0xC1, reg(UART_IIR));
    ticks(UART_TIMEOUT_CYCLES);
    TEST_ASSERT_EQUAL_HEX32(0xCC, reg(UART_IIR));
}

void test_j16_no_timeout_fifo_off(void) {
    put(UART_IER, UART_IER_ERBFI);
    uart_receive(&uart, 1);
    (void)reg(UART_IIR);
    put(UART_FCR, 0x00);
    ticks(3 * UART_TIMEOUT_CYCLES);
    TEST_ASSERT_EQUAL_HEX32(0x04, reg(UART_IIR));            // the data interrupt, never the timeout
}

void test_j17_line_status_interrupt(void) {
    put(UART_IER, UART_IER_ELSI);
    uart_receive(&uart, 1);
    uart_receive(&uart, 2);
    TEST_ASSERT_EQUAL_HEX32(0x06, reg(UART_IIR));
    TEST_ASSERT_TRUE(uart_interrupt_pending(&uart));
    (void)reg(UART_LSR);
    TEST_ASSERT_EQUAL_HEX32(0x01, reg(UART_IIR));
    TEST_ASSERT_FALSE(uart_interrupt_pending(&uart));
}

void test_j18_priority_order(void) {
    put(UART_IER, 0x0F);                                     // THRE is raised by this write
    (void)reg(UART_IIR);                                     // consume it
    put(UART_MCR, UART_MCR_LOOP);                            // modem change flags
    uart_receive(&uart, 1);
    uart_receive(&uart, 2);                                  // data and overrun
    put(UART_IER, 0x00);
    put(UART_IER, 0x0F);                                     // THRE again
    TEST_ASSERT_EQUAL_HEX32(0x06, reg(UART_IIR));            // line status first
    (void)reg(UART_LSR);
    TEST_ASSERT_EQUAL_HEX32(0x04, reg(UART_IIR));            // then received data
    (void)reg(UART_RBR);
    TEST_ASSERT_EQUAL_HEX32(0x02, reg(UART_IIR));            // then THRE (this read clears it)
    TEST_ASSERT_EQUAL_HEX32(0x00, reg(UART_IIR));            // then modem status
    (void)reg(UART_MSR);
    TEST_ASSERT_EQUAL_HEX32(0x01, reg(UART_IIR));
}

void test_j19_modem_status_interrupt(void) {
    put(UART_IER, UART_IER_EDSSI);
    put(UART_MCR, UART_MCR_LOOP);                            // CTS, DSR and DCD drop
    TEST_ASSERT_EQUAL_HEX32(0x00, reg(UART_IIR));
    TEST_ASSERT_TRUE(uart_interrupt_pending(&uart));
    (void)reg(UART_MSR);
    TEST_ASSERT_EQUAL_HEX32(0x01, reg(UART_IIR));
}

void test_j20_fcr_bit0_change_clears_fifo(void) {
    fifo_on(0);
    feed(3, 0);
    put(UART_FCR, 0x00);
    TEST_ASSERT_EQUAL_HEX32(0x60, reg(UART_LSR));
    fifo_on(0);
    feed(3, 0);
    put(UART_FCR, 0x01);                                     // bit 0 unchanged: nothing cleared
    TEST_ASSERT_EQUAL_HEX32(0x61, reg(UART_LSR));
}

void test_j21_fcr_bit1_clears_fifo_keeps_oe(void) {
    fifo_on(0);
    feed(17, 0);                                             // overrun
    put(UART_FCR, 0x03);
    TEST_ASSERT_EQUAL_HEX32(0x62, reg(UART_LSR));            // empty, OE kept
    TEST_ASSERT_EQUAL_UINT(16, uart_rx_space(&uart));
}

void test_j22_loopback_receives_thr(void) {
    hook_up();
    put(UART_MCR, UART_MCR_LOOP);
    put(UART_THR, 0x41);
    TEST_ASSERT_EQUAL_UINT(0, sent_n);
    TEST_ASSERT_EQUAL_HEX32(0x41, reg(UART_RBR));
}

void test_j23_loopback_thre_and_dr(void) {
    put(UART_MCR, UART_MCR_LOOP);
    put(UART_IER, UART_IER_ETBEI);
    (void)reg(UART_IIR);
    put(UART_THR, 0x41);
    TEST_ASSERT_EQUAL_HEX32(0x61, reg(UART_LSR));
    TEST_ASSERT_EQUAL_HEX32(0x02, reg(UART_IIR));
}

void test_j24_loopback_overrun(void) {
    fifo_on(0);
    put(UART_MCR, UART_MCR_LOOP);
    for (int i = 0; i < 17; i++) put(UART_THR, (uint8_t)i);
    TEST_ASSERT_EQUAL_HEX32(0x02, reg(UART_LSR) & 0x02);
    TEST_ASSERT_EQUAL_HEX32(0x00, reg(UART_RBR));            // the first byte survived
}

void test_j25_linux_loopback_self_test(void) {
    put(UART_MCR, UART_MCR_LOOP);
    put(UART_THR, 0x5A);
    TEST_ASSERT_EQUAL_HEX32(1, reg(UART_LSR) & UART_LSR_DR);
    TEST_ASSERT_EQUAL_HEX32(0x5A, reg(UART_RBR));
    put(UART_MCR, 0x00);
    TEST_ASSERT_EQUAL_HEX32(0, reg(UART_LSR) & UART_LSR_DR);
}

void test_j26_pending_follows_iir(void) {
    put(UART_IER, UART_IER_ERBFI);
    TEST_ASSERT_FALSE(uart_interrupt_pending(&uart));
    uart_receive(&uart, 1);
    TEST_ASSERT_TRUE(uart_interrupt_pending(&uart));
    (void)reg(UART_RBR);
    TEST_ASSERT_FALSE(uart_interrupt_pending(&uart));
}

void test_j27_mem_tick_calls_device_tick(void) {
    uart_receive(&uart, 1);
    TEST_ASSERT_EQUAL_UINT32(0, uart.rx_idle);
    mem_tick(&mem);
    mem_tick(&mem);
    TEST_ASSERT_EQUAL_UINT32(2, uart.rx_idle);
    Memory other;
    mem_init_at(&other, RAM_BASE, 0x1000);                   // no devices: nothing to call
    mem_tick(&other);
    mem_free(&other);
}

static unsigned counted;
static void count_tick(void *ctx) { (void)ctx; counted++; }

void test_j27b_every_device_with_a_tick_is_called(void) {
    Memory two;
    Uart second;
    uart_reset(&second);
    mem_init_at(&two, RAM_BASE, 0x1000);
    mem_add_device(&two, (Device){ .base = 0x20000000u, .size = 0x10, .sizes = DEV_ACCESS_32, .tick = count_tick });
    mem_add_device(&two, (Device){ .base = 0x21000000u, .size = 0x10, .sizes = DEV_ACCESS_32 });   // no tick: skipped
    mem_add_device(&two, uart_device(&second));              // after the others
    counted = 0;
    uart_receive(&second, 1);
    mem_tick(&two);
    mem_tick(&two);
    TEST_ASSERT_EQUAL_UINT(2, counted);
    TEST_ASSERT_EQUAL_UINT32(2, second.rx_idle);
    mem_free(&two);
}

void test_j31_old_trigger_does_not_apply_without_fifos(void) {
    put(UART_IER, UART_IER_ERBFI);
    fifo_on(2);                                              // trigger 8 programmed
    put(UART_FCR, 0x00);                                     // FIFOs off again
    uart_receive(&uart, 1);
    TEST_ASSERT_EQUAL_HEX32(0x04, reg(UART_IIR));            // one character is enough
}

void test_j28_cpu_step_ticks_devices(void) {
    CPU cpu;
    test_cpu_reset_in_mode(&cpu, PRV_M);
    for (uint32_t i = 0; i < 0x40; i += 4) mem_write32(&mem, RAM_BASE + i, 0x00000013);   // NOPs
    cpu.pc = RAM_BASE;
    uart_receive(&uart, 1);
    for (int i = 0; i < 10; i++) cpu_step(&cpu, &mem);
    TEST_ASSERT_EQUAL_UINT32(10, uart.rx_idle);
}

void test_j29_adjustable_timeout(void) {
    uart_set_timeout(&uart, 100);
    put(UART_IER, UART_IER_ERBFI);
    fifo_on(2);
    uart_receive(&uart, 1);
    ticks(99);
    TEST_ASSERT_EQUAL_HEX32(0xC1, reg(UART_IIR));
    ticks(1);
    TEST_ASSERT_EQUAL_HEX32(0xCC, reg(UART_IIR));
}

void test_j30_timeout_survives_reset_and_defaults(void) {
    uart_set_timeout(&uart, 100);
    uart_reset(&uart);
    put(UART_IER, UART_IER_ERBFI);
    fifo_on(2);
    uart_receive(&uart, 1);
    ticks(100);
    TEST_ASSERT_EQUAL_HEX32(0xCC, reg(UART_IIR));
    uart_set_timeout(&uart, 0);                              // back to the default
    TEST_ASSERT_EQUAL_UINT32(UART_TIMEOUT_CYCLES, uart.rx_timeout);
    uart_reset(&uart);
    put(UART_IER, UART_IER_ERBFI);
    fifo_on(2);
    uart_receive(&uart, 1);
    ticks(UART_TIMEOUT_CYCLES - 1);
    TEST_ASSERT_EQUAL_HEX32(0xC1, reg(UART_IIR));
    ticks(1);
    TEST_ASSERT_EQUAL_HEX32(0xCC, reg(UART_IIR));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_a1_reset_values);
    RUN_TEST(test_a2_iir);
    RUN_TEST(test_a3_lsr);
    RUN_TEST(test_a4_msr);
    RUN_TEST(test_a5_others);
    RUN_TEST(test_b1_ier);
    RUN_TEST(test_b2_lcr);
    RUN_TEST(test_b3_mcr);
    RUN_TEST(test_b4_scr);
    RUN_TEST(test_c1_divisor_latches);
    RUN_TEST(test_c2_dlab_clear_shows_rbr_and_ier);
    RUN_TEST(test_c3_divisor_write_touches_nothing_else);
    RUN_TEST(test_c4_ier_survives_dlab);
    RUN_TEST(test_c5_dlab_does_not_affect_other_offsets);
    RUN_TEST(test_d1_fifo_enable_shows_in_iir);
    RUN_TEST(test_d2_fifo_disable);
    RUN_TEST(test_d3_trigger_not_programmed_without_enable);
    RUN_TEST(test_d4_trigger_levels);
    RUN_TEST(test_d5_clear_bits_are_accepted);
    RUN_TEST(test_d6_ier_without_etbei_raises_nothing);
    RUN_TEST(test_d7_offset2_reads_iir_not_fcr);
    RUN_TEST(test_e1_lsr_constant);
    RUN_TEST(test_e2_lsr_write_ignored);
    RUN_TEST(test_f1_loopback_1a);
    RUN_TEST(test_f2_each_output_alone);
    RUN_TEST(test_f3_entering_loopback_sets_change_flags);
    RUN_TEST(test_f4_read_clears_flags);
    RUN_TEST(test_f5_leaving_loopback);
    RUN_TEST(test_f6_teri_on_trailing_edge_only);
    RUN_TEST(test_f7_msr_write_ignored);
    RUN_TEST(test_f8_unchanged_mcr_sets_no_flags);
    RUN_TEST(test_g1_ier_probe);
    RUN_TEST(test_g2_loopback_probe);
    RUN_TEST(test_g3_fifo_probe);
    RUN_TEST(test_g4_scratch_probe);
    RUN_TEST(test_g5_startup_sequence);
    RUN_TEST(test_g6_opensbi_init);
    RUN_TEST(test_h1_offsets_8_and_up_fault);
    RUN_TEST(test_h2_wide_accesses_fault);
    RUN_TEST(test_h3_byte_accesses_succeed);
    RUN_TEST(test_h4_thr_write_dropped);
    RUN_TEST(test_h5_device_description);
    RUN_TEST(test_i1_byte_reaches_host);
    RUN_TEST(test_i2_many_bytes_in_order);
    RUN_TEST(test_i3_dlab_write_is_not_transmitted);
    RUN_TEST(test_i4_no_hook_is_fine);
    RUN_TEST(test_i5_loopback_stays_inside);
    RUN_TEST(test_i6_enabling_etbei_raises_thre);
    RUN_TEST(test_i7_iir_read_clears_it);
    RUN_TEST(test_i8_thr_write_rearms);
    RUN_TEST(test_i9_disabling_etbei_hides_it);
    RUN_TEST(test_i10_every_rising_edge_rearms);
    RUN_TEST(test_i11_staying_enabled_does_not_rearm);
    RUN_TEST(test_i12_fcr_bit0_change_rearms);
    RUN_TEST(test_i13_iir_with_fifos);
    RUN_TEST(test_i14_latch_survives_while_disabled);
    RUN_TEST(test_i15_reset_keeps_hook_and_clears_latch);
    RUN_TEST(test_i16_linux_thre_probe);
    RUN_TEST(test_i17_polled_print_loop);
    RUN_TEST(test_j1_one_byte_fifo_off);
    RUN_TEST(test_j2_fifo_order_and_dr);
    RUN_TEST(test_j3_empty_rbr_read);
    RUN_TEST(test_j4_overrun_fifo_off);
    RUN_TEST(test_j5_overrun_fifo_on);
    RUN_TEST(test_j6_lsr_read_clears_oe);
    RUN_TEST(test_j7_dr_and_oe_together);
    RUN_TEST(test_j8_trigger_levels);
    RUN_TEST(test_j9_rda_drops_below_trigger);
    RUN_TEST(test_j10_rda_fifo_off);
    RUN_TEST(test_j11_erbfi_clear_hides_rda_and_timeout);
    RUN_TEST(test_j12_timeout_after_exact_idle);
    RUN_TEST(test_j13_read_clears_timeout);
    RUN_TEST(test_j14_timeout_returns_after_a_read);
    RUN_TEST(test_j15_arrival_restarts_timeout);
    RUN_TEST(test_j16_no_timeout_fifo_off);
    RUN_TEST(test_j17_line_status_interrupt);
    RUN_TEST(test_j18_priority_order);
    RUN_TEST(test_j19_modem_status_interrupt);
    RUN_TEST(test_j20_fcr_bit0_change_clears_fifo);
    RUN_TEST(test_j21_fcr_bit1_clears_fifo_keeps_oe);
    RUN_TEST(test_j22_loopback_receives_thr);
    RUN_TEST(test_j23_loopback_thre_and_dr);
    RUN_TEST(test_j24_loopback_overrun);
    RUN_TEST(test_j25_linux_loopback_self_test);
    RUN_TEST(test_j26_pending_follows_iir);
    RUN_TEST(test_j27_mem_tick_calls_device_tick);
    RUN_TEST(test_j27b_every_device_with_a_tick_is_called);
    RUN_TEST(test_j28_cpu_step_ticks_devices);
    RUN_TEST(test_j29_adjustable_timeout);
    RUN_TEST(test_j30_timeout_survives_reset_and_defaults);
    RUN_TEST(test_j31_old_trigger_does_not_apply_without_fifos);
    return UNITY_END();
}
