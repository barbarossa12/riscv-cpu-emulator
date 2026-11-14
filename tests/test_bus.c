#include <string.h>
#include "unity.h"
#include "cpu_test_util.h"
#include "../src/execute.h"
#include "../src/mmu.h"
#include "../src/pmp.h"

#define RAM_BASE 0x80000000u
#define DEV_A    0x02000000u   // 32-bit registers only
#define DEV_B    0x03000000u   // bytes, halfwords and words
#define DEV_A_SIZE 0x100u
#define DEV_B_SIZE 0x10u
#define REFUSE_WRITE 0x80u     // device A: writing this offset fails
#define REFUSE_READ  0x84u     // device A: reading this offset fails

// A device that records what it is asked to do. Reads return a value the test sets, so a load can tell
// whether it reached the device.
typedef struct {
    int reads, writes;
    uint32_t last_offset, last_size, last_value;
    uint32_t read_value;
} Fake;

static Fake fake_a, fake_b;

static bool fake_read(void *ctx, uint32_t offset, uint32_t size, uint32_t *value) {
    Fake *f = ctx;
    f->reads++;
    f->last_offset = offset;
    f->last_size = size;
    if (f == &fake_a && offset == REFUSE_READ) {
        return false;
    }
    *value = f->read_value;
    return true;
}

static bool fake_write(void *ctx, uint32_t offset, uint32_t size, uint32_t value) {
    Fake *f = ctx;
    f->writes++;
    f->last_offset = offset;
    f->last_size = size;
    f->last_value = value;
    return !(f == &fake_a && offset == REFUSE_WRITE);
}

static CPU cpu;
static Memory mem;

void setUp(void) {
    memset(&fake_a, 0, sizeof fake_a);
    memset(&fake_b, 0, sizeof fake_b);
    mem_init_at(&mem, RAM_BASE, 0x20000);
    mem_add_device(&mem, (Device){ .base = DEV_A, .size = DEV_A_SIZE, .sizes = DEV_ACCESS_32,
                                   .read = fake_read, .write = fake_write, .ctx = &fake_a });
    mem_add_device(&mem, (Device){ .base = DEV_B, .size = DEV_B_SIZE,
                                   .sizes = DEV_ACCESS_8 | DEV_ACCESS_16 | DEV_ACCESS_32,
                                   .read = fake_read, .write = fake_write, .ctx = &fake_b });
    test_cpu_reset_in_mode(&cpu, PRV_M);
}

void tearDown(void) {
    mem_free(&mem);
}

/* ---- A. The bus functions ---------------------------------------------------------------------- */

void test_a1_ram_sizes_and_misalignment(void) {
    uint32_t v;
    TEST_ASSERT_TRUE(mem_bus_write(&mem, RAM_BASE + 0x100, 4, 0x11223344));
    TEST_ASSERT_TRUE(mem_bus_read(&mem, RAM_BASE + 0x100, 1, &v));
    TEST_ASSERT_EQUAL_HEX32(0x44, v);
    TEST_ASSERT_TRUE(mem_bus_read(&mem, RAM_BASE + 0x100, 2, &v));
    TEST_ASSERT_EQUAL_HEX32(0x3344, v);
    TEST_ASSERT_TRUE(mem_bus_read(&mem, RAM_BASE + 0x100, 4, &v));
    TEST_ASSERT_EQUAL_HEX32(0x11223344, v);
    TEST_ASSERT_TRUE(mem_bus_read(&mem, RAM_BASE + 0x101, 4, &v));          // misaligned, still RAM
    TEST_ASSERT_EQUAL_HEX32(0x00112233, v);
}

void test_a2_ram_three_bytes(void) {
    uint32_t v;
    TEST_ASSERT_TRUE(mem_bus_write(&mem, RAM_BASE + 0x200, 3, 0xAABBCCDD));
    TEST_ASSERT_TRUE(mem_bus_read(&mem, RAM_BASE + 0x200, 3, &v));
    TEST_ASSERT_EQUAL_HEX32(0xBBCCDD, v);
    TEST_ASSERT_EQUAL_HEX8(0, mem_read8(&mem, RAM_BASE + 0x203));          // the fourth byte is untouched
}

