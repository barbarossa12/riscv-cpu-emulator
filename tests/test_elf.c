#include <stdio.h>
#include <string.h>
#include "unity.h"
#include "elf.h"

void setUp(void) {} // Unity calles this before each test function
void tearDown(void) {} // Unity calls this after eacht test function

/* Layout of the in-memory ELF every test starts from:
 *   0x000  ELF header
 *   0x034  program headers (32 bytes each, filled in by the test)
 *   0x100  payload: byte i = 0x10 + i
 *   0x140  .strtab  "\0tohost\0other\0"
 *   0x160  .symtab  [null, other = 0x11111111, tohost = 0x80001000]
 *   0x1A0  section headers [null, .symtab, .strtab]
 */
#define PH(i)       (52 + 32 * (i))
#define PAYLOAD     0x100
#define STRTAB      0x140
#define SYMTAB      0x160
#define SHTAB       0x1A0
#define ELF_LEN     (SHTAB + 3 * 40)
#define ELF_BUF     1024

#define BASE        0x80000000u
#define MEM_SIZE    256

static void put16(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
}

static void put32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

static void make_elf(uint8_t *e, uint16_t phnum) {
    memset(e, 0, ELF_BUF);
    memcpy(e, "\x7f" "ELF", 4);
    e[4] = 1;                 // ELFCLASS32
    e[5] = 1;                 // ELFDATA2LSB
    e[6] = 1;                 // EV_CURRENT
    put16(e + 16, 2);         // ET_EXEC
    put16(e + 18, 243);       // EM_RISCV
    put32(e + 20, 1);
    put32(e + 24, BASE);      // entry
    put32(e + 28, 52);        // phoff
    put32(e + 32, SHTAB);     // shoff
    put16(e + 40, 52);        // ehsize
    put16(e + 42, 32);        // phentsize
    put16(e + 44, phnum);
    put16(e + 46, 40);        // shentsize
    put16(e + 48, 3);         // shnum

    for (int i = 0; i < 64; i++) {
        e[PAYLOAD + i] = (uint8_t)(0x10 + i);
    }

    memcpy(e + STRTAB + 1, "tohost", 7);
    memcpy(e + STRTAB + 8, "other", 6);

    put32(e + SYMTAB + 16 + 0, 8);           // "other"
    put32(e + SYMTAB + 16 + 4, 0x11111111);
    put32(e + SYMTAB + 32 + 0, 1);           // "tohost"
    put32(e + SYMTAB + 32 + 4, 0x80001000);

    put32(e + SHTAB + 40 + 4, 2);            // SHT_SYMTAB
    put32(e + SHTAB + 40 + 16, SYMTAB);
    put32(e + SHTAB + 40 + 20, 48);
    put32(e + SHTAB + 40 + 24, 2);           // link -> .strtab
    put32(e + SHTAB + 40 + 36, 16);
    put32(e + SHTAB + 80 + 4, 3);            // SHT_STRTAB
    put32(e + SHTAB + 80 + 16, STRTAB);
    put32(e + SHTAB + 80 + 20, 14);
}

static void set_phdr(uint8_t *e, int i, uint32_t type, uint32_t offset, uint32_t vaddr,
                     uint32_t filesz, uint32_t memsz) {
    put32(e + PH(i) + 0, type);
    put32(e + PH(i) + 4, offset);
    put32(e + PH(i) + 8, vaddr);
    put32(e + PH(i) + 12, vaddr);
    put32(e + PH(i) + 16, filesz);
    put32(e + PH(i) + 20, memsz);
    put32(e + PH(i) + 24, 5);
}

static void fresh_mem(Memory *m, uint32_t base) {
    mem_init_at(m, base, MEM_SIZE);
    memset(m->buffer, 0xAA, MEM_SIZE);
}

static void assert_mem_untouched(const Memory *m) {
    for (size_t i = 0; i < m->size; i++) {
        TEST_ASSERT_EQUAL_UINT8_MESSAGE(0xAA, m->buffer[i], "memory was written");
    }
}

/* A failed load must not write memory and must not touch *out. */
static void assert_load_fails(const uint8_t *e, size_t len, Memory *m, ElfStatus expected) {
    ElfImage img = { 0xDEADBEEF, 0x12345678, true };
    TEST_ASSERT_EQUAL_INT(expected, elf_load_bytes(e, len, m, &img));
    TEST_ASSERT_EQUAL_UINT32(0xDEADBEEF, img.entry);
    TEST_ASSERT_EQUAL_UINT32(0x12345678, img.tohost);
    TEST_ASSERT_TRUE(img.has_tohost);
    assert_mem_untouched(m);
}

