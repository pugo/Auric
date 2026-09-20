
add_library(tape
        tape.cpp
        tape_autostarter.cpp
        tape_blank.cpp
        tape_tap.cpp
        tape_tap_normal.cpp
        tape_tap_turbo.cpp
)

target_include_directories(tape PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})


target_link_libraries(tape
        PUBLIC
        chip
        utils
        ${AURIC_COMMON_LIBRARIES}
)