void test_a3_outside_ram_and_everything(void) {
    uint32_t v;
    TEST_ASSERT_FALSE(mem_bus_read(&mem, RAM_BASE + 0x20000 - 2, 4, &v));   // partly past the end
    TEST_ASSERT_FALSE(mem_bus_write(&mem, RAM_BASE + 0x20000 - 2, 4, 0));
    TEST_ASSERT_FALSE(mem_bus_read(&mem, 0x10000000u, 4, &v));              // nothing there
}

void test_a4_device_read(void) {
    uint32_t v = 0;
    fake_a.read_value = 0xCAFEF00D;
    TEST_ASSERT_TRUE(mem_bus_read(&mem, DEV_A + 8, 4, &v));
    TEST_ASSERT_EQUAL_HEX32(0xCAFEF00D, v);
    TEST_ASSERT_EQUAL_INT(1, fake_a.reads);
    TEST_ASSERT_EQUAL_UINT32(8, fake_a.last_offset);
    TEST_ASSERT_EQUAL_UINT32(4, fake_a.last_size);
}

void test_a5_device_write(void) {
    TEST_ASSERT_TRUE(mem_bus_write(&mem, DEV_A + 8, 4, 0x12345678));
    TEST_ASSERT_EQUAL_INT(1, fake_a.writes);
    TEST_ASSERT_EQUAL_UINT32(8, fake_a.last_offset);
    TEST_ASSERT_EQUAL_UINT32(4, fake_a.last_size);
    TEST_ASSERT_EQUAL_HEX32(0x12345678, fake_a.last_value);
}

void test_a6_a_width_the_device_does_not_list(void) {
    uint32_t v;
    TEST_ASSERT_FALSE(mem_bus_read(&mem, DEV_A, 1, &v));
    TEST_ASSERT_FALSE(mem_bus_read(&mem, DEV_A, 2, &v));
    TEST_ASSERT_FALSE(mem_bus_write(&mem, DEV_A, 1, 0));
    TEST_ASSERT_EQUAL_INT(0, fake_a.reads + fake_a.writes);                 // the device was never asked
}

void test_a7_device_with_all_widths(void) {
    uint32_t v;
    fake_b.read_value = 0x55;
    TEST_ASSERT_TRUE(mem_bus_read(&mem, DEV_B + 1, 1, &v));
    TEST_ASSERT_EQUAL_UINT32(1, fake_b.last_size);
    TEST_ASSERT_EQUAL_UINT32(1, fake_b.last_offset);
    TEST_ASSERT_TRUE(mem_bus_read(&mem, DEV_B + 2, 2, &v));
    TEST_ASSERT_EQUAL_UINT32(2, fake_b.last_size);
    TEST_ASSERT_TRUE(mem_bus_read(&mem, DEV_B + 4, 4, &v));
    TEST_ASSERT_EQUAL_UINT32(4, fake_b.last_size);
    TEST_ASSERT_EQUAL_INT(3, fake_b.reads);
}

void test_a8_misaligned_device_access(void) {
    uint32_t v;
    TEST_ASSERT_FALSE(mem_bus_read(&mem, DEV_A + 2, 4, &v));
    TEST_ASSERT_FALSE(mem_bus_write(&mem, DEV_A + 2, 4, 0));
    TEST_ASSERT_FALSE(mem_bus_read(&mem, DEV_B + 1, 2, &v));
    TEST_ASSERT_FALSE(mem_bus_write(&mem, DEV_B + 1, 2, 0));
    TEST_ASSERT_EQUAL_INT(0, fake_a.reads + fake_a.writes + fake_b.reads + fake_b.writes);
}

