#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include "cpu.h"
#include "mem.h"
#include "execute.h"
#include "elf.h"
#include "log.h"
#include "uart.h"
#include "plic.h"
#include "uart_host.h"
#include "finisher.h"
#include "boot.h"
#include "machine.h"

#define DEFAULT_MAX_INSTRS    1000000ull   // test mode only; a boot has no limit unless --max-instructions asks for one

#define UART_STDIN_POLL_INTERVAL 1024u   // cpu steps between looks at stdin (a fixed count, so piped input is deterministic)

#define EXIT_TEST_PASS        0
#define EXIT_TEST_FAIL        1
#define EXIT_TEST_TIMEOUT     2
#define EXIT_LOAD_ERROR       3
#define EXIT_REBOOT           4   // the guest asked for a reboot through the finisher

static bool uart_line(void *uart) {
	return uart_interrupt_pending(uart);
}

/* WFI looks at stdin first (B2): input that arrived is delivered to the UART and WFI does not jump time this once. */
typedef struct { HostInput *in; Uart *uart; } IdlePoll;
static bool idle_poll(void *ctx) {
	IdlePoll *p = ctx;
	return host_input_poll(p->in, p->uart) > 0;
}

static int usage(const char *prog) {
	fprintf(stderr, "usage: %s <program.elf> [max-instructions]\n"
	                "       %s (--bios-elf <file> | --bios-bin <file>) [--kernel-elf <file> | --kernel-bin <file>]\n"
	                "               [--dtb <file>] [--max-instructions <n>]\n", prog, prog);
	return EXIT_LOAD_ERROR;
}

/* RISCV_EMU_LOG=error|warn|info|debug sets the log level (default: info). At debug level
 * the emulator logs every privilege-mode change to stderr. */
static void set_log_level_from_env(const char *prog) {
	const char *v = getenv("RISCV_EMU_LOG");
	if (!v) {
		return;
	}
	if      (strcmp(v, "error") == 0) log_set_level(LOG_LEVEL_ERROR);
	else if (strcmp(v, "warn")  == 0) log_set_level(LOG_LEVEL_WARN);
	else if (strcmp(v, "info")  == 0) log_set_level(LOG_LEVEL_INFO);
	else if (strcmp(v, "debug") == 0) log_set_level(LOG_LEVEL_DEBUG);
	else fprintf(stderr, "%s: ignoring RISCV_EMU_LOG=%s (use error, warn, info or debug)\n", prog, v);
}

/* Everything the machine is made of. One setup for both modes: RAM, the CLINT, the PLIC, the UART with its host console, and
 * the finisher. */
typedef struct {
	Memory    mem;
	CPU       cpu;
	Uart      uart;
	Finisher  finisher;
	HostOutput uart_out;
	HostInput uart_in;
	IdlePoll  idle;
} Machine;

static void machine_init(Machine *m, const char *prog) {
	mem_init_at(&m->mem, MACHINE_RAM_BASE, MACHINE_RAM_SIZE);
	cpu_reset(&m->cpu);
	mem_add_device(&m->mem, clint_device(&m->cpu.clint));
	mem_add_device(&m->mem, plic_device(&m->cpu.plic));

	/* The UART: output to stdout (flushed per byte on a terminal), input from stdin only when
	 * RISCV_EMU_UART_STDIN is set, so ordinary runs stay deterministic. RISCV_EMU_UART_TIMEOUT sets
	 * the receive timeout in cpu steps; a bad value is ignored with a warning, like RISCV_EMU_LOG. */
	uart_reset(&m->uart);
	m->uart_out = (HostOutput){ stdout, isatty(STDOUT_FILENO) != 0 };
	uart_set_tx(&m->uart, uart_host_tx, &m->uart_out);
	const char *timeout_env = getenv("RISCV_EMU_UART_TIMEOUT");
	if (timeout_env) {
		uint32_t cycles;
		if (uart_host_parse_u32(timeout_env, &cycles)) uart_set_timeout(&m->uart, cycles);
		else fprintf(stderr, "%s: ignoring RISCV_EMU_UART_TIMEOUT=%s (use a number of cpu steps)\n", prog, timeout_env);
	}
	mem_add_device(&m->mem, uart_device(&m->uart));
	plic_set_source(&m->cpu.plic, MACHINE_UART_IRQ, uart_line, &m->uart);   // PLIC source 10, as the device tree says
	host_input_init(&m->uart_in, getenv("RISCV_EMU_UART_STDIN") ? STDIN_FILENO : -1);
	m->idle = (IdlePoll){ &m->uart_in, &m->uart };
	m->cpu.idle_poll = idle_poll;
	m->cpu.idle_poll_ctx = &m->idle;

	finisher_reset(&m->finisher);
	mem_add_device(&m->mem, finisher_device(&m->finisher));
}

