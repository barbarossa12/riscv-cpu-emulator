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
endif()