void test_a9_straddling_the_end_of_a_device(void) {
    uint32_t v;
    TEST_ASSERT_FALSE(mem_bus_read(&mem, DEV_A + DEV_A_SIZE - 2, 4, &v));
    TEST_ASSERT_FALSE(mem_bus_read(&mem, DEV_A + DEV_A_SIZE, 4, &v));       // just past it
    TEST_ASSERT_EQUAL_INT(0, fake_a.reads);
    TEST_ASSERT_TRUE(mem_bus_read(&mem, DEV_B + DEV_B_SIZE - 1, 1, &v));    // the last byte of device B
    TEST_ASSERT_EQUAL_INT(1, fake_b.reads);
    TEST_ASSERT_FALSE(mem_bus_read(&mem, DEV_B + DEV_B_SIZE, 1, &v));       // the first byte after it
    TEST_ASSERT_FALSE(mem_bus_write(&mem, DEV_B + DEV_B_SIZE, 1, 0));
    TEST_ASSERT_EQUAL_INT(1, fake_b.reads);
    TEST_ASSERT_EQUAL_INT(0, fake_b.writes);
}

void test_a10_unmapped_address(void) {
    uint32_t v;
    TEST_ASSERT_FALSE(mem_bus_read(&mem, 0x02800000u, 4, &v));
    TEST_ASSERT_FALSE(mem_bus_write(&mem, 0x02800000u, 4, 0));
}

void test_a11_the_device_refuses(void) {
    uint32_t v;
    TEST_ASSERT_FALSE(mem_bus_read(&mem, DEV_A + REFUSE_READ, 4, &v));
    TEST_ASSERT_EQUAL_INT(1, fake_a.reads);                                 // it was asked, and said no
    TEST_ASSERT_FALSE(mem_bus_write(&mem, DEV_A + REFUSE_WRITE, 4, 0));
    TEST_ASSERT_EQUAL_INT(1, fake_a.writes);
}

void test_a12_read_only_and_write_only_devices(void) {
    Memory m;
    uint32_t v;
    mem_init_at(&m, RAM_BASE, 0x1000);
    mem_add_device(&m, (Device){ .base = 0x04000000u, .size = 0x10, .sizes = DEV_ACCESS_32,
                                 .read = fake_read, .ctx = &fake_a });      // read-only
    mem_add_device(&m, (Device){ .base = 0x05000000u, .size = 0x10, .sizes = DEV_ACCESS_32,
                                 .write = fake_write, .ctx = &fake_b });    // write-only
    TEST_ASSERT_TRUE(mem_bus_read(&m, 0x04000000u, 4, &v));
    TEST_ASSERT_FALSE(mem_bus_write(&m, 0x04000000u, 4, 0));
    TEST_ASSERT_FALSE(mem_bus_writable(&m, 0x04000000u, 4));
    TEST_ASSERT_FALSE(mem_bus_read(&m, 0x05000000u, 4, &v));
    TEST_ASSERT_TRUE(mem_bus_write(&m, 0x05000000u, 4, 0));
    mem_free(&m);
}

void test_a13_writable_touches_nothing(void) {
    TEST_ASSERT_TRUE(mem_bus_writable(&mem, DEV_A, 4));
    TEST_ASSERT_TRUE(mem_bus_writable(&mem, DEV_B + 1, 1));
    TEST_ASSERT_TRUE(mem_bus_writable(&mem, RAM_BASE, 4));
    TEST_ASSERT_FALSE(mem_bus_writable(&mem, DEV_A + 2, 4));                // misaligned
    TEST_ASSERT_FALSE(mem_bus_writable(&mem, DEV_A, 1));                    // a width it does not list
    TEST_ASSERT_FALSE(mem_bus_writable(&mem, 0x02800000u, 4));              // unmapped
    TEST_ASSERT_EQUAL_INT(0, fake_a.reads + fake_a.writes + fake_b.reads + fake_b.writes);
}

