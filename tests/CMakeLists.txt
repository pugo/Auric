project(tests)

find_package(Boost REQUIRED)
find_package(SDL2 REQUIRED)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/releases/download/v1.15.2/googletest-1.15.2.tar.gz
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})
include_directories(${gmock_SOURCE_DIR}/include ${gmock_SOURCE_DIR})
include_directories(src)

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

target_include_directories(gtests_run PUBLIC ../src)

target_link_libraries(gtests_run  gtest_main gmock auric_lib ${BOOST_LIBRARIES} ${SDL2_LIBRARIES})

add_test(NAME gtests_run COMMAND gtests_run)