
add_library(frontend_sdl2
        frontend.cpp
        status_bar.cpp
        texture.cpp
)

target_include_directories(frontend_sdl2
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(frontend_sdl2
        PUBLIC
        chip
        ${BOOST_LIBRARIES}
        ${SDL2_LIBRARIES}
        frontends_imgui
)