void test_elf_loads_single_segment(void) {
    uint8_t e[ELF_BUF]; Memory m; ElfImage img;
    make_elf(e, 1);
    set_phdr(e, 0, 1, PAYLOAD, BASE, 16, 16);
    fresh_mem(&m, BASE);

    TEST_ASSERT_EQUAL_INT(ELF_OK, elf_load_bytes(e, ELF_LEN, &m, &img));

    TEST_ASSERT_EQUAL_UINT32(BASE, img.entry);
    for (int i = 0; i < 16; i++) {
        TEST_ASSERT_EQUAL_UINT8(0x10 + i, m.buffer[i]);
    }
    TEST_ASSERT_EQUAL_UINT8(0xAA, m.buffer[16]); // nothing past the segment
    mem_free(&m);
}

void test_elf_zero_fills_bss(void) {
    uint8_t e[ELF_BUF]; Memory m; ElfImage img;
    make_elf(e, 1);
    set_phdr(e, 0, 1, PAYLOAD, BASE, 8, 24); // 8 bytes from the file, 16 of .bss
    fresh_mem(&m, BASE);

    TEST_ASSERT_EQUAL_INT(ELF_OK, elf_load_bytes(e, ELF_LEN, &m, &img));

    for (int i = 0; i < 8; i++) {
        TEST_ASSERT_EQUAL_UINT8(0x10 + i, m.buffer[i]);
    }
    for (int i = 8; i < 24; i++) {
        TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, m.buffer[i], "bss not zeroed");
    }
    TEST_ASSERT_EQUAL_UINT8(0xAA, m.buffer[24]);
    mem_free(&m);
}

/* RISCV_ATTRIBUTES-style header: not PT_LOAD, vaddr 0, memsz 0 */
void test_elf_skips_non_load_segments(void) {
    uint8_t e[ELF_BUF]; Memory m; ElfImage img;
    make_elf(e, 2);
    set_phdr(e, 0, 0x70000003, PAYLOAD, 0, 16, 0);
    set_phdr(e, 1, 1, PAYLOAD, BASE, 16, 16);
    fresh_mem(&m, BASE);

    TEST_ASSERT_EQUAL_INT(ELF_OK, elf_load_bytes(e, ELF_LEN, &m, &img));

    TEST_ASSERT_EQUAL_UINT8(0x10, m.buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0xAA, m.buffer[16]);
    mem_free(&m);
}

void test_elf_loads_two_segments(void) {
    uint8_t e[ELF_BUF]; Memory m; ElfImage img;
    make_elf(e, 2);
    set_phdr(e, 0, 1, PAYLOAD, BASE, 8, 8);
    set_phdr(e, 1, 1, PAYLOAD + 8, BASE + 0x80, 8, 8);
    fresh_mem(&m, BASE);

    TEST_ASSERT_EQUAL_INT(ELF_OK, elf_load_bytes(e, ELF_LEN, &m, &img));

    TEST_ASSERT_EQUAL_UINT8(0x10, m.buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0x18, m.buffer[0x80]);
    TEST_ASSERT_EQUAL_UINT8(0xAA, m.buffer[8]);
    TEST_ASSERT_EQUAL_UINT8(0xAA, m.buffer[0x88]);
    mem_free(&m);
}

void test_elf_finds_tohost_symbol(void) {
    uint8_t e[ELF_BUF]; Memory m; ElfImage img;
    make_elf(e, 0);
    fresh_mem(&m, BASE);

    TEST_ASSERT_EQUAL_INT(ELF_OK, elf_load_bytes(e, ELF_LEN, &m, &img));

    TEST_ASSERT_TRUE(img.has_tohost);
    TEST_ASSERT_EQUAL_UINT32(0x80001000, img.tohost);
    mem_free(&m);
}

void test_elf_without_symbol_table_has_no_tohost(void) {
    uint8_t e[ELF_BUF]; Memory m; ElfImage img;
    make_elf(e, 0);
    put32(e + 32, 0);  // e_shoff = 0
    put16(e + 48, 0);  // e_shnum = 0
    fresh_mem(&m, BASE);

    TEST_ASSERT_EQUAL_INT(ELF_OK, elf_load_bytes(e, ELF_LEN, &m, &img));

    TEST_ASSERT_FALSE(img.has_tohost);
    mem_free(&m);
}

