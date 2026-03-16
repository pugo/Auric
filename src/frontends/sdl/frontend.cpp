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

#include <boost/log/trivial.hpp>
#include <unordered_map>

#include <SDL3/SDL.h>

#include "file_dialog.hpp"
#include "frontend.hpp"
#include "frontends/gui/status_bar.hpp"
#include "frontends/gui/imgui/backends/imgui_impl_opengl3_loader.h"

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
constexpr uint16_t border_size_horizontal = 100;
constexpr uint16_t border_size_vertical = 50;

constexpr std::string window_title = "Auric";
constexpr std::string window_icon_name = "window_icon.png";

namespace
{
GLuint compile_shader(GLenum type, const char* source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE) {
        GLchar log[512];
        glGetShaderInfoLog(shader, static_cast<GLsizei>(sizeof(log)), nullptr, log);
        BOOST_LOG_TRIVIAL(error) << "OpenGL shader compilation failed: " << log;
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

GLuint create_screen_program()
{
    constexpr const char* vertex_shader = R"(
        #version 150
        in vec2 a_pos;
        in vec2 a_uv;
        out vec2 v_uv;
        void main()
        {
            v_uv = a_uv;
            gl_Position = vec4(a_pos, 0.0, 1.0);
        }
    )";

    constexpr const char* fragment_shader = R"(
        #version 150
        in vec2 v_uv;
        out vec4 out_color;
        uniform sampler2D u_texture;
        uniform int u_enable_scanlines;
        uniform int u_enable_vertical_lines;
        uniform float u_vignette_strength;

        float hash(vec2 p)
        {
            return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
        }

        void main()
        {
            vec2 uv = v_uv;

            // One screen pixel in UV space at current output resolution.
            vec2 texel = vec2(abs(dFdx(v_uv.x)), abs(dFdy(v_uv.y)));
            vec4 c0 = texture(u_texture, uv).bgra;
            vec4 c1 = texture(u_texture, clamp(uv + vec2(texel.x, 0.0), 0.0, 1.0)).bgra;
            vec4 c2 = texture(u_texture, clamp(uv - vec2(texel.x, 0.0), 0.0, 1.0)).bgra;
            vec4 c3 = texture(u_texture, clamp(uv + vec2(0.0, texel.y), 0.0, 1.0)).bgra;
            vec4 c4 = texture(u_texture, clamp(uv - vec2(0.0, texel.y), 0.0, 1.0)).bgra;
            vec4 color = c0 * 0.50 + (c1 + c2) * 0.20 + (c3 + c4) * 0.05;

            float scanline = 0.94 + 0.06 * sin(gl_FragCoord.y/2 * 3.14159265);
            float mask = 0.96 + 0.04 * sin(gl_FragCoord.x * 0.5);
            vec2 centered = abs(v_uv * 2.0 - 1.0);
            float edge = max(centered.x * 0.85, centered.y);
            float vignette = 1.0 - u_vignette_strength * pow(edge, 2.2);
            float noise = (hash(gl_FragCoord.xy) - 0.5) * 0.02;

            if (u_enable_scanlines != 0) {
                color.rgb *= scanline;
            }

            if (u_enable_vertical_lines != 0) {
                color.rgb *= mask;
            }

            color.rgb *= vignette;
            color.rgb += noise;
            out_color = vec4(clamp(color.rgb, 0.0, 1.0), 1.0);
        }
    )";

    GLuint vs = compile_shader(GL_VERTEX_SHADER, vertex_shader);
    if (vs == 0) {
        return 0;
    }
    GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fragment_shader);
    if (fs == 0) {
        glDeleteShader(vs);
        return 0;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint success = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (success == GL_FALSE) {
        GLchar log[512];
        glGetProgramInfoLog(program, static_cast<GLsizei>(sizeof(log)), nullptr, log);
        BOOST_LOG_TRIVIAL(error) << "OpenGL program link failed: " << log;
        glDeleteProgram(program);
        return 0;
    }

    return program;
}
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
    audio_locked(false)
{
    for (uint8_t i = 0; i < 64; ++i) {
        if (scancode_map[i] != 0) {
            oric_key_map[scancode_map[i]] = i;
        }
    }
}

Frontend::~Frontend()
{
    close_graphics();
    close_sdl();
}

