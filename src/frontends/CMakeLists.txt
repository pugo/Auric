
add_library(frontend
        sdl/frontend.cpp
        sdl/status_bar.cpp
        sdl/texture.cpp
        imgui/imgui.cpp
        imgui/imgui_draw.cpp
        imgui/imgui_impl_sdl2.cpp
        imgui/imgui_impl_sdlrenderer2.cpp
        imgui/imgui_tables.cpp
        imgui/imgui_widgets.cpp
)

target_include_directories(frontend PUBLIC sdl imgui)

target_link_libraries(frontend
        PUBLIC
        chip
        ${BOOST_LIBRARIES}
        ${SDL2_LIBRARIES}
        ${IMGUI_LIBRARIES}
)

