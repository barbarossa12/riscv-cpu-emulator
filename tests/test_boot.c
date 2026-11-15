#include <stdio.h>
#include <string.h>
#include "unity.h"
#include "../src/boot.h"

#define RAM_BASE 0x80000000u
#define RAM_SIZE 0x100000u
#define PATH     "test_boot_tmp.bin"

static Memory mem;
static BootLayout layout;

void setUp(void) {
    mem_init_at(&mem, RAM_BASE, RAM_SIZE);
    memset(mem.buffer, 0xAA, RAM_SIZE);
    boot_layout_init(&layout);
}

void tearDown(void) {
    remove(PATH);
    mem_free(&mem);
}

static void write_file(const uint8_t *data, size_t len) {
    FILE *f = fopen(PATH, "wb");
    TEST_ASSERT_NOT_NULL(f);
    if (len) TEST_ASSERT_EQUAL_size_t(len, fwrite(data, 1, len, f));
    fclose(f);
}

static void assert_ram_untouched(void) {
    for (size_t i = 0; i < RAM_SIZE; i++) {
        if (mem.buffer[i] != 0xAA) {
            char msg[64];
            snprintf(msg, sizeof msg, "RAM was written at offset 0x%zx", i);
            TEST_FAIL_MESSAGE(msg);
        }
    }
}

static void put32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24);
}

static void put16(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8);
}

/* A minimal RV32 executable ELF with n loadable segments: vaddr[i], filesz[i], memsz[i]. The file bytes of segment i are 0x40 + i.
 * No section headers. Returns the file length. */
static size_t make_elf(uint8_t *e, uint32_t entry, int n, const uint32_t *vaddr, const uint32_t *filesz, const uint32_t *memsz) {
    memset(e, 0, 4096);
    memcpy(e, "\x7f" "ELF", 4);
    e[4] = 1; e[5] = 1; e[6] = 1;
    put16(e + 16, 2);                       // ET_EXEC
    put16(e + 18, 243);                     // EM_RISCV
    put32(e + 20, 1);
    put32(e + 24, entry);
    put32(e + 28, 52);                      // phoff
    put16(e + 40, 52);
    put16(e + 42, 32);                      // phentsize
    put16(e + 44, (uint16_t)n);
    put16(e + 46, 40);
    uint32_t off = 52 + 32 * (uint32_t)n;
    for (int i = 0; i < n; i++) {
        uint8_t *ph = e + 52 + 32 * i;
        put32(ph + 0, 1);                   // PT_LOAD
        put32(ph + 4, off);
        put32(ph + 8, vaddr[i]);
        put32(ph + 12, vaddr[i]);
        put32(ph + 16, filesz[i]);
        put32(ph + 20, memsz[i]);
        put32(ph + 24, 5);
        memset(e + off, 0x40 + i, filesz[i]);
        off += filesz[i];
    }
    return off;
}

static size_t one_segment_elf(uint8_t *e, uint32_t entry, uint32_t vaddr, uint32_t filesz, uint32_t memsz) {
    return make_elf(e, entry, 1, &vaddr, &filesz, &memsz);
}

/* A device tree blob: the magic, totalsize and zeros, file_len bytes long. */
static void make_dtb(uint8_t *d, size_t file_len, uint32_t magic, uint32_t total) {
    memset(d, 0x77, file_len);
    d[0] = (uint8_t)(magic >> 24); d[1] = (uint8_t)(magic >> 16); d[2] = (uint8_t)(magic >> 8); d[3] = (uint8_t)magic;
    d[4] = (uint8_t)(total >> 24); d[5] = (uint8_t)(total >> 16); d[6] = (uint8_t)(total >> 8); d[7] = (uint8_t)total;
}

/* ---- Flat binaries ------------------------------------------------------------------------------------------------ */

void test_f1_a_flat_file_lands_at_the_address(void) {
    uint8_t d[16];
    for (int i = 0; i < 16; i++) d[i] = (uint8_t)(1 + i);
    write_file(d, sizeof d);
    TEST_ASSERT_EQUAL_INT(BOOT_OK, boot_load_flat(&mem, &layout, PATH, "firmware", RAM_BASE + 0x100));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(d, mem.buffer + 0x100, 16);
    TEST_ASSERT_EQUAL_UINT8(0xAA, mem.buffer[0xFF]);
    TEST_ASSERT_EQUAL_UINT8(0xAA, mem.buffer[0x110]);
    TEST_ASSERT_EQUAL_INT(1, layout.count);
    TEST_ASSERT_EQUAL_STRING("firmware", layout.regions[0].name);
    TEST_ASSERT_EQUAL_HEX32(RAM_BASE + 0x100, layout.regions[0].start);
    TEST_ASSERT_EQUAL_HEX32(RAM_BASE + 0x110, layout.regions[0].end);
}

