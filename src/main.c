#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "cpu.h"
#include "mem.h"
#include "execute.h"
#include "elf.h"

#define RAM_BASE              0x80000000u
#define RAM_SIZE              (16u * 1024 * 1024)
#define DEFAULT_MAX_INSTRS    1000000ull

#define EXIT_TEST_PASS        0
#define EXIT_TEST_FAIL        1
#define EXIT_TEST_TIMEOUT     2
#define EXIT_LOAD_ERROR       3

static int usage(const char *prog) {
	fprintf(stderr, "usage: %s <program.elf> [max-instructions]\n", prog);
	return EXIT_LOAD_ERROR;
}

int main(int argc, char **argv) {
	if (argc < 2 || argc > 3) {
		return usage(argv[0]);
	}

	unsigned long long max_instrs = DEFAULT_MAX_INSTRS;
	if (argc == 3) {
		char *end;
		errno = 0;
		max_instrs = strtoull(argv[2], &end, 10);
		if (errno != 0 || *end != '\0' || end == argv[2] || max_instrs == 0) {
			return usage(argv[0]);
		}
	}

	Memory mem;
	mem_init_at(&mem, RAM_BASE, RAM_SIZE);

	ElfImage img;
	ElfStatus status = elf_load_file(argv[1], &mem, &img);
	if (status != ELF_OK) {
		fprintf(stderr, "%s: %s: %s\n", argv[0], argv[1], elf_status_str(status));
		mem_free(&mem);
		return EXIT_LOAD_ERROR;
	}

	/* The riscv-tests report their result by writing the `tohost` word, so a program
	 * without one, or with it outside RAM, cannot be run to completion. */
	uint32_t tohost_word;
	if (!img.has_tohost) {
		fprintf(stderr, "%s: %s: no `tohost` symbol, cannot detect completion\n", argv[0], argv[1]);
		mem_free(&mem);
		return EXIT_LOAD_ERROR;
	}
	if (!mem_try_read32(&mem, img.tohost, &tohost_word)) {
		fprintf(stderr, "%s: %s: `tohost` (0x%08x) is outside RAM\n", argv[0], argv[1], img.tohost);
		mem_free(&mem);
		return EXIT_LOAD_ERROR;
	}

	CPU cpu = {0};
	cpu.pc = img.entry;

	for (unsigned long long executed = 0; executed < max_instrs; executed++) {
		cpu_step(&cpu, &mem);

		(void)mem_try_read32(&mem, img.tohost, &tohost_word); // address checked above
		if (tohost_word != 0) {
			int code;
			if (tohost_word == 1) {
				printf("PASS (%llu instructions)\n", executed + 1);
				code = EXIT_TEST_PASS;
			} else {
				/* A reported failure has mcause 11 (the ecall). Any other cause means an
				 * unexpected exception reached the test's trap handler first. */
				printf("FAIL: test %u (tohost = 0x%x)\n", tohost_word >> 1, tohost_word);
				printf("      mcause = %u, mepc = 0x%08x, mtval = 0x%08x\n",
				       cpu_read_csr(&cpu, CSR_MCAUSE), cpu_read_csr(&cpu, CSR_MEPC),
				       cpu_read_csr(&cpu, CSR_MTVAL));
				code = EXIT_TEST_FAIL;
			}
			mem_free(&mem);
			return code;
		}
	}

	printf("TIMEOUT after %llu instructions (pc = 0x%08x)\n", max_instrs, cpu.pc);
	mem_free(&mem);
	return EXIT_TEST_TIMEOUT;
}
