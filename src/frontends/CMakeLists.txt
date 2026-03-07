add_subdirectory(gui)
add_subdirectory(sdl)

add_library(frontend INTERFACE)
target_link_libraries(frontend INTERFACE frontend_sdl3)