bool Frontend::init_graphics()
{
    SDL_SetHint(SDL_HINT_APP_NAME, window_title.c_str());

    // Initialize SDL
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        BOOST_LOG_TRIVIAL(error) << "SDL could not initialize! SDL_Error: " << SDL_GetError();
        return false;
    }

    // Set texture filtering to linear
    // if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear")) {
    //     BOOST_LOG_TRIVIAL(warning) << "Linear texture filtering not enabled!";
    // }

    auto zoom = oric.get_config().zoom();
    BOOST_LOG_TRIVIAL(debug) << "Setting zoom to: " << static_cast<int>(zoom);

    oric_texture.set_render_zoom(zoom);

    oric_texture.render_rect.x = border_size_horizontal;
    oric_texture.render_rect.y = border_size_vertical;

    uint16_t width = oric_texture.render_rect.w + (border_size_horizontal * 2);
    uint16_t height = oric_texture.render_rect.h + status_bar_height + (border_size_vertical * 2);

    gui.status_bar().set_size(width, status_bar_height);
    gui.status_bar().set_position(0, height - status_bar_height);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    sdl_window = SDL_CreateWindow(window_title.c_str(), width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (sdl_window == nullptr) {
        BOOST_LOG_TRIVIAL(error) << "Window could not be created! SDL_Error: " << SDL_GetError();
        return false;
    }

    gl_context = SDL_GL_CreateContext(sdl_window);
    if (gl_context == nullptr) {
        BOOST_LOG_TRIVIAL(error) << "OpenGL context could not be created! SDL_Error: " << SDL_GetError();
        return false;
    }
    SDL_GL_MakeCurrent(sdl_window, gl_context);
    SDL_GL_SetSwapInterval(1);

    if (imgl3wInit() != GL3W_OK) {
        BOOST_LOG_TRIVIAL(error) << "Failed to initialize OpenGL function loader";
        return false;
    }

    // Try to load a window icon.
    auto path = oric.get_config().images_path() / window_icon_name;
    if (std::filesystem::exists(path)) {
        SDL_Surface* icon = SDL_LoadSurface(path.string().c_str());
        if (! icon) {
            BOOST_LOG_TRIVIAL(error) << "Failed loading application icon: " << path << ": " << SDL_GetError();
        }
        else {
            if (!SDL_SetWindowIcon(sdl_window, icon)) {
                BOOST_LOG_TRIVIAL(error) << "Failed setting application icon";
            }
            SDL_DestroySurface(icon);
        }
    }

    gl_program = create_screen_program();
    if (gl_program == 0) {
        return false;
    }
    const GLint pos_attr = glGetAttribLocation(gl_program, "a_pos");
    const GLint uv_attr = glGetAttribLocation(gl_program, "a_uv");
    if (pos_attr < 0 || uv_attr < 0) {
        BOOST_LOG_TRIVIAL(error) << "Failed to resolve OpenGL shader attributes";
        return false;
    }
    gl_u_texture = glGetUniformLocation(gl_program, "u_texture");
    if (gl_u_texture < 0) {
        BOOST_LOG_TRIVIAL(error) << "Failed to resolve OpenGL shader uniforms";
        return false;
    }

    gl_u_enable_scanlines = glGetUniformLocation(gl_program, "u_enable_scanlines");
    gl_u_enable_vertical_lines = glGetUniformLocation(gl_program, "u_enable_vertical_lines");
    gl_u_vignette_strength = glGetUniformLocation(gl_program, "u_vignette_strength");

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
    gui.init(sdl_window, gl_context);

    return true;
}


bool Frontend::init_sound()
{
    BOOST_LOG_TRIVIAL(debug) << "Initializing sound..";

    if (!SDL_Init(SDL_INIT_AUDIO))
    {
        BOOST_LOG_TRIVIAL(error) << "Error: failed initializing SDL: " << SDL_GetError();
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
        BOOST_LOG_TRIVIAL(error) << "Error: creating SDL audio stream: " << SDL_GetError();
        return false;
    }

    SDL_AudioSpec audio_spec;
    if (SDL_GetAudioStreamFormat(sound_audio_stream, &audio_spec, nullptr)) {
        BOOST_LOG_TRIVIAL(debug) << "Freq: " << std::dec << (int) audio_spec.freq;
        BOOST_LOG_TRIVIAL(debug) << "format: " << (int) audio_spec.format;
        BOOST_LOG_TRIVIAL(debug) << "channels: " << (int) audio_spec.channels;
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
                                oric.get_machine().stop();
                                oric.do_break();
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
    glUniform1i(gl_u_enable_scanlines, 1);
    glUniform1i(gl_u_enable_vertical_lines, 1);
    // glUniform1f(gl_u_vignette_strength, 0.8);
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


void Frontend::close_sound() const
{
    if (sound_audio_stream) {
        SDL_DestroyAudioStream(sound_audio_stream);
    }
}


void Frontend::close_graphics()
{
    if (sdl_window != nullptr && gl_context != nullptr) {
        SDL_GL_MakeCurrent(sdl_window, gl_context);
    }

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

    gui.close();

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
