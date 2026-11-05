#include <stdio.h>
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

/* ---- pmp_check ---------------------------------------------------------------------------- */

#define CFG_NAPOT 0x18u
#define CFG_TOR   0x08u
#define CFG_NA4   0x10u
#define CFG_L     0x80u

/* pmpaddr for a NAPOT region [base, base + size), size a power of two >= 8 */
static uint32_t napot(uint32_t base, uint32_t size) {
    return (base >> 2) | ((size >> 3) - 1);
}

static void set_entry(CPU *c, unsigned i, uint8_t cfg, uint32_t addr) {
    c->pmpcfg[i] = cfg;
    c->pmpaddr[i] = addr;
}

static const PmpAccess ALL_ACCESSES[] = {PMP_ACCESS_READ, PMP_ACCESS_WRITE, PMP_ACCESS_EXEC};
static const PrivMode  ALL_MODES[]    = {PRV_U, PRV_S, PRV_M};

static bool allowed(const CPU *c, uint32_t addr, uint32_t size, PmpAccess a, PrivMode m) {
    return pmp_check(c, addr, size, a, m);
}

void test_pmp_check_with_no_entries(void) {
    CPU c; cpu_reset(&c);
    const uint32_t sizes[] = {1, 2, 4};
    for (unsigned s = 0; s < 3; s++) {
        for (unsigned a = 0; a < 3; a++) {
            TEST_ASSERT_TRUE(allowed(&c, 0x1000, sizes[s], ALL_ACCESSES[a], PRV_M));   // M succeeds
            TEST_ASSERT_FALSE(allowed(&c, 0x1000, sizes[s], ALL_ACCESSES[a], PRV_S));  // S and U fail
            TEST_ASSERT_FALSE(allowed(&c, 0x1000, sizes[s], ALL_ACCESSES[a], PRV_U));
        }
    }
}

/* One NAPOT region [0x1000, 0x2000); an access of 4 bytes at 0x1000, inside it. */
void test_pmp_check_explicit_permission_rows(void) {
    const struct { uint8_t perms_and_l; PrivMode mode; PmpAccess access; bool ok; const char *why; } v[] = {
        {0x01, PRV_U, PMP_ACCESS_READ,  true,  "R: U reads"},
        {0x01, PRV_U, PMP_ACCESS_WRITE, false, "R: U cannot write"},
        {0x01, PRV_U, PMP_ACCESS_EXEC,  false, "R: U cannot execute"},
        {0x04, PRV_S, PMP_ACCESS_EXEC,  true,  "X: S executes"},
        {0x04, PRV_S, PMP_ACCESS_READ,  false, "X: S cannot read"},
        {0x03, PRV_U, PMP_ACCESS_WRITE, true,  "RW: U writes"},
        {0x03, PRV_U, PMP_ACCESS_EXEC,  false, "RW: U cannot execute"},
        {0x01, PRV_M, PMP_ACCESS_WRITE, true,  "L clear: M ignores the permissions"},
        {0x00, PRV_M, PMP_ACCESS_READ,  true,  "L clear, no permissions: M still reads"},
        {0x81, PRV_M, PMP_ACCESS_READ,  true,  "L set, R: M reads"},
        {0x81, PRV_M, PMP_ACCESS_WRITE, false, "L set, R: M cannot write"},
        {0x80, PRV_M, PMP_ACCESS_READ,  false, "L set, no permissions: M cannot read"},
    };
    for (unsigned i = 0; i < sizeof v / sizeof v[0]; i++) {
        CPU c; cpu_reset(&c);
        set_entry(&c, 0, CFG_NAPOT | v[i].perms_and_l, napot(0x1000, 0x1000));
        TEST_ASSERT_EQUAL_INT_MESSAGE(v[i].ok, allowed(&c, 0x1000, 4, v[i].access, v[i].mode), v[i].why);
    }
}

