

if (DEFINED CMAKE_TOOLCHAIN_FILE)
    message("Using vcpkg")

    find_package(Boost REQUIRED COMPONENTS log program_options assign circular_buffer)
    find_package(SDL2 CONFIG REQUIRED)
    find_package(SDL2_image CONFIG REQUIRED)

    set(BOOST_LIBRARIES Boost::log Boost::program_options Boost::assign Boost::circular_buffer)
    set(SDL2_LIBRARIES SDL2::SDL2 SDL2_image::SDL2_image-static)
else ()
    message("Not using vcpkg")

    find_package(Boost 1.71.0 COMPONENTS program_options log)
    find_package(SDL2 REQUIRED)
    find_package(SDL2_image REQUIRED)

    set(BOOST_LIBRARIES ${Boost_LIBRARIES})
    set(SDL2_LIBRARIES ${SDL2_LIBRARIES} ${SDL2_IMAGE_LIBRARIES})

    target_link_libraries(oric_lib PUBLIC chip frontend tape ${BOOST_LIBRARIES} ${SDL2_LIBRARIES})
endif ()

add_subdirectory(chip)
add_subdirectory(disk)
add_subdirectory(tape)
add_subdirectory(frontends)

# Static library (for reuse by tests, etc.)
add_library(oric_lib STATIC
        oric.cpp
        memory.cpp
        machine.cpp
        monitor.cpp
        config.cpp
        snapshot.cpp
)

target_include_directories(oric_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(oric_lib PUBLIC chip frontend tape ${BOOST_LIBRARIES} ${SDL2_LIBRARIES})

# Link everything here (chip/tape/frontend libs defined in their subdirs)
target_link_libraries(oric_lib
        PUBLIC
        chip
        disk
        frontend
        tape
        ${BOOST_LIBRARIES}
)

# Main executable
add_executable(oric main.cpp)
target_link_libraries(oric PRIVATE oric_lib)

install(TARGETS oric RUNTIME DESTINATION bin)
