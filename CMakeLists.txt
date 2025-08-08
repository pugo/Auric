cmake_minimum_required(VERSION 3.5)

set(CMAKE_BUILD_TYPE Debug)

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
set(BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

#set(CMAKE_CXX_FLAGS "-O3 -Wno-unsequenced")
set(CMAKE_CXX_FLAGS "-O3 -pg")

add_definitions(-DBOOST_LOG_DYN_LINK -DBOOST_SPIRIT_DEBUG -DBOOST_SPIRIT_DEBUG_ -DBOOST_SPIRIT_DEBUG_FLAGS_TREES)


project(oric)

if (DEFINED CMAKE_TOOLCHAIN_FILE)
    message("Using vcpkg")

    find_package(Boost REQUIRED COMPONENTS log program_options assign circular_buffer)
    find_package(SDL2 CONFIG REQUIRED)
    find_package(SDL2_image CONFIG REQUIRED)
    find_package(SDL2_ttf CONFIG REQUIRED)

    set(BOOST_LIBRARIES Boost::log Boost::program_options Boost::assign Boost::circular_buffer)
    set(SDL2_LIBRARIES SDL2::SDL2 SDL2_image::SDL2_image-static SDL2_ttf::SDL2_ttf-static)
else ()
    message("Not using vcpkg")

    find_package(Boost 1.71.0 COMPONENTS program_options log)
    find_package(SDL2 REQUIRED)
    find_package(SDL2_image REQUIRED)
    find_package(SDL2_ttf REQUIRED)

    set(BOOST_LIBRARIES ${Boost_LIBRARIES})
    set(SDL2_LIBRARIES ${SDL2_LIBRARIES} ${SDL2_IMAGE_LIBRARIES} SDL2_ttf)
endif ()


set (LIB_SOURCES
        oric.cpp
        memory.cpp
        machine.cpp
        monitor.cpp
        config.cpp
        snapshot.cpp
        oric.hpp
)

include_directories(${Boost_INCLUDE_DIRS} ${SDL2_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIRS} .)

include(chip/CMakeLists.txt)
include(tape/CMakeLists.txt)
include(frontends/sdl/CMakeLists.txt)

add_executable(oric main.cpp ${LIB_SOURCES} )
target_link_libraries(oric
        PRIVATE
        ${BOOST_LIBRARIES} ${SDL2_LIBRARIES}
)

add_library(oric_lib STATIC ${LIB_SOURCES})
target_link_libraries(oric_lib ${BOOST_LIBRARIES} ${SDL2_LIBRARIES})

install(TARGETS oric RUNTIME DESTINATION bin)

enable_testing()
add_subdirectory(tests)



