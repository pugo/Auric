
add_library(frontend
   sdl/frontend.cpp
   sdl/status_bar.cpp
   sdl/texture.cpp
)

target_include_directories(frontend PUBLIC sdl)

target_link_libraries(frontend
        PUBLIC
        chip
        ${BOOST_LIBRARIES}
        ${SDL2_LIBRARIES}
)

