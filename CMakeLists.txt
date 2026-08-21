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
)
target_include_directories(riscv_core PUBLIC src)

# The actual emulator executable.
add_executable(riscv_emu src/main.c)
target_link_libraries(riscv_emu PRIVATE riscv_core)

enable_testing()
add_subdirectory(tests)
