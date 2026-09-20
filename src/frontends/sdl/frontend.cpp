// =========================================================================
//   Copyright (C) 2009-2026 by Anders Piniesjö <pugo@pugo.org>
//
//   This program is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with this program.  If not, see <http://www.gnu.org/licenses/>
// =========================================================================

#include <spdlog/spdlog.h>
#include <algorithm>
#include <format>
#include <unordered_map>

#include <SDL3/SDL.h>

#include "file_dialog.hpp"
#include "frontend.hpp"
#include "gl_loader.hpp"
#include "frontends/gui/status_bar.hpp"

#include "chip/ay3_8912.hpp"
#include "oric.hpp"

static SDL_Scancode scancode_map[] = {
    SDL_SCANCODE_7,     SDL_SCANCODE_N,     SDL_SCANCODE_5,         SDL_SCANCODE_V,     SDL_SCANCODE_UNKNOWN,   SDL_SCANCODE_1,         SDL_SCANCODE_X,             SDL_SCANCODE_3,
    SDL_SCANCODE_J,     SDL_SCANCODE_T,     SDL_SCANCODE_R,         SDL_SCANCODE_F,     SDL_SCANCODE_UNKNOWN,   SDL_SCANCODE_ESCAPE,    SDL_SCANCODE_Q,             SDL_SCANCODE_D,
    SDL_SCANCODE_M,     SDL_SCANCODE_6,     SDL_SCANCODE_B,         SDL_SCANCODE_4,     SDL_SCANCODE_LCTRL,     SDL_SCANCODE_Z,         SDL_SCANCODE_2,             SDL_SCANCODE_C,
    SDL_SCANCODE_K,     SDL_SCANCODE_9,     SDL_SCANCODE_SEMICOLON, SDL_SCANCODE_MINUS, SDL_SCANCODE_UNKNOWN,   SDL_SCANCODE_UNKNOWN,   SDL_SCANCODE_BACKSLASH,     SDL_SCANCODE_APOSTROPHE,
    SDL_SCANCODE_SPACE, SDL_SCANCODE_COMMA, SDL_SCANCODE_PERIOD,    SDL_SCANCODE_UP,    SDL_SCANCODE_LSHIFT,    SDL_SCANCODE_LEFT,      SDL_SCANCODE_DOWN,          SDL_SCANCODE_RIGHT,
    SDL_SCANCODE_U,     SDL_SCANCODE_I,     SDL_SCANCODE_O,         SDL_SCANCODE_P,     SDL_SCANCODE_LALT,      SDL_SCANCODE_BACKSPACE, SDL_SCANCODE_RIGHTBRACKET,  SDL_SCANCODE_LEFTBRACKET,
    SDL_SCANCODE_Y,     SDL_SCANCODE_H,     SDL_SCANCODE_G,         SDL_SCANCODE_E,     SDL_SCANCODE_UNKNOWN,   SDL_SCANCODE_A,         SDL_SCANCODE_S,             SDL_SCANCODE_W,
    SDL_SCANCODE_8,     SDL_SCANCODE_L,     SDL_SCANCODE_0,         SDL_SCANCODE_SLASH, SDL_SCANCODE_RSHIFT,    SDL_SCANCODE_RETURN,    SDL_SCANCODE_UNKNOWN,       SDL_SCANCODE_EQUALS
};

std::unordered_map<SDL_Scancode, uint8_t> oric_key_map;


// ----- Frontend ----------------

constexpr uint16_t status_bar_height = 20;
constexpr float layout_gutter_width = 16.0f;
constexpr float layout_min_margin = 8.0f;
constexpr float layout_max_margin = 80.0f;

constexpr std::string window_title = "Auric";
constexpr std::string window_icon_name = "window_icon.png";

namespace
{
#include "gl_screen_program.cpp"
}


Frontend::Frontend(Oric& oric) :
    oric(oric),
    sdl_window(nullptr),
    gl_context(nullptr),
    gl_program(0),
    gl_vao(0),
    gl_vbo(0),
    gl_u_texture(-1),
    gui(oric),
    oric_texture(texture_width, texture_height, texture_bpp),
    sound_audio_stream(nullptr),
    audio_locked(false),
    current_window_width(0),
    current_window_height(0)
{
    for (uint8_t i = 0; i < 64; ++i) {
        if (scancode_map[i] != 0) {
            oric_key_map[scancode_map[i]] = i;
        }
    }

    enable_scanlines = oric.get_config().enable_scanlines() ? 1 : 0;
    enable_vertical_lines = oric.get_config().enable_vertical_lines() ? 1 : 0;
    enable_vignette = oric.get_config().enable_vignette() ? 1 : 0;
    vignette_strength = oric.get_config().vignette_strength();
}

