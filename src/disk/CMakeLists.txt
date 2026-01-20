
add_library(disk
        disk_image.cpp
        drive_microdrive.cpp
        drive_none.cpp
)

target_include_directories(disk PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})


target_link_libraries(disk
        PUBLIC
        chip
        ${BOOST_LIBRARIES}
)