void test_a14_in_ram(void) {
    TEST_ASSERT_TRUE(mem_in_ram(&mem, RAM_BASE, 4));
    TEST_ASSERT_FALSE(mem_in_ram(&mem, DEV_A, 4));
    TEST_ASSERT_FALSE(mem_in_ram(&mem, RAM_BASE + 0x20000 - 2, 4));
}

void test_a15_a_new_memory_has_no_devices(void) {
    uint32_t v;
    mem_free(&mem);
    mem_init_at(&mem, RAM_BASE, 0x1000);
    TEST_ASSERT_EQUAL_INT(0, mem.device_count);
    TEST_ASSERT_FALSE(mem_bus_read(&mem, DEV_A, 4, &v));
}

/* ---- B. Through the CPU -------------------------------------------------------------------------- */

#define RD   1
#define ADDR 2
#define SRC  3
#define PC   (RAM_BASE + 0x400u)
#define MTVEC (RAM_BASE + 0x800u)

static uint32_t enc_load(uint32_t funct3)  { return (ADDR << 15) | (funct3 << 12) | (RD << 7) | 0x03; }
static uint32_t enc_store(uint32_t funct3) { return (SRC << 20) | (ADDR << 15) | (funct3 << 12) | 0x23; }
#define LB  enc_load(0)
#define LH  enc_load(1)
#define LW  enc_load(2)
#define LBU enc_load(4)
#define LHU enc_load(5)
#define SB  enc_store(0)
#define SH  enc_store(1)
#define SW  enc_store(2)
#define AMOADD_W 0x003120AFu   // AMOADD.W x1, x3, (x2)
#define LR_W     0x100120AFu   // LR.W x1, (x2)
#define SC_W     0x183120AFu   // SC.W x1, x3, (x2)

static void run(uint32_t raw, uint32_t va) {
    cpu_write_csr(&cpu, CSR_MTVEC, MTVEC);
    cpu_write_reg(&cpu, ADDR, va);
    cpu.pc = PC;
    DecodedInstruction instr = decode_instruction(raw);
    execute(&cpu, &mem, &instr);
}

static void expect_trap(uint32_t cause, uint32_t tval) {
    TEST_ASSERT_EQUAL_HEX32(MTVEC, cpu.pc);
    TEST_ASSERT_EQUAL_UINT32(cause, cpu_read_csr(&cpu, CSR_MCAUSE));
    TEST_ASSERT_EQUAL_HEX32(tval, cpu_read_csr(&cpu, CSR_MTVAL));
}

static void expect_untouched(void) {
    TEST_ASSERT_EQUAL_INT(0, fake_a.reads + fake_a.writes + fake_b.reads + fake_b.writes);
}

void test_b1_lw_from_a_device(void) {
    fake_a.read_value = 0xCAFEF00D;
    run(LW, DEV_A + 0x10);
    TEST_ASSERT_EQUAL_HEX32(PC + 4, cpu.pc);
    TEST_ASSERT_EQUAL_HEX32(0xCAFEF00D, cpu_read_reg(&cpu, RD));
    TEST_ASSERT_EQUAL_INT(1, fake_a.reads);
    TEST_ASSERT_EQUAL_UINT32(0x10, fake_a.last_offset);
    TEST_ASSERT_EQUAL_UINT32(4, fake_a.last_size);
}

void test_b2_narrow_loads_from_a_device(void) {
    fake_b.read_value = 0x80;
    run(LB, DEV_B + 1);
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFF80, cpu_read_reg(&cpu, RD));
    TEST_ASSERT_EQUAL_UINT32(1, fake_b.last_size);
    run(LBU, DEV_B + 1);
    TEST_ASSERT_EQUAL_HEX32(0x80, cpu_read_reg(&cpu, RD));
    fake_b.read_value = 0x8001;
    run(LH, DEV_B + 2);
    TEST_ASSERT_EQUAL_HEX32(0xFFFF8001, cpu_read_reg(&cpu, RD));
    TEST_ASSERT_EQUAL_UINT32(2, fake_b.last_size);
    run(LHU, DEV_B + 2);
    TEST_ASSERT_EQUAL_HEX32(0x8001, cpu_read_reg(&cpu, RD));
    TEST_ASSERT_EQUAL_INT(4, fake_b.reads);
}