Frontend::~Frontend()
{
    close_sound();
    close_graphics();
    close_sdl();
}

bool Frontend::init_graphics()
{
    SDL_SetHint(SDL_HINT_APP_NAME, window_title.c_str());

    // Initialize SDL
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        spdlog::error("SDL could not initialize! SDL_Error: {}", SDL_GetError());
        return false;
    }

    auto window_width = oric.get_config().window_width();
    auto window_height = oric.get_config().window_height();

    gui.status_bar().set_size(window_width, status_bar_height);
    gui.status_bar().set_position(0, window_height - status_bar_height);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    sdl_window = SDL_CreateWindow(window_title.c_str(), window_width, window_height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (sdl_window == nullptr) {
        spdlog::error("Window could not be created! SDL_Error: {}", SDL_GetError());
        return false;
    }

    handle_window_resize(window_width, window_height);

    // Set minimum window size to ensure content is always visible
    uint16_t min_width = texture_width * 1.10;
    uint16_t min_height = texture_height * 1.10;
    SDL_SetWindowMinimumSize(sdl_window, min_width, min_height);

    gl_context = SDL_GL_CreateContext(sdl_window);
    if (gl_context == nullptr) {
        spdlog::error("OpenGL context could not be created! SDL_Error: {}", SDL_GetError());
        return false;
    }
    SDL_GL_MakeCurrent(sdl_window, gl_context);
    SDL_GL_SetSwapInterval(1);

    if (!load_gl_functions()) {
        spdlog::error("Failed to initialize GLAD OpenGL function loader");
        return false;
    }

    // Try to load a window icon.
    auto path = oric.get_config().images_path() / window_icon_name;
    if (std::filesystem::exists(path)) {
        SDL_Surface* icon = SDL_LoadSurface(path.string().c_str());
        if (! icon) {
            spdlog::error("Failed loading application icon: {}: {}", path.string(), SDL_GetError());
        }
        else {
            if (!SDL_SetWindowIcon(sdl_window, icon)) {
                spdlog::error("Failed setting application icon");
            }
            SDL_DestroySurface(icon);
        }
    }

    return init_gl();
}


bool Frontend::init_sound()
{
    spdlog::debug("Initializing sound..");

    if (!SDL_Init(SDL_INIT_AUDIO))
    {
        spdlog::error("Error: failed initializing SDL: {}", SDL_GetError());
        return false;
    }

    SDL_AudioSpec audio_spec_want;
    SDL_zero(audio_spec_want);

    audio_spec_want.freq     = 44100;
    audio_spec_want.format   = SDL_AUDIO_S16LE;
    audio_spec_want.channels = 2;
    AY3_8912* ay3 = oric.get_machine().ay3.get();

    sound_audio_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &audio_spec_want,
                                                    AY3_8912::audio_callback, (void*) ay3);

    if (!sound_audio_stream)
    {
        spdlog::error("Error: creating SDL audio stream: {}", SDL_GetError());
        return false;
    }

    SDL_AudioSpec audio_spec;
    if (SDL_GetAudioStreamFormat(sound_audio_stream, &audio_spec, nullptr)) {
        spdlog::debug("Freq: {}", static_cast<int>(audio_spec.freq));
        spdlog::debug("format: {}", static_cast<int>(audio_spec.format));
        spdlog::debug("channels: {}", static_cast<int>(audio_spec.channels));
    }

    return true;
}


void Frontend::pause_sound(bool pause_on)
{
    if (pause_on) {
        SDL_PauseAudioStreamDevice(sound_audio_stream);
    } else {
        SDL_ResumeAudioStreamDevice(sound_audio_stream);
    }
}


void Frontend::clear_audio()
{
    if (sound_audio_stream) {
        SDL_ClearAudioStream(sound_audio_stream);
    }
}


