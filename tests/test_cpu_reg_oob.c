#include "../src/log.h"
#include "../src/cpu.h"
#include "../src/log.h"

int main() {
	CPU testCpu;

	//deliberately read past the register count limit.
	uint32_t reg_val = cpu_read_reg(&testCpu, 33);
	LOG_INFO("register value: %d", reg_val);
	return 0;
}
