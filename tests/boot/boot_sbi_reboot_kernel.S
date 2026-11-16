# boot_sbi_reboot_kernel.S: the handoff stub, asking OpenSBI for a cold reboot instead of a shutdown. OpenSBI's syscon-reboot driver
# writes 0x7777 to the finisher, which the emulator reports as a reboot request (exit status 4).
#define RESET_TYPE SRST_COLD_REBOOT
#include "boot_sbi_handoff_kernel.S"