void test_b3_a_width_the_device_does_not_take(void) {
    cpu_write_reg(&cpu, RD, 0x12345678);
    run(LB, DEV_A);
    expect_trap(5, DEV_A);
    TEST_ASSERT_EQUAL_HEX32(0x12345678, cpu_read_reg(&cpu, RD));
    expect_untouched();
}

void test_b4_a_store_never_reads_the_device(void) {
    cpu_write_reg(&cpu, SRC, 0xAABBCCDD);
    run(SW, DEV_A + 0x20);
    TEST_ASSERT_EQUAL_HEX32(PC + 4, cpu.pc);
    TEST_ASSERT_EQUAL_INT(1, fake_a.writes);
    TEST_ASSERT_EQUAL_HEX32(0xAABBCCDD, fake_a.last_value);
    TEST_ASSERT_EQUAL_INT(0, fake_a.reads);
    run(SB, DEV_B + 1);
    TEST_ASSERT_EQUAL_HEX32(0xDD, fake_b.last_value & 0xFF);
    TEST_ASSERT_EQUAL_UINT32(1, fake_b.last_size);
    run(SH, DEV_B + 2);
    TEST_ASSERT_EQUAL_UINT32(2, fake_b.last_size);
    TEST_ASSERT_EQUAL_INT(2, fake_b.writes);
    TEST_ASSERT_EQUAL_INT(0, fake_b.reads);
}

void test_b5_a_refused_store(void) {
    run(SW, DEV_A + REFUSE_WRITE);
    expect_trap(7, DEV_A + REFUSE_WRITE);
}

void test_b6_unmapped_addresses(void) {
    run(LW, 0x02800000u);
    expect_trap(5, 0x02800000u);
    run(SW, 0x02800000u);
    expect_trap(7, 0x02800000u);
}

void test_b7_misaligned_device_access_is_an_access_fault(void) {
    run(LW, DEV_A + 2);
    expect_trap(5, DEV_A + 2);                                              // not cause 4
    run(SW, DEV_A + 2);
    expect_trap(7, DEV_A + 2);                                              // not cause 6
    expect_untouched();
}

void test_b8_atomics_on_a_device(void) {
    run(AMOADD_W, DEV_A);
    expect_trap(7, DEV_A);
    run(LR_W, DEV_A);
    expect_trap(5, DEV_A);
    run(SC_W, DEV_A);
    expect_trap(7, DEV_A);
    expect_untouched();
}

void test_b9_fetch_from_a_device(void) {
    cpu_write_csr(&cpu, CSR_MTVEC, MTVEC);
    cpu.pc = DEV_A;
    cpu_step(&cpu, &mem);
    expect_trap(1, DEV_A);
    expect_untouched();
}

// Sv32 on in S-mode: root table at RAM_BASE + 0x1000, level-0 table at RAM_BASE + 0x2000
#define ROOT (RAM_BASE + 0x1000u)
#define L0   (RAM_BASE + 0x2000u)

static void set_pte(uint32_t table, uint32_t index, uint32_t ppn, uint32_t flags) {
    mem_write32(&mem, table + index * 4, (ppn << 10) | flags);
}

static void enable_paging(void) {
    cpu.privilege_mode = PRV_S;
    cpu_write_csr(&cpu, CSR_SATP, 0x80000000u | (ROOT >> 12));
    set_pte(ROOT, 0, L0 >> 12, 0x01);
}

void test_b10_page_table_inside_a_device(void) {
    cpu.privilege_mode = PRV_S;
    cpu_write_csr(&cpu, CSR_SATP, 0x80000000u | (DEV_A >> 12));
    run(LW, 0x5000);
    expect_trap(5, 0x5000);
    expect_untouched();
}

