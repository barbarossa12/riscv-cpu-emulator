#include "../src/log.h"
#include "../src/cpu.h"

int main() {
	CPU test_cpu = {0};

	// deliberately read past the CSR address limit (0xFFF).
	uint32_t csr_val = cpu_read_csr(&test_cpu, 0x1000);
	LOG_INFO("csr value: %d", csr_val);
	return 0;
}
