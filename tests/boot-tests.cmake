# The boot loader, end to end through the real emulator: small firmware and next-stage stubs (tests/boot/*.S) are built into ELF and
# flat images, loaded with the explicit image flags, and must see the RISC-V boot contract (a0 = hart id, a1 = the device tree, MMU
# off, machine mode) before they finish through the test finisher. The emulator's exit status and messages are matched. These need
# a RISC-V gcc, like the riscv-tests; the ones that load a device tree also need dtc.

if(RISCV_GCC)
    set(BOOT_STUBS_DIR "${CMAKE_BINARY_DIR}/boot-tests")
    file(GLOB _boot_sources CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/tests/boot/boot_*.S")
    add_test(NAME boot.build
             COMMAND "${CMAKE_SOURCE_DIR}/scripts/build-boot-tests.sh" "${BOOT_STUBS_DIR}" ${_boot_sources})
    set_tests_properties(boot.build PROPERTIES FIXTURES_SETUP boot-stubs TIMEOUT 120)

    # boot_test(name regex args...): run the emulator with the args, stderr merged into stdout, then print its exit status as [exit=N].
    function(boot_test name regex)
        string(JOIN " " args ${ARGN})
        add_test(NAME boot.${name}
                 COMMAND sh -c "$<TARGET_FILE:riscv_emu> ${args} 2>&1; echo \"[exit=$?]\"")
        set_tests_properties(boot.${name} PROPERTIES
            FIXTURES_REQUIRED boot-stubs TIMEOUT 60 PASS_REGULAR_EXPRESSION "${regex}")
    endfunction()

    set(S "${BOOT_STUBS_DIR}")
    set(LIMIT --max-instructions 100000)

    # what the firmware is promised, from an ELF and from a flat binary, then the same through a next stage
    if(TARGET riscv_emu_dtb)
        set(DTB --dtb "${RISCV_EMU_DTB}")
        boot_test(elf-firmware        "\\[exit=0\\]" --bios-elf ${S}/boot_ok.elf ${DTB} ${LIMIT})
        boot_test(flat-firmware       "\\[exit=0\\]" --bios-bin ${S}/boot_ok.bin ${DTB} ${LIMIT})
        boot_test(flat-next-stage     "\\[exit=0\\]" --bios-elf ${S}/boot_jump.elf --kernel-bin ${S}/boot_kernel.bin ${DTB} ${LIMIT})
        boot_test(elf-next-stage      "\\[exit=0\\]" --bios-bin ${S}/boot_jump.bin --kernel-elf ${S}/boot_kernel.elf ${DTB} ${LIMIT})
        boot_test(no-limit-by-default "\\[exit=0\\]" --bios-elf ${S}/boot_ok.elf ${DTB})   # a boot has no instruction limit unless asked for
        boot_test(bad-dtb             "not a valid device tree blob.*\\[exit=3\\]"
                  --bios-elf ${S}/boot_ok.elf --dtb ${S}/boot_ok.bin ${LIMIT})
        boot_test(missing-dtb         "no_such.dtb: cannot read the file.*\\[exit=3\\]"
                  --bios-elf ${S}/boot_ok.elf --dtb ${S}/no_such.dtb ${LIMIT})
    endif()
    boot_test(no-dtb              "\\[exit=0\\]" --bios-elf ${S}/boot_nodtb.elf ${LIMIT})

    # the finisher's results become the exit status
    boot_test(finisher-fail       "the guest reported failure \\(code 42\\).*\\[exit=42\\]" --bios-elf ${S}/boot_fail.elf ${LIMIT})
    boot_test(finisher-fail-zero  "the guest reported failure \\(code 1\\).*\\[exit=1\\]"   --bios-elf ${S}/boot_fail0.elf ${LIMIT})
    boot_test(finisher-reboot     "asked for a reboot.*\\[exit=4\\]"                         --bios-elf ${S}/boot_reboot.elf ${LIMIT})
    boot_test(instruction-limit   "TIMEOUT after 1000 instructions.*\\[exit=2\\]"            --bios-elf ${S}/boot_spin.elf --max-instructions 1000)

    # images that cannot be loaded
    boot_test(missing-firmware    "no_such.elf: cannot read the file.*\\[exit=3\\]"          --bios-elf ${S}/no_such.elf)
    boot_test(elf-as-flat         "this is an ELF file.*\\[exit=3\\]"                        --bios-bin ${S}/boot_ok.elf)
    boot_test(flat-as-elf         "not an RV32 little-endian executable ELF.*\\[exit=3\\]"   --bios-elf ${S}/boot_ok.bin)
    boot_test(images-overlap      "overlaps another boot image.*\\[exit=3\\]"                --bios-elf ${S}/boot_ok.elf --kernel-elf ${S}/boot_ok.elf)
    boot_test(kernel-over-firmware "overlaps another boot image.*\\[exit=3\\]"               --bios-bin ${S}/boot_ok.bin --kernel-elf ${S}/boot_ok.elf)

    # the command line: every wrong shape prints the usage and exits 3
    boot_test(usage-no-arguments  "usage:.*\\[exit=3\\]")
    boot_test(usage-no-firmware   "usage:.*\\[exit=3\\]" --dtb ${S}/boot_ok.bin)
    boot_test(usage-two-firmwares "usage:.*\\[exit=3\\]" --bios-elf ${S}/boot_ok.elf --bios-bin ${S}/boot_ok.bin)
    boot_test(usage-two-kernels   "usage:.*\\[exit=3\\]" --bios-elf ${S}/boot_ok.elf --kernel-elf ${S}/boot_kernel.elf --kernel-bin ${S}/boot_kernel.bin)
    boot_test(usage-missing-value "usage:.*\\[exit=3\\]" --bios-elf)
    boot_test(usage-repeated-flag "usage:.*\\[exit=3\\]" --bios-elf ${S}/boot_ok.elf --bios-elf ${S}/boot_ok.elf)
    boot_test(usage-unknown-flag  "usage:.*\\[exit=3\\]" --bios-elf ${S}/boot_ok.elf --frobnicate 1)
    boot_test(usage-program-and-flag "usage:.*\\[exit=3\\]" ${S}/boot_ok.elf --bios-elf ${S}/boot_ok.elf)
    boot_test(usage-limit-zero    "usage:.*\\[exit=3\\]" --bios-elf ${S}/boot_ok.elf --max-instructions 0)
    boot_test(usage-limit-text    "usage:.*\\[exit=3\\]" --bios-elf ${S}/boot_ok.elf --max-instructions abc)

    # OpenSBI itself, as the firmware. It is built out of tree from the opensbi/ checkout with a Linux-flavoured RISC-V gcc (the
    # bare-metal one cannot link position-independent executables, which OpenSBI's Makefile requires); only fw_jump is needed. The
    # flat image is loaded at the RAM base, OpenSBI relocates itself, prints its banner on our UART and hands over to the next stage
    # at 0x80400000 in S-mode. The next stage is a stub that checks what it was promised and then asks OpenSBI to shut down or reboot.
    find_program(RISCV_LINUX_GCC riscv64-linux-gnu-gcc)
    find_program(MAKE_PROGRAM make)
    if(RISCV_LINUX_GCC AND MAKE_PROGRAM AND EXISTS "${CMAKE_SOURCE_DIR}/opensbi/Makefile" AND TARGET riscv_emu_dtb)
        set(OPENSBI_BUILD_DIR "${CMAKE_BINARY_DIR}/opensbi")
        set(OPENSBI_FW "${OPENSBI_BUILD_DIR}/platform/generic/firmware/fw_jump.bin")
        add_test(NAME opensbi.build
                 COMMAND ${MAKE_PROGRAM} -C "${CMAKE_SOURCE_DIR}/opensbi" O=${OPENSBI_BUILD_DIR} -j4
                         PLATFORM=generic PLATFORM_RISCV_XLEN=32 PLATFORM_RISCV_ISA=rv32imac_zicsr_zifencei
                         PLATFORM_RISCV_ABI=ilp32 CROSS_COMPILE=riscv64-linux-gnu- FW_JUMP=y FW_DYNAMIC=n FW_PAYLOAD=n)
        set_tests_properties(opensbi.build PROPERTIES FIXTURES_SETUP opensbi-fw TIMEOUT 600)

        function(opensbi_test name regex kernel)
            add_test(NAME opensbi.${name}
                     COMMAND sh -c "$<TARGET_FILE:riscv_emu> --bios-bin ${OPENSBI_FW} --kernel-elf ${S}/${kernel} --dtb ${RISCV_EMU_DTB} --max-instructions 50000000 2>&1; echo \"[exit=$?]\"")
            set_tests_properties(opensbi.${name} PROPERTIES
                FIXTURES_REQUIRED "boot-stubs;opensbi-fw" TIMEOUT 120 PASS_REGULAR_EXPRESSION "${regex}")
        endfunction()

        # the stub checks the handoff (its own exit code for each item) and OpenSBI's shutdown ends the run with status 0
        opensbi_test(handoff "\\[exit=0\\]" boot_sbi_handoff_kernel.elf)
        # the banner describes our machine: the parts that come from our device tree and our CPU, in OpenSBI's order
        opensbi_test(banner
            "Platform Name *: riscv-emu.*Platform HART Count *: 1.*Platform HART Protection *: pmp.*Platform IPI Device *: aclint-mswi.*Platform Timer Device *: aclint-mtimer @ 10000000Hz.*Platform Console Device *: uart8250.*Platform Reboot Device *: syscon-reboot.*Platform Shutdown Device *: syscon-poweroff.*Firmware Base *: 0x80000000.*Domain0 Next Address *: 0x80400000.*Domain0 Next Mode *: S-mode.*Boot HART ID *: 0.*Boot HART Base ISA *: rv32imac.*Boot HART PMP Count *: 16.*\\[exit=0\\]"
            boot_sbi_handoff_kernel.elf)
        opensbi_test(reboot "asked for a reboot.*\\[exit=4\\]" boot_sbi_reboot_kernel.elf)

        # S-mode services through OpenSBI (SBI calls, the timer, IPIs, counters): each stub prints a line and shuts down
        opensbi_test(services-base    "sbi base ok.*\\[exit=0\\]"    boot_sbi_base_kernel.elf)
        opensbi_test(services-console "AB.*buffer.*Z.*sbi console ok.*\\[exit=0\\]" boot_sbi_console_kernel.elf)
        opensbi_test(services-timer   "sbi timer ok.*\\[exit=0\\]"   boot_sbi_timer_kernel.elf)
        opensbi_test(services-ipi     "sbi ipi ok.*\\[exit=0\\]"     boot_sbi_ipi_kernel.elf)
        opensbi_test(services-state    "sbi state ok.*\\[exit=0\\]"  boot_sbi_state_kernel.elf)

        # The same stubs on QEMU's virt machine with the same OpenSBI, as an oracle: what the stubs expect of the SBI is what a real
        # implementation does. Optional (needs qemu-system-riscv32).
        find_program(QEMU_RV32 qemu-system-riscv32)
        if(QEMU_RV32)
            function(qemu_test name regex kernel)
                add_test(NAME qemu.${name}
                         COMMAND sh -c "timeout 60 ${QEMU_RV32} -M virt -m 128M -smp 1 -nographic -monitor none -serial stdio -bios ${OPENSBI_FW} -kernel ${S}/${kernel} 2>&1; echo \"[exit=$?]\"")
                set_tests_properties(qemu.${name} PROPERTIES
                    FIXTURES_REQUIRED "boot-stubs;opensbi-fw" TIMEOUT 90 PASS_REGULAR_EXPRESSION "${regex}")
            endfunction()
            qemu_test(handoff        "\\[exit=0\\]"                         boot_sbi_handoff_kernel.elf)
            qemu_test(services-base    "sbi base ok.*\\[exit=0\\]"    boot_sbi_base_kernel.elf)
            qemu_test(services-console "AB.*buffer.*Z.*sbi console ok.*\\[exit=0\\]" boot_sbi_console_kernel.elf)
            qemu_test(services-timer   "sbi timer ok.*\\[exit=0\\]"   boot_sbi_timer_kernel.elf)
            qemu_test(services-ipi     "sbi ipi ok.*\\[exit=0\\]"     boot_sbi_ipi_kernel.elf)
            qemu_test(services-state   "sbi state ok.*\\[exit=0\\]"   boot_sbi_state_kernel.elf)
        endif()
    else()
        message(STATUS "OpenSBI boot tests not registered (need the opensbi/ checkout, riscv64-linux-gnu-gcc, make and dtc)")
    endif()
endif()