void test_b11_translated_access_reaches_the_device(void) {
    enable_paging();
    set_pte(L0, 5, DEV_A >> 12, 0x01 | 0x02 | 0x04 | 0x40 | 0x80);        // V R W A D
    fake_a.read_value = 0x600DF00D;
    run(LW, 0x5010);
    TEST_ASSERT_EQUAL_HEX32(PC + 4, cpu.pc);
    TEST_ASSERT_EQUAL_HEX32(0x600DF00D, cpu_read_reg(&cpu, RD));
    TEST_ASSERT_EQUAL_UINT32(0x10, fake_a.last_offset);
    cpu_write_reg(&cpu, SRC, 0x11111111);
    run(SW, 0x5020);
    TEST_ASSERT_EQUAL_UINT32(0x20, fake_a.last_offset);
    TEST_ASSERT_EQUAL_HEX32(0x11111111, fake_a.last_value);
}

void test_b12_pmp_still_applies_to_a_device(void) {
    cpu.privilege_mode = PRV_S;
    cpu.pmpcfg[0] = 0x18 | 0x04;                                            // NAPOT, write only: no read
    cpu.pmpaddr[0] = (DEV_A >> 2) | 0x1F;                                   // 256 bytes at DEV_A
    cpu.pmpcfg[1] = 0x1F;
    cpu.pmpaddr[1] = 0x7FFFFFFF;
    run(LW, DEV_A);
    expect_trap(5, DEV_A);
    expect_untouched();
}

void test_b13_split_access_onto_a_device(void) {
    enable_paging();
    set_pte(L0, 4, (RAM_BASE + 0x3000u) >> 12, 0x01 | 0x02 | 0x04 | 0x40 | 0x80);   // va 0x4000 -> RAM
    set_pte(L0, 5, DEV_B >> 12, 0x01 | 0x02 | 0x04 | 0x40 | 0x80);                  // va 0x5000 -> device B,
                                                                                    // which takes 2-byte accesses
    mem_write8(&mem, RAM_BASE + 0x3FFE, 0xA2);
    mem_write8(&mem, RAM_BASE + 0x3FFF, 0xA3);
    cpu_write_reg(&cpu, SRC, 0x11223344);
    run(LW, 0x4FFE);
    expect_trap(5, 0x5000);
    cpu.privilege_mode = PRV_S;                                             // the trap took us to M-mode
    run(SW, 0x4FFE);
    expect_trap(7, 0x5000);
    TEST_ASSERT_EQUAL_HEX8(0xA2, mem_read8(&mem, RAM_BASE + 0x3FFE));       // the first half was not written
    TEST_ASSERT_EQUAL_HEX8(0xA3, mem_read8(&mem, RAM_BASE + 0x3FFF));
    expect_untouched();
}

void test_b14_misaligned_ram_access_still_works(void) {
    mem_write32(&mem, RAM_BASE + 0x500, 0x44332211);
    mem_write32(&mem, RAM_BASE + 0x504, 0x88776655);
    run(LW, RAM_BASE + 0x502);
    TEST_ASSERT_EQUAL_HEX32(PC + 4, cpu.pc);
    TEST_ASSERT_EQUAL_HEX32(0x66554433, cpu_read_reg(&cpu, RD));
}

/* ---- Idle time hooks ---- */

static uint64_t horizon_a = 500, horizon_b = 200;
static uint64_t idled_a, idled_b;
static uint64_t get_a(void *ctx) { (void)ctx; return horizon_a; }
static uint64_t get_b(void *ctx) { (void)ctx; return horizon_b; }
static void idle_a(void *ctx, uint64_t n) { (void)ctx; idled_a += n; }
static void idle_b(void *ctx, uint64_t n) { (void)ctx; idled_b += n; }