/* The guest told the finisher to stop. Pass exits 0; a failure exits with the guest's code, and never 0 (a code of 0, or one that
 * does not fit in an exit status, becomes 1, so a failure cannot look like a pass). */
static int finisher_exit(const Machine *m) {
	fflush(stdout);
	switch (m->finisher.status) {
		case FINISHER_PASS:
			return EXIT_TEST_PASS;
		case FINISHER_FAIL: {
			unsigned code = m->finisher.code & 0xFFu;
			if (code == 0) code = EXIT_TEST_FAIL;
			fprintf(stderr, "riscv_emu: the guest reported failure (code %u)\n", code);
			return (int)code;
		}
		default:
			fprintf(stderr, "riscv_emu: the guest asked for a reboot\n");
			return EXIT_REBOOT;
	}
}

/* Run until the guest finishes (the finisher, and in test mode the riscv-tests `tohost` word) or max_instrs have run. */
static int run(Machine *m, unsigned long long max_instrs, bool test_mode, uint32_t tohost) {
	for (unsigned long long executed = 0; executed < max_instrs; executed++) {
		cpu_step(&m->cpu, &m->mem);
		/* after each full interval, not at step 0: the guest gets to set the UART up first, and enabling the
		 * FIFOs discards anything already received (datasheet, FCR bit 0) */
		if ((executed + 1) % UART_STDIN_POLL_INTERVAL == 0) host_input_poll(&m->uart_in, &m->uart);

		if (m->finisher.done) {
			return finisher_exit(m);
		}
		if (!test_mode) {
			continue;
		}
		uint32_t tohost_word = 0;
		(void)mem_try_read32(&m->mem, tohost, &tohost_word); // address checked by the caller
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
				       cpu_read_csr(&m->cpu, CSR_MCAUSE), cpu_read_csr(&m->cpu, CSR_MEPC),
				       cpu_read_csr(&m->cpu, CSR_MTVAL));
				code = EXIT_TEST_FAIL;
			}
			return code;
		}
	}

	printf("TIMEOUT after %llu instructions (pc = 0x%08x)\n", max_instrs, m->cpu.pc);
	return EXIT_TEST_TIMEOUT;
}

/* A decimal count of instructions, at least 1. */
static bool parse_count(const char *s, unsigned long long *out) {
	char *end;
	errno = 0;
	unsigned long long v = strtoull(s, &end, 10);
	if (errno != 0 || *end != '\0' || end == s || v == 0) return false;
	*out = v;
	return true;
}

typedef struct {
	const char *bios_elf, *bios_bin, *kernel_elf, *kernel_bin, *dtb;
	bool has_max;
	unsigned long long max_instrs;
	const char *program;   /* test mode: the positional ELF */
	bool boot;             /* any boot flag was given */
} Args;

static bool set_once(const char **slot, const char *value) {
	if (*slot) return false;
	*slot = value;
	return true;
}

/* False for anything that is not one of the two valid shapes (see usage). */
static bool parse_args(int argc, char **argv, Args *a) {
	*a = (Args){ 0 };
	const char *positional[2];
	int npos = 0;
	for (int i = 1; i < argc; i++) {
		const char *arg = argv[i];
		if (strncmp(arg, "--", 2) != 0) {
			if (npos == 2) return false;
			positional[npos++] = arg;
			continue;
		}
		if (i + 1 >= argc) return false;               /* every flag takes a value */
		const char *value = argv[++i];
		a->boot = true;
		bool ok;
		if      (strcmp(arg, "--bios-elf") == 0)         ok = set_once(&a->bios_elf, value);
		else if (strcmp(arg, "--bios-bin") == 0)         ok = set_once(&a->bios_bin, value);
		else if (strcmp(arg, "--kernel-elf") == 0)       ok = set_once(&a->kernel_elf, value);
		else if (strcmp(arg, "--kernel-bin") == 0)       ok = set_once(&a->kernel_bin, value);
		else if (strcmp(arg, "--dtb") == 0)              ok = set_once(&a->dtb, value);
		else if (strcmp(arg, "--max-instructions") == 0) ok = !a->has_max && parse_count(value, &a->max_instrs) && (a->has_max = true);
		else ok = false;
		if (!ok) return false;
	}
	if (a->boot) {
		/* boot mode: exactly one firmware image, at most one next stage, no positional program */
		return npos == 0 && (a->bios_elf != NULL) != (a->bios_bin != NULL) && !(a->kernel_elf && a->kernel_bin);
	}
	if (npos < 1) return false;
	a->program = positional[0];
	if (npos == 2) {
		if (!parse_count(positional[1], &a->max_instrs)) return false;
		a->has_max = true;
	}
	return true;
}

