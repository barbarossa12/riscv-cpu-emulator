#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "mem.h"

typedef struct {
	uint32_t entry;      // e_entry
	uint32_t tohost;     // address of the `tohost` symbol
	bool     has_tohost; // false if the ELF has no such symbol
} ElfImage;

typedef enum {
	ELF_OK,
	ELF_ERR_IO,     // the file could not be read
	ELF_ERR_FORMAT, // not a well-formed RV32 little-endian executable ELF
	ELF_ERR_RANGE,  // a segment does not fit inside the memory region
} ElfStatus;

/* Load the PT_LOAD segments of an ELF32 executable into mem. Never aborts on a bad
 * file. Everything is validated before the first byte is written, so a failed load
 * leaves mem untouched. *out is only written on ELF_OK. */
ElfStatus elf_load_bytes(const uint8_t *data, size_t len, Memory *mem, ElfImage *out);
ElfStatus elf_load_file(const char *path, Memory *mem, ElfImage *out);

const char *elf_status_str(ElfStatus status);
