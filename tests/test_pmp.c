#include "unity.h"
#include "../src/pmp.h"

void setUp(void) {}
void tearDown(void) {}

/* ---- pmp_range: (cfg, pmpaddr, previous pmpaddr) -> byte range, or none --------------------- */

void test_pmp_range_table(void) {
    const struct {
        const char *name;
        uint8_t cfg; uint32_t addr, prev;
        bool matches; uint64_t lo, hi;
    } v[] = {
        {"OFF",                         0x00, 0x00001234, 0, false, 0, 0},
        {"OFF with permissions",        0x07, 0x00001234, 0, false, 0, 0},
        {"OFF with L",                  0x80, 0x00001234, 0, false, 0, 0},

        {"NA4",                         0x10, 0x20000010, 0, true, 0x80000040ull, 0x80000044ull},
        {"NA4 with RWX and L",          0x97, 0x20000010, 0, true, 0x80000040ull, 0x80000044ull},
        {"NA4, top of the space",       0x10, 0xFFFFFFFF, 0, true, 0x3FFFFFFFCull, 0x400000000ull},

        {"NAPOT 8 bytes",               0x18, 0x00000000, 0, true, 0, 8},
        {"NAPOT 16 bytes",              0x18, 0x00000001, 0, true, 0, 16},
        {"NAPOT 32 bytes",              0x18, 0x00000003, 0, true, 0, 32},
        {"NAPOT 32 bytes at 0x80000000", 0x18, 0x20000003, 0, true, 0x80000000ull, 0x80000020ull},
        {"NAPOT 64 bytes at 0x80000000", 0x18, 0x20000007, 0, true, 0x80000000ull, 0x80000040ull},
        {"NAPOT, base has bits above the size", 0x18, 0x2000000B, 0, true, 0x80000020ull, 0x80000040ull},
        {"NAPOT 2^33",                  0x18, 0x3FFFFFFF, 0, true, 0, 0x200000000ull},
        {"NAPOT 2^34 (the harness)",    0x1F, 0x7FFFFFFF, 0, true, 0, 0x400000000ull},
        {"NAPOT 2^35",                  0x18, 0xFFFFFFFF, 0, true, 0, 0x800000000ull},

        {"TOR",                         0x08, 0x00000200, 0x00000100, true, 0x400, 0x800},
        {"TOR with L and RWX",          0x8F, 0x00000200, 0x00000100, true, 0x400, 0x800},
        {"TOR, bottom 0",               0x08, 0x00000001, 0x00000000, true, 0, 4},
        {"TOR, reversed",               0x08, 0x00000100, 0x00000200, false, 0, 0},
        {"TOR, equal",                  0x08, 0x00000100, 0x00000100, false, 0, 0},
        {"TOR, top 0",                  0x08, 0x00000000, 0x00000000, false, 0, 0},
        {"TOR, high addresses",         0x08, 0x20000010, 0x20000000, true, 0x80000000ull, 0x80000040ull},
        {"TOR, largest",                0x08, 0xFFFFFFFF, 0x00000000, true, 0, 0x3FFFFFFFCull},
    };
    for (unsigned i = 0; i < sizeof v / sizeof v[0]; i++) {
        PmpRange r = {0xDEAD, 0xBEEF};
        bool matches = pmp_range(v[i].cfg, v[i].addr, v[i].prev, &r);
        TEST_ASSERT_EQUAL_INT_MESSAGE(v[i].matches, matches, v[i].name);
        if (v[i].matches) {
            TEST_ASSERT_EQUAL_UINT64_MESSAGE(v[i].lo, r.lo, v[i].name);
            TEST_ASSERT_EQUAL_UINT64_MESSAGE(v[i].hi, r.hi, v[i].name);
        }
    }
}

/* ---- pmp_entry_range: the same, from a CPU's entries ---------------------------------------- */

void test_pmp_entry_0_tor_starts_at_zero(void) {
    CPU c; cpu_reset(&c);
    c.pmpcfg[0] = 0x08;                 // TOR
    c.pmpaddr[0] = 0x100;
    PmpRange r;
    TEST_ASSERT_TRUE(pmp_entry_range(&c, 0, &r));
    TEST_ASSERT_EQUAL_UINT64(0, r.lo);
    TEST_ASSERT_EQUAL_UINT64(0x400, r.hi);
}

/* TOR uses the previous pmpaddr whatever the previous entry's own config is. */
void test_pmp_tor_ignores_the_previous_entrys_config(void) {
    const uint8_t previous_cfg[] = {0x00, 0x1F, 0x9F, 0x10};   /* OFF, NAPOT RWX, locked NAPOT, NA4 */
    for (unsigned i = 0; i < sizeof previous_cfg; i++) {
        CPU c; cpu_reset(&c);
        c.pmpcfg[0] = previous_cfg[i];
        c.pmpaddr[0] = 0x100;
        c.pmpcfg[1] = 0x08;             // TOR
        c.pmpaddr[1] = 0x200;
        PmpRange r;
        TEST_ASSERT_TRUE(pmp_entry_range(&c, 1, &r));
        TEST_ASSERT_EQUAL_UINT64(0x400, r.lo);
        TEST_ASSERT_EQUAL_UINT64(0x800, r.hi);
    }
}

void test_pmp_entry_15_tor_uses_pmpaddr14(void) {
    CPU c; cpu_reset(&c);
    c.pmpaddr[14] = 0x100;
    c.pmpcfg[15] = 0x08;
    c.pmpaddr[15] = 0x180;
    PmpRange r;
    TEST_ASSERT_TRUE(pmp_entry_range(&c, 15, &r));
    TEST_ASSERT_EQUAL_UINT64(0x400, r.lo);
    TEST_ASSERT_EQUAL_UINT64(0x600, r.hi);
}

void test_pmp_entry_range_of_an_off_entry_is_none(void) {
    CPU c; cpu_reset(&c);
    c.pmpaddr[3] = 0x1234;              // address set, but A = OFF
    PmpRange r;
    TEST_ASSERT_FALSE(pmp_entry_range(&c, 3, &r));
}

void test_pmp_entry_range_beyond_the_implemented_entries_is_none(void) {
    CPU c; cpu_reset(&c);
    /* Make a read past the end of pmpcfg[] look like a NAPOT entry (the byte after the array is the
     * low byte of the next field on this layout), so a missing bounds check cannot hide as OFF. */
    c.pmpaddr[0] = 0x18;
    PmpRange r;
    TEST_ASSERT_FALSE(pmp_entry_range(&c, 16, &r));
    TEST_ASSERT_FALSE(pmp_entry_range(&c, 17, &r));
    TEST_ASSERT_FALSE(pmp_entry_range(&c, 63, &r));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_pmp_range_table);
    RUN_TEST(test_pmp_entry_0_tor_starts_at_zero);
    RUN_TEST(test_pmp_tor_ignores_the_previous_entrys_config);
    RUN_TEST(test_pmp_entry_15_tor_uses_pmpaddr14);
    RUN_TEST(test_pmp_entry_range_of_an_off_entry_is_none);
    RUN_TEST(test_pmp_entry_range_beyond_the_implemented_entries_is_none);
    return UNITY_END();
}
