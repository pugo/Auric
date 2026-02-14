

if (DEFINED CMAKE_TOOLCHAIN_FILE)
    message("Using vcpkg")

    find_package(Boost REQUIRED COMPONENTS log program_options circular_buffer)
    find_package(SDL2 CONFIG REQUIRED)
    find_package(SDL2_image CONFIG REQUIRED)

    set(BOOST_LIBRARIES Boost::log Boost::program_options Boost::circular_buffer)
    set(SDL2_LIBRARIES SDL2::SDL2 SDL2_image::SDL2_image-static)
else ()
    message("Not using vcpkg")

    find_package(Boost 1.71.0 COMPONENTS program_options log)
    find_package(SDL2 REQUIRED)
    find_package(SDL2_image REQUIRED)

    set(BOOST_LIBRARIES ${Boost_LIBRARIES})
    set(SDL2_LIBRARIES ${SDL2_LIBRARIES} ${SDL2_IMAGE_LIBRARIES})
endif ()

add_subdirectory(chip)
add_subdirectory(disk)
add_subdirectory(tape)
add_subdirectory(frontends)

# Static library (for reuse by tests, etc.)
add_library(auric_lib STATIC
        oric.cpp
        memory.cpp
        machine.cpp
        monitor.cpp
        config.cpp
        snapshot.cpp
)

target_include_directories(auric_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(auric_lib PUBLIC chip frontend tape ${BOOST_LIBRARIES} ${SDL2_LIBRARIES})

# Link everything here (chip/tape/frontend libs defined in their subdirs)
target_link_libraries(auric_lib
        PUBLIC
        chip
        disk
        frontend
        tape
        ${BOOST_LIBRARIES}
)

# Main executable
add_executable(auric main.cpp)
target_link_libraries(auric PRIVATE auric_lib)

install(TARGETS auric RUNTIME DESTINATION bin)