/* Boot mode: the firmware, an optional next stage and the device tree go into RAM, and the hart starts in M-mode at the
 * firmware with a0 = hart id (0) and a1 = the device tree address (0 without one), the RISC-V boot convention. */
static int load_boot_images(Machine *m, const Args *a, const char *prog, uint32_t *entry, uint32_t *dtb_addr) {
	BootLayout layout;
	boot_layout_init(&layout);
	const char *path;
	BootStatus st;

	if (a->bios_elf) { path = a->bios_elf; st = boot_load_elf(&m->mem, &layout, path, "firmware", entry); }
	else             { path = a->bios_bin; st = boot_load_flat(&m->mem, &layout, path, "firmware", MACHINE_RAM_BASE); *entry = MACHINE_RAM_BASE; }
	if (st == BOOT_OK && (a->kernel_elf || a->kernel_bin)) {
		uint32_t ignored;
		if (a->kernel_elf) { path = a->kernel_elf; st = boot_load_elf(&m->mem, &layout, path, "kernel", &ignored); }
		else               { path = a->kernel_bin; st = boot_load_flat(&m->mem, &layout, path, "kernel", MACHINE_KERNEL_ADDR); }
	}
	*dtb_addr = 0;
	if (st == BOOT_OK && a->dtb) {
		path = a->dtb;
		st = boot_load_dtb(&m->mem, &layout, path, MACHINE_DTB_ADDR, MACHINE_DTB_MAX);
		*dtb_addr = MACHINE_DTB_ADDR;
	}
	if (st != BOOT_OK) {
		fprintf(stderr, "%s: %s: %s\n", prog, path, boot_status_str(st));
		return EXIT_LOAD_ERROR;
	}
	return 0;
}

int main(int argc, char **argv) {
	set_log_level_from_env(argv[0]);

	Args args;
	if (!parse_args(argc, argv, &args)) {
		return usage(argv[0]);
	}

	Machine *m = malloc(sizeof *m);   /* the CPU and its TLBs are large; keep them off the stack */
	if (!m) {
		fprintf(stderr, "%s: out of memory\n", argv[0]);
		return EXIT_LOAD_ERROR;
	}
	machine_init(m, argv[0]);

	int code;
	if (args.boot) {
		uint32_t entry, dtb_addr;
		code = load_boot_images(m, &args, argv[0], &entry, &dtb_addr);
		if (code == 0) {
			m->cpu.pc = entry;
			cpu_write_reg(&m->cpu, 10, 0);          /* a0 = hart id */
			cpu_write_reg(&m->cpu, 11, dtb_addr);   /* a1 = device tree */
			code = run(m, args.has_max ? args.max_instrs : ULLONG_MAX, false, 0);
		}
	} else {
		ElfImage img;
		ElfStatus status = elf_load_file(args.program, &m->mem, &img);
		if (status != ELF_OK) {
			fprintf(stderr, "%s: %s: %s\n", argv[0], args.program, elf_status_str(status));
			code = EXIT_LOAD_ERROR;
		} else if (!img.has_tohost) {
			/* The riscv-tests report their result by writing the `tohost` word, so a program
			 * without one, or with it outside RAM, cannot be run to completion. */
			fprintf(stderr, "%s: %s: no `tohost` symbol, cannot detect completion\n", argv[0], args.program);
			code = EXIT_LOAD_ERROR;
		} else if (!mem_try_read32(&m->mem, img.tohost, &(uint32_t){0})) {
			fprintf(stderr, "%s: %s: `tohost` (0x%08x) is outside RAM\n", argv[0], args.program, img.tohost);
			code = EXIT_LOAD_ERROR;
		} else {
			m->cpu.pc = img.entry;
			code = run(m, args.has_max ? args.max_instrs : DEFAULT_MAX_INSTRS, true, img.tohost);
		}
	}

	mem_free(&m->mem);
	free(m);
	return code;
}