bool Frontend::handle_frame()
{
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        bool wanted_key = false;
        bool wanted_mouse = false;
        gui.handle_event(event, wanted_key, wanted_mouse);

        auto scancode = event.key.scancode;

        // Toggle gui regardless of ImGui.
        if (event.type == SDL_EVENT_KEY_DOWN) {
            if (scancode == SDL_SCANCODE_F1) {
                gui.toggle_gui();
            }
        }

        if (!wanted_key) {
            switch (event.type) {
                case SDL_EVENT_KEY_DOWN:
                case SDL_EVENT_KEY_UP:
                {
                    bool special_pressed{false};

                    if (event.type == SDL_EVENT_KEY_DOWN) {
                        if (event.key.mod & SDL_KMOD_CTRL) {
                            if (scancode == SDL_SCANCODE_W) {
                                oric.get_machine().toggle_warp_mode();
                                special_pressed = true;
                            }

                            else if (scancode == SDL_SCANCODE_R) {
                                oric.get_machine().cpu->NMI();
                                special_pressed = true;
                            }

                            else if (scancode == SDL_SCANCODE_B) {
                                oric.break_execution();
                                special_pressed = true;
                            }
                        }
                        else {
                            if (scancode == SDL_SCANCODE_F2) {
                                oric.get_machine().save_snapshot();
                                special_pressed = true;
                            }

                            else if (scancode == SDL_SCANCODE_F3) {
                                oric.get_machine().load_snapshot();
                                special_pressed = true;
                            }
                        }
                    }

                    if (! special_pressed) {
                        if (!special_pressed) {
                            auto key = oric_key_map.find(scancode);
                            if (key != oric_key_map.end()) {
                                oric.get_machine().key_press(key->second, event.type == SDL_EVENT_KEY_DOWN);
                            }
                        }
                    }
                    break;
                }
            }
        }

        if (!wanted_mouse) {
            switch (event.type)
            {
                case SDL_EVENT_WINDOW_RESIZED:
                    if (event.window.windowID == SDL_GetWindowID(sdl_window)) {
                        // Window was resized, dimensions will be recalculated in render_graphics
                    }
                    break;

                case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                    if (event.window.windowID == SDL_GetWindowID(sdl_window)) {
                        oric.do_quit();
                        return false;
                    }
                break;
            }
        }
    }
    return true;
}


void Frontend::render_graphics(std::vector<uint8_t>& pixels)
{
    int window_width = 0;
    int window_height = 0;
    SDL_GetWindowSizeInPixels(sdl_window, &window_width, &window_height);

    if (window_width != current_window_width || window_height != current_window_height) {
        handle_window_resize(window_width, window_height);
        gui.status_bar().show_text_for(std::format("[{}x{}]", window_width, window_height), std::chrono::milliseconds(2000));
    }
    update_render_rect(window_width, window_height);

    glViewport(0, 0, window_width, window_height);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    oric_texture.update_pixels(pixels.data());

    const float left = (oric_texture.render_rect.x / static_cast<float>(window_width)) * 2.0f - 1.0f;
    const float right = ((oric_texture.render_rect.x + oric_texture.render_rect.w) / static_cast<float>(window_width)) * 2.0f - 1.0f;
    const float top = 1.0f - (oric_texture.render_rect.y / static_cast<float>(window_height)) * 2.0f;
    const float bottom = 1.0f - ((oric_texture.render_rect.y + oric_texture.render_rect.h) / static_cast<float>(window_height)) * 2.0f;

    const float vertices[] = {
        left,  top,    0.0f, 0.0f,
        left,  bottom, 0.0f, 1.0f,
        right, top,    1.0f, 0.0f,
        right, bottom, 1.0f, 1.0f
    };

    glUseProgram(gl_program);
    glUniform1i(gl_u_enable_scanlines, enable_scanlines);
    glUniform1i(gl_u_enable_vertical_lines, enable_vertical_lines);
    glUniform1i(gl_u_enable_vignette, enable_vignette);
    glUniform1f(gl_u_vignette_strength, vignette_strength);
    glUniform1f(gl_u_texture_height, static_cast<float>(texture_height));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, oric_texture.texture);
    glUniform1i(gl_u_texture, 0);

    glBindVertexArray(gl_vao);
    glBindBuffer(GL_ARRAY_BUFFER, gl_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);

    gui.render();

    SDL_GL_SwapWindow(sdl_window);
}


