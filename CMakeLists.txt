cmake_minimum_required(VERSION 3.16)
project(riscv_emu C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# treat warnings as errors.
add_compile_options(-Wall -Wextra -Werror)

# Debug info always available; switch build type with
# -DCMAKE_BUILD_TYPE=Release when we care about speed later.
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()
set(CMAKE_C_FLAGS_DEBUG "-g -O0")
set(CMAKE_C_FLAGS_RELEASE "-O2")

# Core emulator library (everything except main()), so tests can link
# against the same code without pulling in a main().
add_library(riscv_core
    src/cpu.c
    src/mem.c
    src/log.c
    src/pmp.c
    src/mmu.c
    src/clint.c
    src/plic.c
    src/uart.c
    src/uart_host.c
    src/decode.c
    src/compressed.c
    src/execute.c
    src/elf.c
)
target_include_directories(riscv_core PUBLIC src)

# The actual emulator executable.
add_executable(riscv_emu src/main.c)
target_link_libraries(riscv_emu PRIVATE riscv_core)

enable_testing()
add_subdirectory(tests)

# The device tree. dtc prints nothing for a clean tree, so any output on stderr (a warning) fails the build.
find_program(DTC dtc)
find_program(FDTGET fdtget)
if(DTC AND FDTGET)
    set(RISCV_EMU_DTB ${CMAKE_BINARY_DIR}/riscv-emu.dtb)
    add_custom_command(
        OUTPUT ${RISCV_EMU_DTB}
        COMMAND sh -c "${DTC} -I dts -O dtb -o ${RISCV_EMU_DTB} ${CMAKE_SOURCE_DIR}/dts/riscv-emu.dts 2> ${RISCV_EMU_DTB}.log; rc=$?; cat ${RISCV_EMU_DTB}.log; test $rc -eq 0 -a ! -s ${RISCV_EMU_DTB}.log || { rm -f ${RISCV_EMU_DTB}; exit 1; }"
        DEPENDS ${CMAKE_SOURCE_DIR}/dts/riscv-emu.dts
        VERBATIM)
    add_custom_target(riscv_emu_dtb ALL DEPENDS ${RISCV_EMU_DTB})
    add_test(NAME dts-matches-headers
             COMMAND sh ${CMAKE_SOURCE_DIR}/tests/check-dts.sh ${RISCV_EMU_DTB} ${CMAKE_SOURCE_DIR})
else()
    message(STATUS "dtc or fdtget not found: the device tree is not built or checked")
endif()