void test_f2_a_file_ending_exactly_at_the_end_of_ram_fits(void) {
    uint8_t d[16] = {1};
    write_file(d, sizeof d);
    TEST_ASSERT_EQUAL_INT(BOOT_OK, boot_load_flat(&mem, &layout, PATH, "x", RAM_BASE + RAM_SIZE - 16));
    TEST_ASSERT_EQUAL_UINT8(1, mem.buffer[RAM_SIZE - 16]);
}

void test_f3_one_byte_past_the_end_of_ram_does_not(void) {
    uint8_t d[16] = {1};
    write_file(d, sizeof d);
    TEST_ASSERT_EQUAL_INT(BOOT_ERR_RANGE, boot_load_flat(&mem, &layout, PATH, "x", RAM_BASE + RAM_SIZE - 15));
    assert_ram_untouched();
    TEST_ASSERT_EQUAL_INT(0, layout.count);
}

void test_f4_below_and_above_ram_are_out_of_range(void) {
    uint8_t d[4] = {1, 2, 3, 4};
    write_file(d, sizeof d);
    TEST_ASSERT_EQUAL_INT(BOOT_ERR_RANGE, boot_load_flat(&mem, &layout, PATH, "x", RAM_BASE - 4));
    TEST_ASSERT_EQUAL_INT(BOOT_ERR_RANGE, boot_load_flat(&mem, &layout, PATH, "x", 0));
    TEST_ASSERT_EQUAL_INT(BOOT_ERR_RANGE, boot_load_flat(&mem, &layout, PATH, "x", RAM_BASE + RAM_SIZE));
    TEST_ASSERT_EQUAL_INT(BOOT_ERR_RANGE, boot_load_flat(&mem, &layout, PATH, "x", 0xFFFFFFFCu));   // the end wraps past 2^32
    assert_ram_untouched();
}

void test_f5_an_empty_file_has_nothing_to_load(void) {
    write_file(NULL, 0);
    TEST_ASSERT_EQUAL_INT(BOOT_ERR_EMPTY, boot_load_flat(&mem, &layout, PATH, "x", RAM_BASE));
    assert_ram_untouched();
}

void test_f6_an_elf_given_as_flat_is_refused(void) {
    uint8_t e[4096];
    size_t len = one_segment_elf(e, RAM_BASE, RAM_BASE, 16, 16);
    write_file(e, len);
    TEST_ASSERT_EQUAL_INT(BOOT_ERR_LOOKS_LIKE_ELF, boot_load_flat(&mem, &layout, PATH, "x", RAM_BASE));
    assert_ram_untouched();
}

void test_f7_the_elf_magic_needs_all_four_bytes(void) {
    const uint8_t three[3] = { 0x7f, 'E', 'L' };
    write_file(three, sizeof three);
    TEST_ASSERT_EQUAL_INT(BOOT_OK, boot_load_flat(&mem, &layout, PATH, "x", RAM_BASE));
    const uint8_t near[4] = { 0x7f, 'E', 'L', 'G' };
    write_file(near, sizeof near);
    TEST_ASSERT_EQUAL_INT(BOOT_OK, boot_load_flat(&mem, &layout, PATH, "y", RAM_BASE + 0x10));
}

void test_f8_a_missing_file_is_an_io_error(void) {
    TEST_ASSERT_EQUAL_INT(BOOT_ERR_IO, boot_load_flat(&mem, &layout, "no_such_boot_image.bin", "x", RAM_BASE));
}

