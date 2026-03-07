
add_library(frontends_imgui
        imgui.cpp
        imgui_draw.cpp
        imgui_stdlib.cpp
        imgui_tables.cpp
        imgui_widgets.cpp
)

target_include_directories(frontends_imgui
        PUBLIC
        .
)

target_link_libraries(frontends_imgui
        PUBLIC
        ${SDL3_LIBRARIES}
        frontends_imgui_backends

)