void test_elf_symbol_table_without_tohost(void) {
    uint8_t e[ELF_BUF]; Memory m; ElfImage img;
    make_elf(e, 0);
    e[STRTAB + 1] = 'x'; // "xohost": near miss must not match
    fresh_mem(&m, BASE);

    TEST_ASSERT_EQUAL_INT(ELF_OK, elf_load_bytes(e, ELF_LEN, &m, &img));

    TEST_ASSERT_FALSE(img.has_tohost);
    mem_free(&m);
}

/* A longer name that merely starts with "tohost" must not match either. */
void test_elf_tohost_prefix_does_not_match(void) {
    uint8_t e[ELF_BUF]; Memory m; ElfImage img;
    make_elf(e, 0);
    e[STRTAB + 7] = 'X'; // "tohostXother": the NUL after "tohost" is gone
    fresh_mem(&m, BASE);

    TEST_ASSERT_EQUAL_INT(ELF_OK, elf_load_bytes(e, ELF_LEN, &m, &img));

    TEST_ASSERT_FALSE(img.has_tohost);
    mem_free(&m);
}

void test_elf_zero_program_headers_loads_nothing(void) {
    uint8_t e[ELF_BUF]; Memory m; ElfImage img;
    make_elf(e, 0);
    put16(e + 42, 0); // phentsize is irrelevant when there are no headers
    fresh_mem(&m, BASE);

    TEST_ASSERT_EQUAL_INT(ELF_OK, elf_load_bytes(e, ELF_LEN, &m, &img));

    TEST_ASSERT_EQUAL_UINT32(BASE, img.entry);
    assert_mem_untouched(&m);
    mem_free(&m);
}

/* Header fields that must reject the file: {offset, width in bytes, bad value} */
void test_elf_rejects_bad_headers(void) {
    static const struct { size_t off; int width; uint32_t val; const char *what; } cases[] = {
        { 0,  1, 0x7E,  "bad magic" },
        { 4,  1, 2,     "ELFCLASS64" },
        { 5,  1, 2,     "big-endian" },
        { 16, 2, 1,     "ET_REL" },
        { 18, 2, 0x3E,  "x86-64 machine" },
        { 42, 2, 20,    "wrong phentsize" },
    };
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        uint8_t e[ELF_BUF]; Memory m;
        make_elf(e, 1);
        set_phdr(e, 0, 1, PAYLOAD, BASE, 16, 16);
        if (cases[i].width == 1) { e[cases[i].off] = (uint8_t)cases[i].val; }
        else { put16(e + cases[i].off, (uint16_t)cases[i].val); }
        fresh_mem(&m, BASE);

        TEST_MESSAGE(cases[i].what);
        assert_load_fails(e, ELF_LEN, &m, ELF_ERR_FORMAT);
        mem_free(&m);
    }
}

void test_elf_rejects_input_shorter_than_header(void) {
    uint8_t e[ELF_BUF]; Memory m;
    make_elf(e, 0);
    fresh_mem(&m, BASE);
    assert_load_fails(e, 51, &m, ELF_ERR_FORMAT);
    assert_load_fails(e, 0, &m, ELF_ERR_FORMAT);
    mem_free(&m);
}

void test_elf_rejects_program_header_table_past_end(void) {
    uint8_t e[ELF_BUF]; Memory m;
    make_elf(e, 1);
    set_phdr(e, 0, 1, PAYLOAD, BASE, 16, 16);
    fresh_mem(&m, BASE);

    assert_load_fails(e, PH(1) - 1, &m, ELF_ERR_FORMAT); // table cut one byte short

    put32(e + 28, 0xFFFFFFF0); // phoff near 2^32: offset + size must not wrap
    assert_load_fails(e, ELF_LEN, &m, ELF_ERR_FORMAT);
    mem_free(&m);
}

void test_elf_rejects_segment_file_range_past_end(void) {
    uint8_t e[ELF_BUF]; Memory m;
    make_elf(e, 1);
    fresh_mem(&m, BASE);

    set_phdr(e, 0, 1, ELF_LEN - 8, BASE, 16, 16); // 8 bytes of file, 16 claimed
    assert_load_fails(e, ELF_LEN, &m, ELF_ERR_FORMAT);

    set_phdr(e, 0, 1, 0xFFFFFFF8, BASE, 16, 16); // offset + filesz wraps past 2^32
    assert_load_fails(e, ELF_LEN, &m, ELF_ERR_FORMAT);
    mem_free(&m);
}