void test_f9_overlap_is_refused_and_adjacent_is_fine(void) {
    uint8_t d[16] = {1};
    write_file(d, sizeof d);
    TEST_ASSERT_EQUAL_INT(BOOT_OK, boot_load_flat(&mem, &layout, PATH, "a", RAM_BASE + 0x100));
    uint8_t before = mem.buffer[0x108];
    d[0] = 9;
    write_file(d, sizeof d);
    TEST_ASSERT_EQUAL_INT(BOOT_ERR_OVERLAP, boot_load_flat(&mem, &layout, PATH, "b", RAM_BASE + 0x108));   // second half overlaps
    TEST_ASSERT_EQUAL_INT(BOOT_ERR_OVERLAP, boot_load_flat(&mem, &layout, PATH, "b", RAM_BASE + 0xF8));    // first half overlaps
    TEST_ASSERT_EQUAL_INT(BOOT_ERR_OVERLAP, boot_load_flat(&mem, &layout, PATH, "b", RAM_BASE + 0x100));   // the same place
    TEST_ASSERT_EQUAL_UINT8(before, mem.buffer[0x108]);
    TEST_ASSERT_EQUAL_INT(1, layout.count);
    TEST_ASSERT_EQUAL_INT(BOOT_OK, boot_load_flat(&mem, &layout, PATH, "c", RAM_BASE + 0x110));            // starts where "a" ends
    TEST_ASSERT_EQUAL_INT(BOOT_OK, boot_load_flat(&mem, &layout, PATH, "d", RAM_BASE + 0xF0));             // ends where "a" starts
}

void test_f10_at_most_eight_images(void) {
    uint8_t d[1] = {1};
    write_file(d, sizeof d);
    for (int i = 0; i < BOOT_MAX_REGIONS; i++) {
        TEST_ASSERT_EQUAL_INT(BOOT_OK, boot_load_flat(&mem, &layout, PATH, "x", RAM_BASE + 0x10 * (uint32_t)i));
    }
    TEST_ASSERT_EQUAL_INT(BOOT_ERR_TOO_MANY, boot_load_flat(&mem, &layout, PATH, "x", RAM_BASE + 0x1000));
    TEST_ASSERT_EQUAL_UINT8(0xAA, mem.buffer[0x1000]);
}

/* ---- ELF images ---------------------------------------------------------------------------------------------------- */

void test_e1_an_elf_lands_at_its_own_addresses(void) {
    uint8_t e[4096];
    size_t len = one_segment_elf(e, RAM_BASE + 0x40, RAM_BASE + 0x200, 16, 16);
    write_file(e, len);
    uint32_t entry = 0;
    TEST_ASSERT_EQUAL_INT(BOOT_OK, boot_load_elf(&mem, &layout, PATH, "kernel", &entry));
    TEST_ASSERT_EQUAL_HEX32(RAM_BASE + 0x40, entry);
    TEST_ASSERT_EQUAL_UINT8(0x40, mem.buffer[0x200]);
    TEST_ASSERT_EQUAL_UINT8(0x40, mem.buffer[0x20F]);
    TEST_ASSERT_EQUAL_UINT8(0xAA, mem.buffer[0x1FF]);
    TEST_ASSERT_EQUAL_UINT8(0xAA, mem.buffer[0x210]);
    TEST_ASSERT_EQUAL_INT(1, layout.count);
    TEST_ASSERT_EQUAL_HEX32(RAM_BASE + 0x200, layout.regions[0].start);
    TEST_ASSERT_EQUAL_HEX32(RAM_BASE + 0x210, layout.regions[0].end);
}

void test_e2_bss_is_zeroed_and_counted(void) {
    uint8_t e[4096];
    size_t len = one_segment_elf(e, RAM_BASE, RAM_BASE, 8, 24);
    write_file(e, len);
    uint32_t entry;
    TEST_ASSERT_EQUAL_INT(BOOT_OK, boot_load_elf(&mem, &layout, PATH, "fw", &entry));
    TEST_ASSERT_EQUAL_UINT8(0x40, mem.buffer[7]);
    TEST_ASSERT_EQUAL_UINT8(0, mem.buffer[8]);
    TEST_ASSERT_EQUAL_UINT8(0, mem.buffer[23]);
    TEST_ASSERT_EQUAL_UINT8(0xAA, mem.buffer[24]);
    TEST_ASSERT_EQUAL_HEX32(RAM_BASE + 24, layout.regions[0].end);
}

void test_e3_a_flat_file_given_as_an_elf_is_a_format_error(void) {
    uint8_t d[64] = {1};
    write_file(d, sizeof d);
    uint32_t entry;
    TEST_ASSERT_EQUAL_INT(BOOT_ERR_FORMAT, boot_load_elf(&mem, &layout, PATH, "x", &entry));
    assert_ram_untouched();
    TEST_ASSERT_EQUAL_INT(0, layout.count);
}