/* Every legal permission set (the reserved W-only sets are skipped), every access, mode and L. */
void test_pmp_check_every_permission_combination(void) {
    const uint8_t perms[] = {0x0, 0x1, 0x3, 0x4, 0x5, 0x7};
    const uint8_t bit_of[] = {0x1, 0x2, 0x4};                 /* read, write, execute */
    for (unsigned p = 0; p < 6; p++) {
        for (unsigned l = 0; l < 2; l++) {
            CPU c; cpu_reset(&c);
            set_entry(&c, 0, CFG_NAPOT | perms[p] | (l ? CFG_L : 0), napot(0x1000, 0x1000));
            for (unsigned m = 0; m < 3; m++) {
                for (unsigned a = 0; a < 3; a++) {
                    bool expected = (ALL_MODES[m] == PRV_M && !l) || (perms[p] & bit_of[a]);
                    char msg[64];
                    snprintf(msg, sizeof msg, "perms 0x%x L %u mode %d access %u", perms[p], l, (int)ALL_MODES[m], a);
                    TEST_ASSERT_EQUAL_INT_MESSAGE(expected, allowed(&c, 0x1000, 4, ALL_ACCESSES[a], ALL_MODES[m]), msg);
                }
            }
        }
    }
}

/* Region [0x1000, 0x2000) with RWX, reads. A partial match fails for every mode, M with L clear too. */
void test_pmp_check_region_boundaries(void) {
    const struct { const char *name; uint32_t addr, size; bool user, machine; } v[] = {
        {"start",                 0x1000, 4, true,  true },
        {"ends exactly at the top", 0x1FFC, 4, true,  true },
        {"last byte",             0x1FFF, 1, true,  true },
        {"partial: past the top", 0x1FFD, 4, false, false},
        {"partial: below the base", 0x0FFD, 4, false, false},
        {"2 bytes from the last byte", 0x1FFF, 2, false, false},
        {"just below, no match",  0x0FFC, 4, false, true },
        {"byte after, no match",  0x2000, 1, false, true },
    };
    CPU c; cpu_reset(&c);
    set_entry(&c, 0, CFG_NAPOT | PMP_RWX, napot(0x1000, 0x1000));
    for (unsigned i = 0; i < sizeof v / sizeof v[0]; i++) {
        TEST_ASSERT_EQUAL_INT_MESSAGE(v[i].user,    allowed(&c, v[i].addr, v[i].size, PMP_ACCESS_READ, PRV_U), v[i].name);
        TEST_ASSERT_EQUAL_INT_MESSAGE(v[i].user,    allowed(&c, v[i].addr, v[i].size, PMP_ACCESS_READ, PRV_S), v[i].name);
        TEST_ASSERT_EQUAL_INT_MESSAGE(v[i].machine, allowed(&c, v[i].addr, v[i].size, PMP_ACCESS_READ, PRV_M), v[i].name);
    }
}

void test_pmp_check_lowest_entry_wins(void) {
    CPU c; cpu_reset(&c);
    set_entry(&c, 0, CFG_NA4 | 0x00, 0x1000 >> 2);                       /* NA4 at 0x1000, no permissions */
    set_entry(&c, 1, CFG_NAPOT | PMP_RWX, napot(0x1000, 0x1000));        /* the whole page, RWX */
    TEST_ASSERT_FALSE(allowed(&c, 0x1000, 4, PMP_ACCESS_READ, PRV_U));   /* entry 0 decides: denied */
    TEST_ASSERT_TRUE (allowed(&c, 0x1004, 4, PMP_ACCESS_READ, PRV_U));   /* entry 0 does not match; entry 1 allows */
    TEST_ASSERT_FALSE(allowed(&c, 0x1000, 8, PMP_ACCESS_READ, PRV_U));   /* entry 0 matches part: denied, though entry 1 covers all */

    CPU d; cpu_reset(&d);
    set_entry(&d, 0, CFG_NA4 | PMP_RWX, 0x1000 >> 2);                    /* NA4 at 0x1000, RWX */
    set_entry(&d, 1, CFG_NAPOT | 0x00, napot(0x1000, 0x1000));           /* the whole page, no permissions */
    TEST_ASSERT_TRUE (allowed(&d, 0x1000, 4, PMP_ACCESS_READ, PRV_U));   /* entry 0 allows */
    TEST_ASSERT_FALSE(allowed(&d, 0x1004, 4, PMP_ACCESS_READ, PRV_U));   /* entry 1 denies */
}

void test_pmp_check_disabled_and_empty_entries_do_not_match(void) {
    CPU c; cpu_reset(&c);
    set_entry(&c, 0, 0x00 | PMP_RWX, napot(0x1000, 0x1000));             /* OFF: address set, permissions set */
    set_entry(&c, 1, CFG_NAPOT | PMP_RWX, napot(0x1000, 0x1000));
    TEST_ASSERT_TRUE(allowed(&c, 0x1000, 4, PMP_ACCESS_WRITE, PRV_U));

    CPU d; cpu_reset(&d);
    set_entry(&d, 0, CFG_TOR | PMP_RWX, 0);                              /* TOR with top 0: empty */
    TEST_ASSERT_FALSE(allowed(&d, 0x0, 4, PMP_ACCESS_READ, PRV_U));      /* no match: U denied */
}