void test_elf_rejects_filesz_larger_than_memsz(void) {
    uint8_t e[ELF_BUF]; Memory m;
    make_elf(e, 1);
    set_phdr(e, 0, 1, PAYLOAD, BASE, 16, 8);
    fresh_mem(&m, BASE);
    assert_load_fails(e, ELF_LEN, &m, ELF_ERR_FORMAT);
    mem_free(&m);
}

void test_elf_rejects_segment_outside_memory(void) {
    uint8_t e[ELF_BUF]; Memory m;
    make_elf(e, 1);
    fresh_mem(&m, BASE);

    set_phdr(e, 0, 1, PAYLOAD, BASE - 4, 16, 16);          // starts below the base
    assert_load_fails(e, ELF_LEN, &m, ELF_ERR_RANGE);
    set_phdr(e, 0, 1, PAYLOAD, 0, 16, 16);                 // nowhere near the region
    assert_load_fails(e, ELF_LEN, &m, ELF_ERR_RANGE);
    set_phdr(e, 0, 1, PAYLOAD, BASE + MEM_SIZE, 16, 16);   // starts at the end
    assert_load_fails(e, ELF_LEN, &m, ELF_ERR_RANGE);
    set_phdr(e, 0, 1, PAYLOAD, BASE + MEM_SIZE - 8, 16, 16); // straddles the end
    assert_load_fails(e, ELF_LEN, &m, ELF_ERR_RANGE);
    set_phdr(e, 0, 1, PAYLOAD, BASE, 16, MEM_SIZE + 1);    // bss runs past the end
    assert_load_fails(e, ELF_LEN, &m, ELF_ERR_RANGE);
    mem_free(&m);
}

/* vaddr + memsz wraps past 2^32 into low addresses: must be rejected, not wrapped */
void test_elf_rejects_segment_wrapping_past_address_space(void) {
    uint8_t e[ELF_BUF]; Memory m;
    make_elf(e, 1);
    set_phdr(e, 0, 1, PAYLOAD, 0xFFFFFFF0u, 16, 0x20);
    fresh_mem(&m, 0xFFFFFF00u);
    assert_load_fails(e, ELF_LEN, &m, ELF_ERR_RANGE);
    mem_free(&m);
}

/* (vaddr - base) + memsz overflows 32 bits back to a small number: a 32-bit range check
 * would accept this, the 64-bit one must not. */
void test_elf_rejects_memsz_that_overflows_32_bit_sum(void) {
    uint8_t e[ELF_BUF]; Memory m;
    make_elf(e, 1);
    set_phdr(e, 0, 1, PAYLOAD, BASE + 0xF0, 16, 0xFFFFFF20u); // 0xF0 + 0xFFFFFF20 = 0x1_0000_0010
    fresh_mem(&m, BASE);
    assert_load_fails(e, ELF_LEN, &m, ELF_ERR_RANGE);
    mem_free(&m);
}

/* The second segment is bad: the valid first segment must not have been written. */
void test_elf_failed_load_writes_nothing(void) {
    uint8_t e[ELF_BUF]; Memory m;
    make_elf(e, 2);
    set_phdr(e, 0, 1, PAYLOAD, BASE, 16, 16);
    set_phdr(e, 1, 1, PAYLOAD, BASE + MEM_SIZE, 16, 16);
    fresh_mem(&m, BASE);
    assert_load_fails(e, ELF_LEN, &m, ELF_ERR_RANGE);
    mem_free(&m);
}

void test_elf_rejects_bad_section_header_table(void) {
    uint8_t e[ELF_BUF]; Memory m;
    make_elf(e, 1);
    set_phdr(e, 0, 1, PAYLOAD, BASE, 16, 16);
    fresh_mem(&m, BASE);

    put32(e + 32, 0xFFFFFF00); // shoff far past the end
    assert_load_fails(e, ELF_LEN, &m, ELF_ERR_FORMAT);

    make_elf(e, 1);
    set_phdr(e, 0, 1, PAYLOAD, BASE, 16, 16);
    put32(e + SHTAB + 40 + 24, 9); // .symtab links to a section that does not exist
    assert_load_fails(e, ELF_LEN, &m, ELF_ERR_FORMAT);

    make_elf(e, 1);
    set_phdr(e, 0, 1, PAYLOAD, BASE, 16, 16);
    put32(e + SHTAB + 40 + 16, 0xFFFFFFF0); // .symtab offset wraps
    assert_load_fails(e, ELF_LEN, &m, ELF_ERR_FORMAT);
    mem_free(&m);
}

