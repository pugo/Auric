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

#include <boost/assign.hpp>
#include <boost/log/trivial.hpp>
#include <unordered_map>

#include <SDL_hints.h>
#include <SDL_image.h>
#include <SDL_scancode.h>

#include "frontend.hpp"
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

constexpr uint16_t border_size_horizontal = 100;
constexpr uint16_t border_size_vertical = 50;


Frontend::Frontend(Oric& oric) :
    oric(oric),
    sdl_window(nullptr),
    sdl_renderer(nullptr),
    oric_texture(texture_width, texture_height, texture_bpp),
    status_bar(texture_width * oric.get_config().zoom(), 16, texture_bpp),
    sound_audio_device_id(),
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
    SDL_SetHint(SDL_HINT_APP_NAME, "Auric");

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        BOOST_LOG_TRIVIAL(error) << "SDL could not initialize! SDL_Error: " << SDL_GetError();
        return false;
    }

    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        BOOST_LOG_TRIVIAL(error) << "Could not initialize sdl2_image: " << IMG_GetError();
        return false;
    }

    // Set texture filtering to linear
    if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1")) {
        BOOST_LOG_TRIVIAL(warning) << "Linear texture filtering not enabled!";
    }

    auto zoom = oric.get_config().zoom();
    BOOST_LOG_TRIVIAL(debug) << "Setting zoom to: " << static_cast<int>(zoom);

    oric_texture.set_render_zoom(zoom);
    status_bar.set_render_zoom(1);

    oric_texture.render_rect.x = border_size_horizontal;
    oric_texture.render_rect.y = border_size_vertical;

    uint16_t width = oric_texture.render_rect.w + (border_size_horizontal * 2);
    uint16_t height = oric_texture.render_rect.h + status_bar.render_rect.h + (border_size_vertical * 2);

    status_bar.render_rect.x = border_size_horizontal;
    status_bar.render_rect.y = height - status_bar.render_rect.h;

    sdl_window = SDL_CreateWindow("Auric", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                  width, height, SDL_WINDOW_SHOWN);
    if (sdl_window == nullptr) {
        BOOST_LOG_TRIVIAL(error) << "Window could not be created! SDL_Error: " << SDL_GetError();
        return false;
    }

    // Try to load a window icon.
    SDL_Surface* icon = IMG_Load("images/window_icon.png");
    if (icon) {
        SDL_SetWindowIcon(sdl_window, icon);
    }

    // Create renderer for window
    sdl_renderer = SDL_CreateRenderer(sdl_window, -1, SDL_RENDERER_ACCELERATED);

    if (sdl_renderer == nullptr) {
        BOOST_LOG_TRIVIAL(error) << "Renderer could not be created! SDL Error: " << SDL_GetError();
        return false;
    }

    if (! oric_texture.create_texture(sdl_renderer) ||
        ! status_bar.init(sdl_renderer)) {
        return false;
    }

    // Initialize renderer color
    SDL_SetRenderDrawColor(sdl_renderer, 0x00, 0x00, 0x00, 0xff);
    SDL_RenderClear(sdl_renderer);

    return true;
}


bool Frontend::init_sound()
{
    BOOST_LOG_TRIVIAL(debug) << "Initializing sound..";

    if (SDL_Init(SDL_INIT_AUDIO) < 0)
    {
        BOOST_LOG_TRIVIAL(error) << "Error: failed initializing SDL: " << SDL_GetError();
        return false;
    }

    SDL_AudioSpec audio_spec_want, audio_spec;
    SDL_memset(&audio_spec_want, 0, sizeof(audio_spec_want));

    audio_spec_want.freq     = 44100;
    audio_spec_want.format   = AUDIO_S16SYS;
    audio_spec_want.channels = 2;
    audio_spec_want.samples  = 2048;
    AY3_8912* ay3 = oric.get_machine().ay3.get();
    audio_spec_want.callback = AY3_8912::audio_callback;
    audio_spec_want.userdata = (void*) ay3;

    sound_audio_device_id = SDL_OpenAudioDevice(nullptr, 0, &audio_spec_want, &audio_spec, 0);

    if (!sound_audio_device_id)
    {
        BOOST_LOG_TRIVIAL(error) << "Error: creating SDL audio device: " << SDL_GetError();
        return false;
    }

    if (audio_spec_want.format != audio_spec.format) {
        BOOST_LOG_TRIVIAL(error) << "Failed to get the desired AudioSpec";
    }

    BOOST_LOG_TRIVIAL(debug) << "Freq: " << std::dec << (int) audio_spec.freq;
    BOOST_LOG_TRIVIAL(debug) << "Silence: " << (int) audio_spec.silence;
    BOOST_LOG_TRIVIAL(debug) << "format: " << (int) audio_spec.format;
    BOOST_LOG_TRIVIAL(debug) << "channels: " << (int) audio_spec.channels;
    BOOST_LOG_TRIVIAL(debug) << "samples: " << (int) audio_spec.samples;

    return true;
}


void Frontend::pause_sound(bool pause_on)
{
    SDL_PauseAudioDevice(sound_audio_device_id, pause_on ? 1 : 0);
}


bool Frontend::handle_frame()
{
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type)
        {
            case SDL_KEYDOWN:
            case SDL_KEYUP:
            {
                auto scancode = event.key.keysym.scancode;
                bool special_pressed{false};

                if (event.type == SDL_KEYDOWN) {
                    if (event.key.keysym.mod & KMOD_CTRL) {
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
                        if (scancode == SDL_SCANCODE_F1) {
                            oric.get_machine().save_snapshot();
                            special_pressed = true;
                        }

                        else if (scancode == SDL_SCANCODE_F2) {
                            oric.get_machine().load_snapshot();
                            special_pressed = true;
                        }
                    }
                }

                if (! special_pressed) {
                    if (!special_pressed) {
                        auto key = oric_key_map.find(scancode);
                        if (key != oric_key_map.end()) {
                            oric.get_machine().key_press(key->second, event.type == SDL_KEYDOWN);
                        }
                    }
                }
                break;
            }

            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
                    oric.do_quit();
                    return false;
                }
        }
    }
    return true;
}


void Frontend::render_graphics(std::vector<uint8_t>& pixels)
{
    if (status_bar.has_update()) {
        status_bar.update_texture(sdl_renderer);
    }

    SDL_UpdateTexture(oric_texture.texture, nullptr, &pixels[0], oric_texture.width * oric_texture.bpp);
    SDL_RenderCopy(sdl_renderer, oric_texture.texture, nullptr, &oric_texture.render_rect );
    SDL_RenderCopy(sdl_renderer, status_bar.texture, nullptr, &status_bar.render_rect);
    SDL_RenderPresent(sdl_renderer);
}


void Frontend::close_graphics()
{
    if (sdl_renderer != nullptr) {
        SDL_DestroyRenderer(sdl_renderer);
        sdl_renderer = nullptr;
    }

    if (sdl_window != nullptr) {
        SDL_DestroyWindow(sdl_window);
        sdl_window = nullptr;
    }
}


void Frontend::close_sound() const
{
    SDL_PauseAudioDevice(sound_audio_device_id, true);
    SDL_CloseAudioDevice(sound_audio_device_id);
}


void Frontend::close_sdl()
{
    SDL_Quit(); // Quit all SDL subsystems
}

