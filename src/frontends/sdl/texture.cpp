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

#include "texture.hpp"
#include "frontends/gui/imgui/backends/imgui_impl_opengl3_loader.h"

Texture::Texture(uint16_t width, uint16_t height, uint8_t bpp) :
    width(width),
    height(height),
    bpp(bpp),
    texture(0),
    render_rect()
{
    render_rect = {0.f, 0.0f, static_cast<float>(width), static_cast<float>(height)};
}

bool Texture::create_texture()
{
    glGenTextures(1, &texture);
    if (texture == 0) {
        return false;
    }

    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);

    return true;
}

void Texture::update_pixels(const uint8_t* pixels) const
{
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::destroy_texture()
{
    if (texture != 0) {
        glDeleteTextures(1, &texture);
        texture = 0;
    }
}

void Texture::set_render_zoom(uint8_t zoom)
{
    render_rect = {0.0f, 0.0f, static_cast<float>(width * zoom), static_cast<float>(height * zoom)};
}
