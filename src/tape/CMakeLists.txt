
add_library(tape
   tape_tap.cpp
   tape_blank.cpp
)

target_include_directories(tape PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})


target_link_libraries(tape
        PUBLIC
        chip
        ${BOOST_LIBRARIES}
)

