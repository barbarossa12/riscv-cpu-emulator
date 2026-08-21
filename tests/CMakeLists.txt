include(FetchContent)

FetchContent_Declare(
    unity
    GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
    GIT_TAG        v2.6.0
)
FetchContent_MakeAvailable(unity)

# --- Normal (non-crashing) unit tests -------------------------------------
# Each of these links against riscv_core + unity, and is expected to exit 0.

# add_executable(test_mem test_mem.c)
# target_link_libraries(test_mem PRIVATE riscv_core unity)
# add_test(NAME test_mem COMMAND test_mem)

# add_executable(test_cpu test_cpu.c)
# target_link_libraries(test_cpu PRIVATE riscv_core unity)
# add_test(NAME test_cpu COMMAND test_cpu)

# --- Abort-path tests -------------------------------------------------------
# Each of these is a tiny standalone executable (NOT using Unity) that
# deliberately triggers one specific LOG_FATAL/abort() path. CTest normally
# treats a non-zero/signal exit as failure; WILL_FAIL inverts that, and
# PASS_REGULAR_EXPRESSION additionally checks stderr for the expected
# message, so the test only "passes" if it crashed for the RIGHT reason.

add_executable(test_mem_read8_oob test_mem_read8_oob.c)
target_link_libraries(test_mem_read8_oob PRIVATE riscv_core)

add_test(
    NAME test_mem_read8_oob
    COMMAND ${CMAKE_COMMAND} -E env $<TARGET_FILE:test_mem_read8_oob>
)
set_tests_properties(test_mem_read8_oob PROPERTIES
    PASS_REGULAR_EXPRESSION "out-of-bound"
)

add_executable(test_cpu_reg_oob test_cpu_reg_oob.c)
target_link_libraries(test_cpu_reg_oob PRIVATE riscv_core)

add_test(
    NAME test_cpu_reg_oob
    COMMAND ${CMAKE_COMMAND} -E env $<TARGET_FILE:test_cpu_reg_oob>
)
set_tests_properties(test_cpu_reg_oob PROPERTIES
    PASS_REGULAR_EXPRESSION "larger than 32"
)

