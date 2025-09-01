// =========================================================================
//   Copyright (C) 2009-2024 by Anders Piniesjö <pugo@pugo.org>
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
#include <SDL_image.h>

#include "frontend.hpp"
#include "chip/ay3_8912.hpp"
#include "oric.hpp"


static int32_t keytab[] = {
    '7'        , 'n'        , '5'        , 'v'        , 0 ,          '1'        , 'x'        , '3'        ,     // 7
    'j'        , 't'        , 'r'        , 'f'        , 0          , SDLK_ESCAPE, 'q'        , 'd'        ,     // 15
    'm'        , '6'        , 'b'        , '4'        , SDLK_LCTRL , 'z'        , '2'        , 'c'        ,     // 23
    'k'        , '9'        , ';'        , '-'        , 0          , 0          , '\\'       , '\''       ,     // 31
    SDLK_SPACE , ','        , '.'        , SDLK_UP    , SDLK_LSHIFT, SDLK_LEFT  , SDLK_DOWN  , SDLK_RIGHT ,     //
    'u'        , 'i'        , 'o'        , 'p'        , SDLK_LALT  , SDLK_BACKSPACE, ']'     , '['        ,
    'y'        , 'h'        , 'g'        , 'e'        , 0          , 'a'        , 's'        , 'w'        ,
    '8'        , 'l'        , '0'        , '/'        , SDLK_RSHIFT, SDLK_RETURN, 0          , SDLK_EQUALS };


// ----- Frontend ----------------

Frontend::Frontend(Oric* oric) :
    oric(oric),
    sdl_window(NULL),
    sdl_renderer(NULL),
    oric_texture(texture_width, texture_height, texture_bpp),
    status_bar(texture_width*3, 16, texture_bpp)
{
    for (uint8_t i=0; i < 64; ++i) {
        if (keytab[i] != 0) {
            key_map[keytab[i]] = i;
        }
    }

    boost::assign::insert(key_translations)
        (std::make_pair(0xe4, false), std::make_pair('/', false))
        (std::make_pair(0xe4, true), std::make_pair('/', false))
        (std::make_pair(0xf6, false), std::make_pair(';', false))
        (std::make_pair(0xf6, true), std::make_pair(';', false))
        (std::make_pair(0x2b, false), std::make_pair('=', false))
        (std::make_pair(0x2b, true), std::make_pair('=', false));
}

Frontend::~Frontend()
{
    close_graphics();
    close_sound();
    close_sdl();
}

bool Frontend::init_graphics()
{
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cout << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        fprintf(stderr, "could not initialize sdl2_image: %s\n", IMG_GetError());
        return false;
    }

    // Set texture filtering to linear
    if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1")) {
        std::cout <<  "Warning: Linear texture filtering not enabled!" << std::endl;
    }

    oric_texture.set_render_zoom(3);
    status_bar.set_render_zoom(1);
    status_bar.render_rect.y = oric_texture.render_rect.h;

    uint16_t height = oric_texture.render_rect.h + status_bar.render_rect.h;

    sdl_window = SDL_CreateWindow("Pugo-Oric", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                  oric_texture.render_rect.w, height, SDL_WINDOW_SHOWN);
    if (sdl_window == NULL) {
        std::cout << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    // Try to load window icon.
    SDL_Surface* icon = IMG_Load("images/window_icon.png");
    if (icon) {
        SDL_SetWindowIcon(sdl_window, icon);
    }

    // Create renderer for window
    sdl_renderer = SDL_CreateRenderer(sdl_window, -1, SDL_RENDERER_ACCELERATED);

    if (sdl_renderer == NULL) {
        std::cout <<  "Renderer could not be created! SDL Error: " << SDL_GetError() << std::endl;
        return false;
    }

    if (! oric_texture.create_texture(sdl_renderer) ||
        ! status_bar.init(sdl_renderer)) {
        return false;
    }

    // Initialize renderer color
    SDL_SetRenderDrawColor(sdl_renderer, 0xff, 0xff, 0xff, 0xff);
    SDL_RenderClear(sdl_renderer);

    return true;
}


bool Frontend::init_sound()
{
    std::cout << "Initializing sound.." << std::endl;

    if (SDL_Init(SDL_INIT_AUDIO) < 0)
    {
        std::cout << "Error: failed initializing SDL: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_AudioSpec audio_spec_want, audio_spec;
    SDL_memset(&audio_spec_want, 0, sizeof(audio_spec_want));

    audio_spec_want.freq     = 44100;
    audio_spec_want.format   = AUDIO_S16SYS;
    audio_spec_want.channels = 2;
    audio_spec_want.samples  = 2048;
    AY3_8912* ay3 = oric->get_machine().ay3.get();
    audio_spec_want.callback = ay3->audio_callback;
    audio_spec_want.userdata = (void*) ay3;

    sound_audio_device_id = SDL_OpenAudioDevice(NULL, 0, &audio_spec_want, &audio_spec, 0);

    if (!sound_audio_device_id)
    {
        std::cout << "Error: creating SDL audio device: " << SDL_GetError() << std::endl;
        return false;
    }

    if (audio_spec_want.format != audio_spec.format) {
        SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "Failed to get the desired AudioSpec");
    }

    std::cout << "Freq: " << std::dec << (int) audio_spec.freq << std::endl;
    std::cout << "Silence: " << (int) audio_spec.silence << std::endl;
    std::cout << "format: " << (int) audio_spec.format << std::endl;
    std::cout << "channels: " << (int) audio_spec.channels << std::endl;
    std::cout << "samples: " << (int) audio_spec.samples << std::endl;

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
                auto sym = event.key.keysym.sym;

                if (event.type == SDL_KEYDOWN) {
                    if (event.key.keysym.sym == SDLK_F12) {
                        oric->get_machine().toggle_warp_mode();
                    }

                    else if (event.key.keysym.sym == SDLK_F10) {
                        oric->get_machine().cpu->NMI();
                    }

                    else if (event.key.keysym.sym == SDLK_F1) {
                        oric->get_machine().save_snapshot();
                    }

                    else if (event.key.keysym.sym == SDLK_F2) {
                        oric->get_machine().load_snapshot();
                    }

                    else if (event.key.keysym.sym == SDLK_b && event.key.keysym.mod & KMOD_CTRL) {
                        oric->get_machine().stop();
                        oric->do_break();
                    }
                }

                auto trans = key_translations.find(std::make_pair(sym, event.key.keysym.mod));
                if (trans != key_translations.end()) {
                    sym = trans->second.first;
                }

                auto key = key_map.find(sym);
                if (key != key_map.end()) {
                    oric->get_machine().key_press(key->second, event.type == SDL_KEYDOWN);
                }
                break;
            }

            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
                    oric->do_quit();
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

    SDL_UpdateTexture(oric_texture.texture, NULL, &pixels[0], oric_texture.width * oric_texture.bpp);
    SDL_RenderCopy(sdl_renderer, oric_texture.texture, NULL, &oric_texture.render_rect );
    SDL_RenderCopy(sdl_renderer, status_bar.texture, NULL, &status_bar.render_rect);
    SDL_RenderPresent(sdl_renderer);
}


void Frontend::close_graphics()
{
    SDL_DestroyRenderer(sdl_renderer);
    SDL_DestroyWindow(sdl_window);
    sdl_window = NULL;
}


void Frontend::close_sound()
{
    SDL_PauseAudioDevice(sound_audio_device_id, true);
    SDL_CloseAudioDevice(sound_audio_device_id);
}


void Frontend::close_sdl()
{
    SDL_Quit(); // Quit all SDL subsystems
}

