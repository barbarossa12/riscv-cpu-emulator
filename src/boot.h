#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "mem.h"

// Loading boot images into RAM: the firmware, an optional next stage (a kernel) and the device tree. An image is named for what it is
// (ELF or flat binary) by the caller, never guessed: an ELF is placed at its own addresses, a flat binary at the address the machine
// gives it. Every load checks that the image is inside RAM and does not overlap one already placed, and a failed load leaves RAM
// untouched.
#define BOOT_MAX_REGIONS 8

typedef struct {
	const char *name;
	uint32_t    start;   // [start, end)
	uint32_t    end;
} BootRegion;

typedef struct {
	BootRegion regions[BOOT_MAX_REGIONS];
	int        count;
} BootLayout;

typedef enum {
	BOOT_OK,
	BOOT_ERR_IO,           // the file could not be read
	BOOT_ERR_EMPTY,        // a flat binary or device tree with no bytes
	BOOT_ERR_FORMAT,       // not a well-formed RV32 little-endian executable ELF
	BOOT_ERR_LOOKS_LIKE_ELF, // a file given as a flat binary starts with the ELF magic
	BOOT_ERR_RANGE,        // does not fit inside RAM
	BOOT_ERR_OVERLAP,      // overlaps an image placed before
	BOOT_ERR_BAD_DTB,      // not a flattened device tree, or larger than its window
	BOOT_ERR_TOO_MANY,     // more images than BOOT_MAX_REGIONS
} BootStatus;

void boot_layout_init(BootLayout *layout);

// An ELF at its own addresses. *entry is its entry point. The whole span from its lowest to its highest segment byte counts as used.
BootStatus boot_load_elf(Memory *mem, BootLayout *layout, const char *path, const char *name, uint32_t *entry);
// A flat binary at addr. A file that starts with the ELF magic is refused, because loading an ELF as raw bytes is always a mistake.
BootStatus boot_load_flat(Memory *mem, BootLayout *layout, const char *path, const char *name, uint32_t addr);
// A device tree at addr: the 0xd00dfeed magic, a totalsize that fits the file and max_size, and 8-byte alignment of addr.
// Exactly totalsize bytes are copied.
BootStatus boot_load_dtb(Memory *mem, BootLayout *layout, const char *path, uint32_t addr, uint32_t max_size);

const char *boot_status_str(BootStatus status);