void test_pmp_check_tor_regions(void) {
    CPU c; cpu_reset(&c);
    set_entry(&c, 0, CFG_TOR | PMP_R, 0x400);                            /* [0, 0x1000), read only */
    set_entry(&c, 1, CFG_TOR | PMP_R | PMP_W, 0x800);                    /* [0x1000, 0x2000), read and write */
    TEST_ASSERT_TRUE (allowed(&c, 0xFFC,  4, PMP_ACCESS_READ,  PRV_U));
    TEST_ASSERT_FALSE(allowed(&c, 0xFFC,  4, PMP_ACCESS_WRITE, PRV_U));
    TEST_ASSERT_TRUE (allowed(&c, 0x1000, 4, PMP_ACCESS_WRITE, PRV_U));
    /* straddles both: entry 0 matches first but not all, so it fails although both allow a read */
    TEST_ASSERT_FALSE(allowed(&c, 0xFFE, 4, PMP_ACCESS_READ, PRV_U));
}

void test_pmp_check_access_types_stay_separate(void) {
    CPU c; cpu_reset(&c);
    set_entry(&c, 0, CFG_NAPOT | PMP_X, napot(0x1000, 0x1000));
    const PrivMode lower[] = {PRV_S, PRV_U};
    for (unsigned m = 0; m < 2; m++) {
        TEST_ASSERT_TRUE (allowed(&c, 0x1000, 4, PMP_ACCESS_EXEC,  lower[m]));
        TEST_ASSERT_FALSE(allowed(&c, 0x1000, 4, PMP_ACCESS_READ,  lower[m]));
        TEST_ASSERT_FALSE(allowed(&c, 0x1000, 4, PMP_ACCESS_WRITE, lower[m]));
    }
}

void test_pmp_check_high_addresses(void) {
    CPU c; cpu_reset(&c);
    set_entry(&c, 0, CFG_NAPOT | PMP_RWX, napot(0x80000000u, 0x10000));
    TEST_ASSERT_TRUE (allowed(&c, 0x80000000u, 4, PMP_ACCESS_READ, PRV_U));
    TEST_ASSERT_TRUE (allowed(&c, 0x8000FFFCu, 4, PMP_ACCESS_READ, PRV_U));
    TEST_ASSERT_FALSE(allowed(&c, 0x80010000u, 4, PMP_ACCESS_READ, PRV_U));

    /* the riscv-tests harness entry: NAPOT, RWX, [0, 2^34) */
    CPU h; cpu_reset(&h);
    set_entry(&h, 0, 0x1F, 0x7FFFFFFF);
    TEST_ASSERT_TRUE(allowed(&h, 0xFFFFFFFCu, 4, PMP_ACCESS_READ, PRV_U));
    TEST_ASSERT_TRUE(allowed(&h, 0xFFFFFFFEu, 4, PMP_ACCESS_READ, PRV_U));   /* reaches 0x100000002 */
    CPU n; cpu_reset(&n);
    TEST_ASSERT_FALSE(allowed(&n, 0xFFFFFFFEu, 4, PMP_ACCESS_READ, PRV_U));  /* no entry: denied */
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_pmp_range_table);
    RUN_TEST(test_pmp_entry_0_tor_starts_at_zero);
    RUN_TEST(test_pmp_tor_ignores_the_previous_entrys_config);
    RUN_TEST(test_pmp_entry_15_tor_uses_pmpaddr14);
    RUN_TEST(test_pmp_entry_range_of_an_off_entry_is_none);
    RUN_TEST(test_pmp_entry_range_beyond_the_implemented_entries_is_none);
    RUN_TEST(test_pmp_check_with_no_entries);
    RUN_TEST(test_pmp_check_explicit_permission_rows);
    RUN_TEST(test_pmp_check_every_permission_combination);
    RUN_TEST(test_pmp_check_region_boundaries);
    RUN_TEST(test_pmp_check_lowest_entry_wins);
    RUN_TEST(test_pmp_check_disabled_and_empty_entries_do_not_match);
    RUN_TEST(test_pmp_check_tor_regions);
    RUN_TEST(test_pmp_check_access_types_stay_separate);
    RUN_TEST(test_pmp_check_high_addresses);
    return UNITY_END();
}
