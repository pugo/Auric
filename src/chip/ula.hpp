// =========================================================================
//   Copyright (C) 2009-2025 by Anders Piniesjö <pugo@pugo.org>
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

#ifndef ULA_H
#define ULA_H

#include "frontends/sdl/frontend.hpp"


class ULA
{
public:
    /**
     * Oric color palette from color index.
     */
    uint32_t colors[8] {0xff000000, 0xffff0000, 0xff00ff00, 0xffffff00, 0xff0000ff, 0xffff00ff, 0xff00ffff, 0xffffffff};

    enum VideoAttribs
    {
        HZ_50 = 0x02,
        HIRES = 0x04
    };

    enum TextAttribs
    {
        ALTERNATE_CHARSET = 0x01,
        DOUBLE_SIZE = 0x02,
        BLINKING = 0x04,
    };

    ULA(Machine& machine, Memory& memory, uint8_t texture_width, uint8_t texture_height, uint8_t texture_bpp);

    /**
     * Paint one raster line.
     * @return true if screen is finished and should be rendered.
     */
    bool paint_raster();

private:
    /**
     * Update graphics for given raster line.
     * @param raster_line raster line to update
     */
    void update_graphics(uint8_t raster_line);

    Machine& machine;
    Memory& memory;

    uint8_t texture_width;
    uint8_t texture_height;
    uint8_t texture_bpp;

    uint8_t video_attrib;
    uint8_t text_attrib;

    uint16_t raster_current;
    uint8_t warpmode_counter;

    uint8_t blink;
    uint32_t frame_count;

    std::vector<uint8_t> pixels;

    // Lookup table for masks corresponding to all bit combinations of character bits.
    // 6 pixels per char pack 3 words, each word = 2 pixels (2 * 32-bit).
    uint64_t char_mask[64][3];
};


#endif // ULA_H
