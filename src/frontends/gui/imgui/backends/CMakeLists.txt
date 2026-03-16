
add_library(frontends_imgui_backends
        imgui_impl_sdl3.cpp
        imgui_impl_opengl3.cpp
)

target_include_directories(frontends_imgui_backends
        PUBLIC
        .
)

target_link_libraries(frontends_imgui_backends
        PUBLIC
        frontends_imgui
        ${GLAD_TARGET}
        OpenGL::GL
)

target_compile_definitions(frontends_imgui_backends
        PRIVATE
        IMGUI_IMPL_OPENGL_LOADER_CUSTOM
)