void test_e4_a_segment_outside_ram_is_out_of_range(void) {
    uint8_t e[4096];
    size_t len = one_segment_elf(e, RAM_BASE, RAM_BASE + RAM_SIZE - 8, 16, 16);
    write_file(e, len);
    uint32_t entry;
    TEST_ASSERT_EQUAL_INT(BOOT_ERR_RANGE, boot_load_elf(&mem, &layout, PATH, "x", &entry));
    assert_ram_untouched();
    TEST_ASSERT_EQUAL_INT(0, layout.count);
}

void test_e5_an_elf_overlapping_an_earlier_image_is_refused_and_writes_nothing(void) {
    uint8_t d[16] = {1};
    write_file(d, sizeof d);
    TEST_ASSERT_EQUAL_INT(BOOT_OK, boot_load_flat(&mem, &layout, PATH, "firmware", RAM_BASE + 0x100));
    uint8_t e[4096];
    size_t len = one_segment_elf(e, RAM_BASE, RAM_BASE + 0x108, 16, 16);
    write_file(e, len);
    uint32_t entry;
    TEST_ASSERT_EQUAL_INT(BOOT_ERR_OVERLAP, boot_load_elf(&mem, &layout, PATH, "kernel", &entry));
    TEST_ASSERT_EQUAL_UINT8(0xAA, mem.buffer[0x110]);                        // nothing of the ELF was written
    TEST_ASSERT_EQUAL_INT(1, layout.count);
}

void test_e6_the_gap_between_segments_counts_as_used(void) {
    const uint32_t vaddr[2] = { RAM_BASE, RAM_BASE + 0x1000 };
    const uint32_t size[2]  = { 16, 16 };
    uint8_t e[4096];
    size_t len = make_elf(e, RAM_BASE, 2, vaddr, size, size);
    write_file(e, len);
    uint32_t entry;
    TEST_ASSERT_EQUAL_INT(BOOT_OK, boot_load_elf(&mem, &layout, PATH, "fw", &entry));
    TEST_ASSERT_EQUAL_HEX32(RAM_BASE + 0x1010, layout.regions[0].end);
    uint8_t d[16] = {1};
    write_file(d, sizeof d);
    TEST_ASSERT_EQUAL_INT(BOOT_ERR_OVERLAP, boot_load_flat(&mem, &layout, PATH, "x", RAM_BASE + 0x800));
}

void test_e7_an_elf_with_no_loadable_segment_has_nothing_to_load(void) {
    uint8_t e[4096];
    size_t len = make_elf(e, RAM_BASE, 0, NULL, NULL, NULL);
    write_file(e, len);
    uint32_t entry;
    TEST_ASSERT_EQUAL_INT(BOOT_ERR_EMPTY, boot_load_elf(&mem, &layout, PATH, "x", &entry));
}

void test_e8_a_missing_elf_is_an_io_error(void) {
    uint32_t entry;
    TEST_ASSERT_EQUAL_INT(BOOT_ERR_IO, boot_load_elf(&mem, &layout, "no_such_boot_image.elf", "x", &entry));
}

void test_e9_the_entry_is_only_set_on_success(void) {
    uint8_t d[64] = {1};
    write_file(d, sizeof d);
    uint32_t entry = 0x12345678;
    TEST_ASSERT_EQUAL_INT(BOOT_ERR_FORMAT, boot_load_elf(&mem, &layout, PATH, "x", &entry));
    TEST_ASSERT_EQUAL_HEX32(0x12345678, entry);
}

/* ---- Device trees ----------------------------------------------------------------------------------------------- */

void test_d1_a_good_dtb_copies_exactly_totalsize_bytes(void) {
    uint8_t d[96];
    make_dtb(d, sizeof d, 0xd00dfeedu, 64);                                  // 32 junk bytes follow the tree in the file
    write_file(d, sizeof d);
    TEST_ASSERT_EQUAL_INT(BOOT_OK, boot_load_dtb(&mem, &layout, PATH, RAM_BASE + 0x2000, 0x1000));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(d, mem.buffer + 0x2000, 64);
    TEST_ASSERT_EQUAL_UINT8(0xAA, mem.buffer[0x2000 + 64]);
    TEST_ASSERT_EQUAL_STRING("dtb", layout.regions[0].name);
    TEST_ASSERT_EQUAL_HEX32(RAM_BASE + 0x2040, layout.regions[0].end);
}

