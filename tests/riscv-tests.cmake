# --- Official riscv-tests ---------------------------------------------------
# Runs the rv32ui-p-* and rv32mi-p-* binaries from the riscv-tests repo through
# riscv_emu. The ELFs are built into <build>/riscv-tests/ by
# scripts/build-riscv-tests.sh, which a CTest setup test runs automatically
# (and the riscv-tests-elfs target runs by hand).
#
# Needs a RISC-V gcc on PATH and the riscv-tests `env` submodule. Without them
# these tests are simply not registered; configure with -DRISCV_TESTS=OFF to
# skip them even when the toolchain is present.

find_program(RISCV_GCC NAMES riscv32-unknown-elf-gcc riscv64-unknown-elf-gcc)
set(_riscv_tests_default OFF)
if(RISCV_GCC)
    set(_riscv_tests_default ON)
endif()
option(RISCV_TESTS "Register the official riscv-tests (needs a RISC-V gcc)" ${_riscv_tests_default})
set(RISCV_TESTS_SRC_DIR "${CMAKE_SOURCE_DIR}/riscv-tests" CACHE PATH "riscv-tests checkout")
set(RISCV_TESTS_BUILD_DIR "${CMAKE_BINARY_DIR}/riscv-tests")

if(RISCV_TESTS AND NOT EXISTS "${RISCV_TESTS_SRC_DIR}/env/p/riscv_test.h")
    message(WARNING "riscv-tests: ${RISCV_TESTS_SRC_DIR}/env is empty, not registering them. "
                    "Run: git -C ${RISCV_TESTS_SRC_DIR} submodule update --init")
    set(RISCV_TESTS OFF)
endif()

if(RISCV_TESTS)
    set(RV32UI_TESTS
        simple add addi and andi auipc beq bge bgeu blt bltu bne fence_i jal jalr
        lb lbu lh lhu lw ld_st lui ma_data or ori sb sh sw st_ld sll slli
        slt slti sltiu sltu sra srai srl srli sub xor xori)
    set(RV32MI_TESTS
        breakpoint csr mcsr illegal ma_fetch ma_addr scall sbreak shamt
        lw-misaligned lh-misaligned sh-misaligned sw-misaligned
        zicntr instret_overflow pmpaddr)

    # Tests we deliberately do not run yet. Each entry needs a reason:
    #   set(RISCV_SKIP_<test-name> "reason")
    # A skipped test is still registered, as disabled, so it shows up in the
    # CTest output instead of silently vanishing. Remove the entry when the
    # feature lands.

    set(_names "")
    foreach(t ${RV32UI_TESTS})
        list(APPEND _names rv32ui-p-${t})
    endforeach()
    foreach(t ${RV32MI_TESTS})
        list(APPEND _names rv32mi-p-${t})
    endforeach()

    set(_build_cmd "${CMAKE_SOURCE_DIR}/scripts/build-riscv-tests.sh"
        "${RISCV_TESTS_SRC_DIR}" "${RISCV_TESTS_BUILD_DIR}" ${_names})

    add_custom_target(riscv-tests-elfs COMMAND ${_build_cmd} VERBATIM
        COMMENT "Building riscv-tests ELFs into ${RISCV_TESTS_BUILD_DIR}")

    add_test(NAME riscv-tests.build COMMAND ${_build_cmd})
    set_tests_properties(riscv-tests.build PROPERTIES
        FIXTURES_SETUP riscv-tests-elfs TIMEOUT 300)

    # The mode trace. With RISCV_EMU_LOG=debug an rv32ui test shows its ecall from
    # U-mode (which also proves the test body runs in U-mode), and without the variable
    # nothing is printed.
    add_test(NAME riscv-tests.trace-mode-on
             COMMAND ${CMAKE_COMMAND} -E env RISCV_EMU_LOG=debug
                     $<TARGET_FILE:riscv_emu> "${RISCV_TESTS_BUILD_DIR}/rv32ui-p-add")
    set_tests_properties(riscv-tests.trace-mode-on PROPERTIES
        FIXTURES_REQUIRED riscv-tests-elfs TIMEOUT 60 LABELS riscv-tests
        PASS_REGULAR_EXPRESSION "trap: U -> M, cause 8")
    add_test(NAME riscv-tests.trace-mode-off
             COMMAND riscv_emu "${RISCV_TESTS_BUILD_DIR}/rv32ui-p-add")
    set_tests_properties(riscv-tests.trace-mode-off PROPERTIES
        FIXTURES_REQUIRED riscv-tests-elfs TIMEOUT 60 LABELS riscv-tests
        FAIL_REGULAR_EXPRESSION "trap:")

    # rv32mi-p-illegal passes through an early exit when S-mode is absent. Check that it really
    # runs its S-mode sections: it must enter S with MRET and execute a legal SRET, and still pass.
    add_test(NAME riscv-tests.illegal-runs-s-mode
             COMMAND ${CMAKE_COMMAND} -E env RISCV_EMU_LOG=debug
                     $<TARGET_FILE:riscv_emu> "${RISCV_TESTS_BUILD_DIR}/rv32mi-p-illegal")
    set_tests_properties(riscv-tests.illegal-runs-s-mode PROPERTIES
        FIXTURES_REQUIRED riscv-tests-elfs TIMEOUT 60 LABELS riscv-tests
        PASS_REGULAR_EXPRESSION "mret: M -> S.*sret: S -> S.*PASS")

    # Assembly tests of our own (tests/asm), built like the rv32mi tests and run like them.
    set(_asm_sources ${CMAKE_SOURCE_DIR}/tests/asm/pmp_denial.S)
    add_test(NAME riscv-tests.build-asm
             COMMAND "${CMAKE_SOURCE_DIR}/scripts/build-asm-tests.sh"
                     "${RISCV_TESTS_SRC_DIR}" "${RISCV_TESTS_BUILD_DIR}" ${_asm_sources})
    set_tests_properties(riscv-tests.build-asm PROPERTIES
        FIXTURES_SETUP riscv-tests-elfs TIMEOUT 300)

    # PMP denial, end to end: in U-mode a load, a store and a fetch that hit a no-permission region
    # must trap with the right cause and mtval (the unit tests cover denial; the official tests only
    # exercise the allowed path through the harness's allow-all entry).
    add_test(NAME riscv-tests.pmp-denial
             COMMAND riscv_emu "${RISCV_TESTS_BUILD_DIR}/pmp_denial")
    set_tests_properties(riscv-tests.pmp-denial PROPERTIES
        FIXTURES_REQUIRED riscv-tests-elfs TIMEOUT 60 LABELS riscv-tests)

    foreach(name ${_names})
        add_test(NAME riscv-tests.${name}
                 COMMAND riscv_emu "${RISCV_TESTS_BUILD_DIR}/${name}")
        set_tests_properties(riscv-tests.${name} PROPERTIES
            FIXTURES_REQUIRED riscv-tests-elfs TIMEOUT 60 LABELS riscv-tests)
        if(DEFINED RISCV_SKIP_${name})
            set_tests_properties(riscv-tests.${name} PROPERTIES DISABLED TRUE)
            message(STATUS "riscv-tests: skipping ${name}: ${RISCV_SKIP_${name}}")
        endif()
    endforeach()
endif()