void test_idle_horizon_is_the_smallest_of_the_devices(void) {
    Memory m;
    mem_init_at(&m, 0x80000000u, 0x1000);
    TEST_ASSERT_EQUAL_UINT64(MEM_IDLE_NEVER, mem_idle_horizon(&m));          // no devices
    mem_add_device(&m, (Device){ .base = 0x04000000u, .size = 0x10, .sizes = DEV_ACCESS_32 });   // no hook: skipped
    TEST_ASSERT_EQUAL_UINT64(MEM_IDLE_NEVER, mem_idle_horizon(&m));
    mem_add_device(&m, (Device){ .base = 0x05000000u, .size = 0x10, .sizes = DEV_ACCESS_32, .idle_horizon = get_a });
    mem_add_device(&m, (Device){ .base = 0x06000000u, .size = 0x10, .sizes = DEV_ACCESS_32, .idle_horizon = get_b });
    TEST_ASSERT_EQUAL_UINT64(200, mem_idle_horizon(&m));
    horizon_b = MEM_IDLE_NEVER;
    TEST_ASSERT_EQUAL_UINT64(500, mem_idle_horizon(&m));
    horizon_b = 200;
    mem_free(&m);
}

void test_idle_calls_every_device_with_a_hook(void) {
    Memory m;
    mem_init_at(&m, 0x80000000u, 0x1000);
    mem_add_device(&m, (Device){ .base = 0x04000000u, .size = 0x10, .sizes = DEV_ACCESS_32 });
    mem_add_device(&m, (Device){ .base = 0x05000000u, .size = 0x10, .sizes = DEV_ACCESS_32, .idle = idle_a });
    mem_add_device(&m, (Device){ .base = 0x06000000u, .size = 0x10, .sizes = DEV_ACCESS_32, .idle = idle_b });
    idled_a = idled_b = 0;
    mem_idle(&m, 123);
    mem_idle(&m, 7);
    TEST_ASSERT_EQUAL_UINT64(130, idled_a);
    TEST_ASSERT_EQUAL_UINT64(130, idled_b);
    mem_free(&m);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_a1_ram_sizes_and_misalignment);
    RUN_TEST(test_a2_ram_three_bytes);
    RUN_TEST(test_a3_outside_ram_and_everything);
    RUN_TEST(test_a4_device_read);
    RUN_TEST(test_a5_device_write);
    RUN_TEST(test_a6_a_width_the_device_does_not_list);
    RUN_TEST(test_a7_device_with_all_widths);
    RUN_TEST(test_a8_misaligned_device_access);
    RUN_TEST(test_a9_straddling_the_end_of_a_device);
    RUN_TEST(test_a10_unmapped_address);
    RUN_TEST(test_a11_the_device_refuses);
    RUN_TEST(test_a12_read_only_and_write_only_devices);
    RUN_TEST(test_a13_writable_touches_nothing);
    RUN_TEST(test_a14_in_ram);
    RUN_TEST(test_a15_a_new_memory_has_no_devices);
    RUN_TEST(test_b1_lw_from_a_device);
    RUN_TEST(test_b2_narrow_loads_from_a_device);
    RUN_TEST(test_b3_a_width_the_device_does_not_take);
    RUN_TEST(test_b4_a_store_never_reads_the_device);
    RUN_TEST(test_b5_a_refused_store);
    RUN_TEST(test_b6_unmapped_addresses);
    RUN_TEST(test_b7_misaligned_device_access_is_an_access_fault);
    RUN_TEST(test_b8_atomics_on_a_device);
    RUN_TEST(test_b9_fetch_from_a_device);
    RUN_TEST(test_b10_page_table_inside_a_device);
    RUN_TEST(test_b11_translated_access_reaches_the_device);
    RUN_TEST(test_b12_pmp_still_applies_to_a_device);
    RUN_TEST(test_b13_split_access_onto_a_device);
    RUN_TEST(test_b14_misaligned_ram_access_still_works);
    RUN_TEST(test_idle_horizon_is_the_smallest_of_the_devices);
    RUN_TEST(test_idle_calls_every_device_with_a_hook);
    return UNITY_END();
}
