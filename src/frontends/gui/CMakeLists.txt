add_subdirectory(imgui)
add_subdirectory(imgui/backends)

add_library(frontends_gui
        gui.cpp
)

target_include_directories(frontends_gui
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(frontends_gui
        PUBLIC
        chip
        frontend
        frontends_imgui
        ${SDL2_LIBRARIES}
)

