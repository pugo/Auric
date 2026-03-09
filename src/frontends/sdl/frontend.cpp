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


FileDialogs::FileDialogs(Oric& oric) :
    oric(oric),
    sdl_window(nullptr),
    sdl_renderer(nullptr),
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

FileDialogs::~FileDialogs()
{
    close_graphics();
    close_sdl();
}

bool FileDialogs::init_graphics()
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

    sdl_window = SDL_CreateWindow(window_title.c_str(), width, height, 0);
    if (sdl_window == nullptr) {
        BOOST_LOG_TRIVIAL(error) << "Window could not be created! SDL_Error: " << SDL_GetError();
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

    // Create renderer for window
    sdl_renderer = SDL_CreateRenderer(sdl_window, nullptr);

    if (sdl_renderer == nullptr) {
        BOOST_LOG_TRIVIAL(error) << "Renderer could not be created! SDL Error: " << SDL_GetError();
        return false;
    }

    if (! oric_texture.create_texture(sdl_renderer)) {
        return false;
    }

    // Initialize renderer color
    SDL_SetRenderDrawColor(sdl_renderer, 0x00, 0x00, 0x00, 0xff);
    SDL_RenderClear(sdl_renderer);

    // Initialize Dear Imgui GUI.
    gui.init(sdl_window, sdl_renderer);

    return true;
}


bool FileDialogs::init_sound()
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


void FileDialogs::pause_sound(bool pause_on)
{
    if (pause_on) {
        SDL_PauseAudioStreamDevice(sound_audio_stream);
    } else {
        SDL_ResumeAudioStreamDevice(sound_audio_stream);
    }
}


bool FileDialogs::handle_frame()
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
                    oric.do_quit();
                    return false;
                break;
            }
        }
    }
    return true;
}


void FileDialogs::render_graphics(std::vector<uint8_t>& pixels)
{
    SDL_SetRenderDrawColor(sdl_renderer, 0x00, 0x00, 0x00, 0xff);
    SDL_RenderClear(sdl_renderer);

    SDL_UpdateTexture(oric_texture.texture, nullptr, &pixels[0], oric_texture.width * oric_texture.bpp);
    SDL_RenderTexture(sdl_renderer, oric_texture.texture, nullptr, &oric_texture.render_rect );
    gui.render();

    SDL_RenderPresent(sdl_renderer);
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


std::optional<std::filesystem::path> FileDialogs::select_file(const std::string& title)
{
    auto result = FileSelectDialog::open(sdl_window, {"tap", "dsk", "rom"});
    return result;
}


void FileDialogs::close_sound() const
{
    if (sound_audio_stream) {
        SDL_DestroyAudioStream(sound_audio_stream);
    }
}


void FileDialogs::close_graphics()
{
    gui.close();

    if (sdl_renderer != nullptr) {
        SDL_DestroyRenderer(sdl_renderer);
        sdl_renderer = nullptr;
    }

    if (sdl_window != nullptr) {
        SDL_DestroyWindow(sdl_window);
        sdl_window = nullptr;
    }
}


void FileDialogs::close_sdl()
{
    SDL_Quit(); // Quit all SDL subsystems
}

