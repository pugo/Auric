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

#ifndef FRONTENDS_SDL_STATUSBAR_H
#define FRONTENDS_SDL_STATUSBAR_H

#include <memory>
#include <iostream>
#include <ostream>
#include <thread>
#include <vector>
#include <SDL.h>
#include <SDL_audio.h>
#include <SDL_ttf.h>


class StatusBar : public Texture
{
public:
    StatusBar(uint16_t width, uint16_t height, uint8_t bpp);
    ~StatusBar();

    bool init(SDL_Renderer* sdl_renderer);

    void paint();
    bool update_texture(SDL_Renderer* sdl_renderer);
    bool has_update() { return has_updated; }

    void set_text(const std::string& text);

private:
    void put_char(uint8_t pos, uint8_t chr);
    void thread_main();
    void stop_thread();

    bool do_stop_thread;
    bool update_requested;
    bool has_updated;
    std::mutex update_mutex;
    std::thread update_thread;
    std::condition_variable condition_variable;

    SDL_Surface* front_surface;
    SDL_Surface* back_surface;
    std::string text;

    uint32_t font_size;
    std::unique_ptr<uint8_t> font_data;
};


#endif // FRONTENDS_SDL_STATUSBAR_H
