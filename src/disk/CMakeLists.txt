
add_library(disk
        disk_microdrive.cpp
        disk_none.cpp
)

target_include_directories(disk PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})


target_link_libraries(disk
        PUBLIC
        chip
        ${BOOST_LIBRARIES}
)

