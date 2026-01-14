
add_library(chip
   mos6502.cpp
   mos6522.cpp
   ay3_8912.cpp
   ula.cpp
   wd1793.cpp
)

target_include_directories(chip PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(chip
        PUBLIC
        ${BOOST_LIBRARIES}
        ${SDL2_LIBRARIES}
)

target_link_libraries(chip PUBLIC ${BOOST_LIBRARIES})

