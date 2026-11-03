#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "elf.h"

#define EHDR_SIZE 52
#define PHDR_SIZE 32
#define SHDR_SIZE 40
#define SYM_SIZE  16

#define EM_RISCV    243
#define ET_EXEC     2
#define PT_LOAD     1
#define SHT_SYMTAB  2
#define SHT_STRTAB  3

static uint16_t rd16(const uint8_t *p) {
	return (uint16_t)(p[0] | (p[1] << 8));
}

static uint32_t rd32(const uint8_t *p) {
	return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

/* True if [off, off + n) lies inside a buffer of len bytes. 64-bit so it can't wrap. */
static bool file_range_ok(size_t len, uint32_t off, uint32_t n) {
	return (uint64_t)off + n <= len;
}

/* True if [addr, addr + n) lies inside the memory region. 64-bit so it can't wrap. */
static bool mem_region_ok(const Memory *mem, uint32_t addr, uint32_t n) {
	return addr >= mem->base && (uint64_t)(addr - mem->base) + n <= mem->size;
}

/* True if strtab[off..] holds exactly the NUL-terminated string `name`. */
static bool name_matches(const uint8_t *strtab, uint32_t strsize, uint32_t off, const char *name) {
	size_t n = strlen(name) + 1; // include the NUL
	return file_range_ok(strsize, off, (uint32_t)n) && memcmp(strtab + off, name, n) == 0;
}

/* Find the `tohost` symbol. A missing symbol table or symbol is not an error. */
static ElfStatus find_tohost(const uint8_t *data, size_t len, ElfImage *img) {
	uint32_t shoff = rd32(data + 32);
	uint16_t shentsize = rd16(data + 46);
	uint16_t shnum = rd16(data + 48);

	img->has_tohost = false;
	if (shoff == 0 || shnum == 0) {
		return ELF_OK;
	}
	if (shentsize != SHDR_SIZE || !file_range_ok(len, shoff, (uint32_t)shnum * SHDR_SIZE)) {
		return ELF_ERR_FORMAT;
	}

	for (uint16_t i = 0; i < shnum; i++) {
		const uint8_t *sh = data + shoff + (size_t)i * SHDR_SIZE;
		if (rd32(sh + 4) != SHT_SYMTAB) {
			continue;
		}
		uint32_t sym_off = rd32(sh + 16);
		uint32_t sym_size = rd32(sh + 20);
		uint32_t link = rd32(sh + 24);
		if (!file_range_ok(len, sym_off, sym_size) || link >= shnum) {
			return ELF_ERR_FORMAT;
		}

		const uint8_t *str_sh = data + shoff + (size_t)link * SHDR_SIZE;
		uint32_t str_off = rd32(str_sh + 16);
		uint32_t str_size = rd32(str_sh + 20);
		if (rd32(str_sh + 4) != SHT_STRTAB || !file_range_ok(len, str_off, str_size)) {
			return ELF_ERR_FORMAT;
		}

		for (uint32_t s = 0; (uint64_t)(s + 1) * SYM_SIZE <= sym_size; s++) {
			const uint8_t *sym = data + sym_off + (size_t)s * SYM_SIZE;
			if (name_matches(data + str_off, str_size, rd32(sym), "tohost")) {
				img->tohost = rd32(sym + 4);
				img->has_tohost = true;
				return ELF_OK;
			}
		}
	}
	return ELF_OK;
}

ElfStatus elf_load_bytes(const uint8_t *data, size_t len, Memory *mem, ElfImage *out) {
	if (len < EHDR_SIZE) {
		return ELF_ERR_FORMAT;
	}
	if (memcmp(data, "\x7f" "ELF", 4) != 0
		|| data[4] != 1   // ELFCLASS32
		|| data[5] != 1   // ELFDATA2LSB
		|| rd16(data + 16) != ET_EXEC
		|| rd16(data + 18) != EM_RISCV) {
		return ELF_ERR_FORMAT;
	}

	ElfImage img = {0};
	img.entry = rd32(data + 24);
	uint32_t phoff = rd32(data + 28);
	uint16_t phentsize = rd16(data + 42);
	uint16_t phnum = rd16(data + 44);

	if (phnum > 0 && (phentsize != PHDR_SIZE || !file_range_ok(len, phoff, (uint32_t)phnum * PHDR_SIZE))) {
		return ELF_ERR_FORMAT;
	}

	/* Pass 1: validate every segment before writing anything. */
	for (uint16_t i = 0; i < phnum; i++) {
		const uint8_t *ph = data + phoff + (size_t)i * PHDR_SIZE;
		if (rd32(ph) != PT_LOAD) {
			continue; // e.g. RISCV_ATTRIBUTES: not something to load
		}
		uint32_t offset = rd32(ph + 4);
		uint32_t vaddr = rd32(ph + 8);
		uint32_t filesz = rd32(ph + 16);
		uint32_t memsz = rd32(ph + 20);
		if (filesz > memsz || !file_range_ok(len, offset, filesz)) {
			return ELF_ERR_FORMAT;
		}
		if (memsz > 0 && !mem_region_ok(mem, vaddr, memsz)) {
			return ELF_ERR_RANGE;
		}
	}

	ElfStatus status = find_tohost(data, len, &img);
	if (status != ELF_OK) {
		return status;
	}

	/* Pass 2: copy the file bytes and zero the rest (.bss). */
	for (uint16_t i = 0; i < phnum; i++) {
		const uint8_t *ph = data + phoff + (size_t)i * PHDR_SIZE;
		if (rd32(ph) != PT_LOAD) {
			continue;
		}
		uint32_t offset = rd32(ph + 4);
		uint32_t vaddr = rd32(ph + 8);
		uint32_t filesz = rd32(ph + 16);
		uint32_t memsz = rd32(ph + 20);
		for (uint32_t b = 0; b < memsz; b++) {
			uint8_t byte = (b < filesz) ? data[offset + b] : 0;
			if (!mem_try_write8(mem, vaddr + b, byte)) {
				return ELF_ERR_RANGE; // unreachable after pass 1, kept as a safety net
			}
		}
	}

	*out = img;
	return ELF_OK;
}

ElfStatus elf_load_file(const char *path, Memory *mem, ElfImage *out) {
	FILE *f = fopen(path, "rb");
	if (!f) {
		return ELF_ERR_IO;
	}
	if (fseek(f, 0, SEEK_END) != 0) {
		fclose(f);
		return ELF_ERR_IO;
	}
	long size = ftell(f);
	if (size < 0 || fseek(f, 0, SEEK_SET) != 0) {
		fclose(f);
		return ELF_ERR_IO;
	}

	uint8_t *data = (uint8_t *)malloc((size_t)size + 1); // +1 so an empty file still allocates
	if (!data) {
		fclose(f);
		return ELF_ERR_IO;
	}
	size_t got = fread(data, 1, (size_t)size, f);
	fclose(f);
	if (got != (size_t)size) {
		free(data);
		return ELF_ERR_IO;
	}

	ElfStatus status = elf_load_bytes(data, (size_t)size, mem, out);
	free(data);
	return status;
}

const char *elf_status_str(ElfStatus status) {
	switch (status) {
		case ELF_OK:         return "ok";
		case ELF_ERR_IO:     return "cannot read file";
		case ELF_ERR_FORMAT: return "not a valid RV32 little-endian executable ELF";
		case ELF_ERR_RANGE:  return "segment does not fit in memory";
	}
	return "unknown error";
}
