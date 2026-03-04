
add_library(frontends_imgui_backends
        imgui_impl_sdl2.cpp
        imgui_impl_sdlrenderer2.cpp
)

target_include_directories(frontends_imgui_backends
        PUBLIC
        .
)

target_link_libraries(frontends_imgui_backends
        PUBLIC
        frontends_imgui
)

