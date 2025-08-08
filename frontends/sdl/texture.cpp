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


Texture::Texture(uint16_t width, uint16_t height, uint8_t bpp) :
    width(width), height(height), bpp(bpp)
{
    render_rect = {0, 0, width, height};
}

bool Texture::create_texture(SDL_Renderer* sdl_renderer)
{
    texture = SDL_CreateTexture(sdl_renderer,
                                SDL_PIXELFORMAT_ARGB8888,
                                SDL_TEXTUREACCESS_STREAMING,
                                width, height);
    return texture != NULL;
}

void Texture::set_render_zoom(uint8_t zoom)
{
    render_rect = {0, 0, width * zoom, height * zoom};
}