void test_elf_load_file_roundtrip(void) {
    uint8_t e[ELF_BUF]; Memory m; ElfImage img;
    const char *path = "test_elf_tmp.elf";
    make_elf(e, 1);
    set_phdr(e, 0, 1, PAYLOAD, BASE, 16, 16);
    fresh_mem(&m, BASE);

    FILE *f = fopen(path, "wb");
    TEST_ASSERT_NOT_NULL(f);
    TEST_ASSERT_EQUAL_size_t(ELF_LEN, fwrite(e, 1, ELF_LEN, f));
    fclose(f);

    ElfStatus st = elf_load_file(path, &m, &img);
    remove(path);

    TEST_ASSERT_EQUAL_INT(ELF_OK, st);
    TEST_ASSERT_EQUAL_UINT32(0x80001000, img.tohost);
    TEST_ASSERT_EQUAL_UINT8(0x10, m.buffer[0]);
    mem_free(&m);
}

void test_elf_load_file_missing_is_io_error(void) {
    Memory m; ElfImage img;
    fresh_mem(&m, BASE);
    TEST_ASSERT_EQUAL_INT(ELF_ERR_IO, elf_load_file("no/such/file.elf", &m, &img));
    mem_free(&m);
}

void test_elf_load_file_empty_is_format_error(void) {
    Memory m; ElfImage img;
    const char *path = "test_elf_empty.elf";
    fresh_mem(&m, BASE);

    FILE *f = fopen(path, "wb");
    TEST_ASSERT_NOT_NULL(f);
    fclose(f);

    ElfStatus st = elf_load_file(path, &m, &img);
    remove(path);

    TEST_ASSERT_EQUAL_INT(ELF_ERR_FORMAT, st);
    mem_free(&m);
}

void test_elf_status_strings_are_distinct(void) {
    const ElfStatus all[] = { ELF_OK, ELF_ERR_IO, ELF_ERR_FORMAT, ELF_ERR_RANGE };
    for (size_t i = 0; i < 4; i++) {
        TEST_ASSERT_NOT_NULL(elf_status_str(all[i]));
        for (size_t j = i + 1; j < 4; j++) {
            TEST_ASSERT_TRUE(strcmp(elf_status_str(all[i]), elf_status_str(all[j])) != 0);
        }
    }
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_elf_loads_single_segment);
    RUN_TEST(test_elf_zero_fills_bss);
    RUN_TEST(test_elf_skips_non_load_segments);
    RUN_TEST(test_elf_loads_two_segments);
    RUN_TEST(test_elf_finds_tohost_symbol);
    RUN_TEST(test_elf_without_symbol_table_has_no_tohost);
    RUN_TEST(test_elf_symbol_table_without_tohost);
    RUN_TEST(test_elf_tohost_prefix_does_not_match);
    RUN_TEST(test_elf_zero_program_headers_loads_nothing);
    RUN_TEST(test_elf_rejects_bad_headers);
    RUN_TEST(test_elf_rejects_input_shorter_than_header);
    RUN_TEST(test_elf_rejects_program_header_table_past_end);
    RUN_TEST(test_elf_rejects_segment_file_range_past_end);
    RUN_TEST(test_elf_rejects_filesz_larger_than_memsz);
    RUN_TEST(test_elf_rejects_segment_outside_memory);
    RUN_TEST(test_elf_rejects_segment_wrapping_past_address_space);
    RUN_TEST(test_elf_rejects_memsz_that_overflows_32_bit_sum);
    RUN_TEST(test_elf_failed_load_writes_nothing);
    RUN_TEST(test_elf_rejects_bad_section_header_table);
    RUN_TEST(test_elf_load_file_roundtrip);
    RUN_TEST(test_elf_load_file_missing_is_io_error);
    RUN_TEST(test_elf_load_file_empty_is_format_error);
    RUN_TEST(test_elf_status_strings_are_distinct);
    return UNITY_END();
}
