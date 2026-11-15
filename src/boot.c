#include "boot.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "elf.h"

void boot_layout_init(BootLayout *layout) {
	layout->count = 0;
}

// The whole file in memory. *len is its size; *data is freed by the caller. A file of 0 bytes still returns a buffer.
static BootStatus read_file(const char *path, uint8_t **data, size_t *len) {
	FILE *f = fopen(path, "rb");
	if (!f) {
		return BOOT_ERR_IO;
	}
	if (fseek(f, 0, SEEK_END) != 0) {
		fclose(f);
		return BOOT_ERR_IO;
	}
	long size = ftell(f);
	if (size < 0 || fseek(f, 0, SEEK_SET) != 0) {
		fclose(f);
		return BOOT_ERR_IO;
	}
	uint8_t *buf = malloc((size_t)size + 1);
	if (!buf) {
		fclose(f);
		return BOOT_ERR_IO;
	}
	size_t got = fread(buf, 1, (size_t)size, f);
	fclose(f);
	if (got != (size_t)size) {
		free(buf);
		return BOOT_ERR_IO;
	}
	*data = buf;
	*len = got;
	return BOOT_OK;
}

static bool in_ram(const Memory *mem, uint32_t start, uint64_t n) {
	return start >= mem->base && (uint64_t)(start - mem->base) + n <= mem->size;
}

// Does [start, end) fit in RAM and miss every region already placed? Records it on success.
static BootStatus claim(const Memory *mem, BootLayout *layout, const char *name, uint32_t start, uint64_t end) {
	if (end <= start || !in_ram(mem, start, end - start)) {
		return BOOT_ERR_RANGE;
	}
	for (int i = 0; i < layout->count; i++) {
		if (start < layout->regions[i].end && layout->regions[i].start < end) {
			return BOOT_ERR_OVERLAP;
		}
	}
	if (layout->count >= BOOT_MAX_REGIONS) {
		return BOOT_ERR_TOO_MANY;
	}
	layout->regions[layout->count++] = (BootRegion){ name, start, (uint32_t)end };
	return BOOT_OK;
}

static void copy_in(Memory *mem, uint32_t addr, const uint8_t *src, size_t n) {
	memcpy(mem->buffer + (addr - mem->base), src, n);
}

static BootStatus from_elf(ElfStatus s) {
	switch (s) {
		case ELF_OK:         return BOOT_OK;
		case ELF_ERR_RANGE:  return BOOT_ERR_RANGE;
		case ELF_ERR_IO:     return BOOT_ERR_IO;
		default:             return BOOT_ERR_FORMAT;
	}
}

BootStatus boot_load_elf(Memory *mem, BootLayout *layout, const char *path, const char *name, uint32_t *entry) {
	uint8_t *data;
	size_t len;
	BootStatus st = read_file(path, &data, &len);
	if (st != BOOT_OK) {
		return st;
	}
	ElfImage img;
	st = from_elf(elf_inspect(data, len, mem, &img));
	if (st == BOOT_OK && img.hi > img.lo) {
		st = claim(mem, layout, name, img.lo, img.hi);
	} else if (st == BOOT_OK) {
		st = BOOT_ERR_EMPTY;   // no loadable segment
	}
	if (st == BOOT_OK) {
		st = from_elf(elf_load_bytes(data, len, mem, &img));
	}
	if (st == BOOT_OK) {
		*entry = img.entry;
	}
	free(data);
	return st;
}

BootStatus boot_load_flat(Memory *mem, BootLayout *layout, const char *path, const char *name, uint32_t addr) {
	uint8_t *data;
	size_t len;
	BootStatus st = read_file(path, &data, &len);
	if (st != BOOT_OK) {
		return st;
	}
	if (len == 0) {
		st = BOOT_ERR_EMPTY;
	} else if (len >= 4 && memcmp(data, "\x7f" "ELF", 4) == 0) {
		st = BOOT_ERR_LOOKS_LIKE_ELF;
	} else {
		st = claim(mem, layout, name, addr, (uint64_t)addr + len);
		if (st == BOOT_OK) {
			copy_in(mem, addr, data, len);
		}
	}
	free(data);
	return st;
}

static uint32_t be32(const uint8_t *p) {
	return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3];
}

BootStatus boot_load_dtb(Memory *mem, BootLayout *layout, const char *path, uint32_t addr, uint32_t max_size) {
	uint8_t *data;
	size_t len;
	BootStatus st = read_file(path, &data, &len);
	if (st != BOOT_OK) {
		return st;
	}
	if (len == 0) {
		st = BOOT_ERR_EMPTY;
	} else if (len < 40 || be32(data) != 0xd00dfeedu) {   // 40 bytes is the header
		st = BOOT_ERR_BAD_DTB;
	} else {
		uint32_t total = be32(data + 4);
		if (total < 40 || total > len || total > max_size || (addr & 7u) != 0) {
			st = BOOT_ERR_BAD_DTB;
		} else {
			st = claim(mem, layout, "dtb", addr, (uint64_t)addr + total);
			if (st == BOOT_OK) {
				copy_in(mem, addr, data, total);
			}
		}
	}
	free(data);
	return st;
}

const char *boot_status_str(BootStatus status) {
	switch (status) {
		case BOOT_OK:               return "ok";
		case BOOT_ERR_IO:           return "cannot read the file";
		case BOOT_ERR_EMPTY:        return "the file has nothing to load";
		case BOOT_ERR_FORMAT:       return "not an RV32 little-endian executable ELF";
		case BOOT_ERR_LOOKS_LIKE_ELF: return "this is an ELF file, give it as an ELF image, not a flat binary";
		case BOOT_ERR_RANGE:        return "does not fit inside RAM";
		case BOOT_ERR_OVERLAP:      return "overlaps another boot image";
		case BOOT_ERR_BAD_DTB:      return "not a valid device tree blob (magic, size or alignment)";
		case BOOT_ERR_TOO_MANY:     return "too many boot images";
	}
	return "unknown error";
}