void test_d2_a_wrong_magic_is_refused(void) {
    uint8_t d[64];
    make_dtb(d, sizeof d, 0xd00dfeecu, 64);
    write_file(d, sizeof d);
    TEST_ASSERT_EQUAL_INT(BOOT_ERR_BAD_DTB, boot_load_dtb(&mem, &layout, PATH, RAM_BASE + 0x2000, 0x1000));
    assert_ram_untouched();
}

void test_d3_totalsize_larger_than_the_file_is_refused(void) {
    uint8_t d[64];
    make_dtb(d, sizeof d, 0xd00dfeedu, 65);
    write_file(d, sizeof d);
    TEST_ASSERT_EQUAL_INT(BOOT_ERR_BAD_DTB, boot_load_dtb(&mem, &layout, PATH, RAM_BASE + 0x2000, 0x1000));
    assert_ram_untouched();
}

void test_d4_totalsize_equal_to_the_file_is_fine(void) {
    uint8_t d[64];
    make_dtb(d, sizeof d, 0xd00dfeedu, 64);
    write_file(d, sizeof d);
    TEST_ASSERT_EQUAL_INT(BOOT_OK, boot_load_dtb(&mem, &layout, PATH, RAM_BASE + 0x2000, 0x1000));
}

void test_d5_larger_than_the_window_is_refused_and_equal_is_fine(void) {
    uint8_t d[256];
    make_dtb(d, sizeof d, 0xd00dfeedu, 256);
    write_file(d, sizeof d);
    TEST_ASSERT_EQUAL_INT(BOOT_ERR_BAD_DTB, boot_load_dtb(&mem, &layout, PATH, RAM_BASE + 0x2000, 255));
    assert_ram_untouched();
    TEST_ASSERT_EQUAL_INT(BOOT_OK, boot_load_dtb(&mem, &layout, PATH, RAM_BASE + 0x2000, 256));
}

void test_d6_shorter_than_the_header_is_refused(void) {
    uint8_t d[39];
    make_dtb(d, sizeof d, 0xd00dfeedu, 39);
    write_file(d, sizeof d);
    TEST_ASSERT_EQUAL_INT(BOOT_ERR_BAD_DTB, boot_load_dtb(&mem, &layout, PATH, RAM_BASE + 0x2000, 0x1000));
    uint8_t h[40];
    make_dtb(h, sizeof h, 0xd00dfeedu, 40);                                  // exactly a header
    write_file(h, sizeof h);
    TEST_ASSERT_EQUAL_INT(BOOT_OK, boot_load_dtb(&mem, &layout, PATH, RAM_BASE + 0x2000, 0x1000));
}

void test_d7_a_totalsize_below_the_header_is_refused(void) {
    uint8_t d[64];
    make_dtb(d, sizeof d, 0xd00dfeedu, 39);
    write_file(d, sizeof d);
    TEST_ASSERT_EQUAL_INT(BOOT_ERR_BAD_DTB, boot_load_dtb(&mem, &layout, PATH, RAM_BASE + 0x2000, 0x1000));
}

void test_d8_the_address_must_be_8_byte_aligned(void) {
    uint8_t d[64];
    make_dtb(d, sizeof d, 0xd00dfeedu, 64);
    write_file(d, sizeof d);
    TEST_ASSERT_EQUAL_INT(BOOT_ERR_BAD_DTB, boot_load_dtb(&mem, &layout, PATH, RAM_BASE + 0x2004, 0x1000));
    TEST_ASSERT_EQUAL_INT(BOOT_ERR_BAD_DTB, boot_load_dtb(&mem, &layout, PATH, RAM_BASE + 0x2001, 0x1000));
    TEST_ASSERT_EQUAL_INT(BOOT_OK, boot_load_dtb(&mem, &layout, PATH, RAM_BASE + 0x2008, 0x1000));
}