bool Frontend::init_gl()
{
    gl_program = create_screen_program();
    if (gl_program == 0) {
        return false;
    }
    const GLint pos_attr = glGetAttribLocation(gl_program, "a_pos");
    const GLint uv_attr = glGetAttribLocation(gl_program, "a_uv");
    if (pos_attr < 0 || uv_attr < 0) {
        spdlog::error("Failed to resolve OpenGL shader attributes");
        return false;
    }
    gl_u_texture = glGetUniformLocation(gl_program, "u_texture");
    if (gl_u_texture < 0) {
        spdlog::error("Failed to resolve OpenGL shader uniforms");
        return false;
    }

    gl_u_enable_scanlines = glGetUniformLocation(gl_program, "u_enable_scanlines");
    gl_u_enable_vertical_lines = glGetUniformLocation(gl_program, "u_enable_vertical_lines");
    gl_u_enable_vignette = glGetUniformLocation(gl_program, "u_enable_vignette");
    gl_u_vignette_strength = glGetUniformLocation(gl_program, "u_vignette_strength");
    gl_u_texture_height = glGetUniformLocation(gl_program, "u_texture_height");

    glGenVertexArrays(1, &gl_vao);
    glBindVertexArray(gl_vao);
    glGenBuffers(1, &gl_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, gl_vbo);
    glBufferData(GL_ARRAY_BUFFER, 16 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(static_cast<GLuint>(pos_attr));
    glVertexAttribPointer(static_cast<GLuint>(pos_attr), 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(static_cast<GLuint>(uv_attr));
    glVertexAttribPointer(static_cast<GLuint>(uv_attr), 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    if (! oric_texture.create_texture()) {
        return false;
    }

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    SDL_GL_SwapWindow(sdl_window);

    // Initialize Dear Imgui GUI.
    gui.set_video_params(enable_scanlines > 0, enable_vertical_lines > 0, enable_vignette > 0, vignette_strength);
    gui.init(sdl_window, gl_context);

    return true;
}


void Frontend::close_graphics()
{
    if (sdl_window != nullptr && gl_context != nullptr) {
        SDL_GL_MakeCurrent(sdl_window, gl_context);
    }

    gui.close();

    oric_texture.destroy_texture();

    if (gl_vbo != 0) {
        glDeleteBuffers(1, &gl_vbo);
        gl_vbo = 0;
    }
    if (gl_vao != 0) {
        glDeleteVertexArrays(1, &gl_vao);
        gl_vao = 0;
    }
    if (gl_program != 0) {
        glDeleteProgram(gl_program);
        gl_program = 0;
    }

    if (gl_context != nullptr) {
        SDL_GL_DestroyContext(gl_context);
        gl_context = nullptr;
    }

    if (sdl_window != nullptr) {
        SDL_DestroyWindow(sdl_window);
        sdl_window = nullptr;
    }
}


void Frontend::close_sdl()
{
    SDL_Quit(); // Quit all SDL subsystems
}


void Frontend::handle_window_resize(int32_t window_width, int32_t window_height)
{
    current_window_width = window_width;
    current_window_height = window_height;

    // Update status bar
    gui.status_bar().set_position(0, window_height - status_bar_height);
    gui.status_bar().set_size(window_width, status_bar_height);
}

void Frontend::update_render_rect(int32_t window_width, int32_t window_height)
{
    if (window_width <= 0 || window_height <= 0) {
        return;
    }

    const float margin_x = std::clamp(static_cast<float>(window_width) * 0.05f, layout_min_margin, layout_max_margin);
    const float margin_y = std::clamp(static_cast<float>(window_height) * 0.05f, layout_min_margin, layout_max_margin);

    float available_left = margin_x;
    float available_right = static_cast<float>(window_width) - margin_x;

    if (gui.wants_side_panel()) {
        available_right = std::min(available_right, gui.side_panel_origin(window_width).x - layout_gutter_width);
    }

    const float available_top = margin_y;
    const float available_bottom = static_cast<float>(window_height) - status_bar_height - margin_y;
    const float content_width = std::max(1.0f, available_right - available_left);
    const float content_height = std::max(1.0f, available_bottom - available_top);

    const float zoom_x = content_width / texture_width;
    const float zoom_y = content_height / texture_height;
    const float dynamic_zoom = std::max(0.01f, std::min(zoom_x, zoom_y));

    const float new_width = texture_width * dynamic_zoom;
    const float new_height = texture_height * dynamic_zoom;

    oric_texture.render_rect = {
        available_left + ((content_width - new_width) / 2.0f),
        available_top + ((content_height - new_height) / 2.0f),
        new_width,
        new_height
    };
}


void SDLCALL open_file_callback(void *userdata, const char *const *filelist, int filter)
{
    if (!filelist) {
        printf("No file selected\n");
        return;
    }

    printf("Selected files:\n");

    for (int i = 0; filelist[i]; i++) {
        printf("  %s\n", filelist[i]);
    }
}


std::optional<std::filesystem::path> Frontend::select_file(const std::string& title)
{
    auto result = FileSelectDialog::open(sdl_window, {"tap", "dsk", "rom"});
    return result;
}


void Frontend::close_sound()
{
    if (sound_audio_stream) {
        if (audio_locked) {
            SDL_UnlockAudioStream(sound_audio_stream);
            audio_locked = false;
        }
        SDL_DestroyAudioStream(sound_audio_stream);
        sound_audio_stream = nullptr;
    }
}
