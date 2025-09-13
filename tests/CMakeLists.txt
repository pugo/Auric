project(tests)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/releases/download/v1.15.2/googletest-1.15.2.tar.gz
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})
include_directories(${gmock_SOURCE_DIR}/include ${gmock_SOURCE_DIR})

add_executable(gtests_run
        6502_test.cpp
        6522_test_registers.cpp
        6522_test_control_registers.cpp
        6522_test_t1.cpp
        6522_test_t2.cpp
        6522_test_shift_registers.cpp
        mocks/test_machine.cpp
        mocks/test_machine.h
)

target_link_libraries(gtests_run  gtest_main gmock oric_lib)

add_test(NAME gtests_run COMMAND gtests_run)