void test_d9_outside_ram_and_overlap(void) {
    uint8_t d[64];
    make_dtb(d, sizeof d, 0xd00dfeedu, 64);
    write_file(d, sizeof d);
    TEST_ASSERT_EQUAL_INT(BOOT_ERR_RANGE, boot_load_dtb(&mem, &layout, PATH, RAM_BASE + RAM_SIZE - 32, 0x1000));
    TEST_ASSERT_EQUAL_INT(BOOT_OK, boot_load_dtb(&mem, &layout, PATH, RAM_BASE + RAM_SIZE - 64, 0x1000));
    TEST_ASSERT_EQUAL_INT(BOOT_ERR_OVERLAP, boot_load_dtb(&mem, &layout, PATH, RAM_BASE + RAM_SIZE - 64, 0x1000));
}

void test_d10_an_empty_dtb_and_a_missing_one(void) {
    write_file(NULL, 0);
    TEST_ASSERT_EQUAL_INT(BOOT_ERR_EMPTY, boot_load_dtb(&mem, &layout, PATH, RAM_BASE + 0x2000, 0x1000));
    TEST_ASSERT_EQUAL_INT(BOOT_ERR_IO, boot_load_dtb(&mem, &layout, "no_such_boot_image.dtb", RAM_BASE + 0x2000, 0x1000));
}

void test_s1_every_status_has_its_own_message(void) {
    static const BootStatus all[] = { BOOT_OK, BOOT_ERR_IO, BOOT_ERR_EMPTY, BOOT_ERR_FORMAT, BOOT_ERR_LOOKS_LIKE_ELF, BOOT_ERR_RANGE,
                                      BOOT_ERR_OVERLAP, BOOT_ERR_BAD_DTB, BOOT_ERR_TOO_MANY };
    unsigned n = sizeof all / sizeof all[0];
    for (unsigned i = 0; i < n; i++) {
        TEST_ASSERT_TRUE(strlen(boot_status_str(all[i])) > 0);
        for (unsigned j = i + 1; j < n; j++) {
            TEST_ASSERT_NOT_EQUAL(0, strcmp(boot_status_str(all[i]), boot_status_str(all[j])));
        }
    }
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_f1_a_flat_file_lands_at_the_address);
    RUN_TEST(test_f2_a_file_ending_exactly_at_the_end_of_ram_fits);
    RUN_TEST(test_f3_one_byte_past_the_end_of_ram_does_not);
    RUN_TEST(test_f4_below_and_above_ram_are_out_of_range);
    RUN_TEST(test_f5_an_empty_file_has_nothing_to_load);
    RUN_TEST(test_f6_an_elf_given_as_flat_is_refused);
    RUN_TEST(test_f7_the_elf_magic_needs_all_four_bytes);
    RUN_TEST(test_f8_a_missing_file_is_an_io_error);
    RUN_TEST(test_f9_overlap_is_refused_and_adjacent_is_fine);
    RUN_TEST(test_f10_at_most_eight_images);
    RUN_TEST(test_e1_an_elf_lands_at_its_own_addresses);
    RUN_TEST(test_e2_bss_is_zeroed_and_counted);
    RUN_TEST(test_e3_a_flat_file_given_as_an_elf_is_a_format_error);
    RUN_TEST(test_e4_a_segment_outside_ram_is_out_of_range);
    RUN_TEST(test_e5_an_elf_overlapping_an_earlier_image_is_refused_and_writes_nothing);
    RUN_TEST(test_e6_the_gap_between_segments_counts_as_used);
    RUN_TEST(test_e7_an_elf_with_no_loadable_segment_has_nothing_to_load);
    RUN_TEST(test_e8_a_missing_elf_is_an_io_error);
    RUN_TEST(test_e9_the_entry_is_only_set_on_success);
    RUN_TEST(test_d1_a_good_dtb_copies_exactly_totalsize_bytes);
    RUN_TEST(test_d2_a_wrong_magic_is_refused);
    RUN_TEST(test_d3_totalsize_larger_than_the_file_is_refused);
    RUN_TEST(test_d4_totalsize_equal_to_the_file_is_fine);
    RUN_TEST(test_d5_larger_than_the_window_is_refused_and_equal_is_fine);
    RUN_TEST(test_d6_shorter_than_the_header_is_refused);
    RUN_TEST(test_d7_a_totalsize_below_the_header_is_refused);
    RUN_TEST(test_d8_the_address_must_be_8_byte_aligned);
    RUN_TEST(test_d9_outside_ram_and_overlap);
    RUN_TEST(test_d10_an_empty_dtb_and_a_missing_one);
    RUN_TEST(test_s1_every_status_has_its_own_message);
    return UNITY_END();
}
