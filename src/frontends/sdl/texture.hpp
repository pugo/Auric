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

#ifndef FRONTENDS_SDL_TEXTURE_H
#define FRONTENDS_SDL_TEXTURE_H

#include <SDL.h>


class Texture
{
public:
    Texture(uint16_t width, uint16_t height, uint8_t bpp);

    bool create_texture(SDL_Renderer* sdl_renderer);
    void set_render_zoom(uint8_t zoom);

    const uint16_t width;
    const uint16_t height;
    const uint8_t bpp;

    SDL_Texture* texture;
    SDL_Rect render_rect;
};

#endif // FRONTENDS_SDL_TEXTURE_H
