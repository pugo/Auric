
add_library(frontend_sdl3
        frontend.cpp
        status_bar.cpp
        texture.cpp
)

target_include_directories(frontend_sdl3
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(frontend_sdl3
        PUBLIC
        chip
        ${BOOST_LIBRARIES}
        ${SDL3_LIBRARIES}
        frontends_gui
)

