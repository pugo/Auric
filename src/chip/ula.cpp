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

#include <vector>
#include <stack>
#include <utility>

#include <boost/assign.hpp>

#include <machine.hpp>
#include "ula.hpp"

constexpr uint16_t raster_max = 312;

constexpr uint16_t raster_visible_lines = 224;
//constexpr uint16_t raster_visible_first = 65;
constexpr uint16_t raster_visible_first = 44;
constexpr uint16_t raster_visible_last = raster_visible_first + raster_visible_lines;


ULA::ULA(Machine& machine, Memory& memory, uint8_t texture_width, uint8_t texture_height, uint8_t texture_bpp) :
    machine(machine),
    memory(memory),
    texture_width(texture_width),
    texture_height(texture_height),
    texture_bpp(texture_bpp),
    raster_current(0),
    video_attrib(0),
    text_attrib(0),
    warpmode_counter(0),
    blink(0x3f),
    frame_count(0)
{
    pixels = std::vector<uint8_t>(texture_width * texture_height * texture_bpp, 0);
}


bool ULA::paint_raster()
{
    bool render_screen = false;

    if ((raster_current >= raster_visible_first) && (raster_current <= raster_visible_last)) {
        update_graphics(raster_current - raster_visible_first);
    }

    if (++raster_current == raster_max) {
        raster_current = 0;
        if (machine.warpmode_on) {
            warpmode_counter = (warpmode_counter + 1) % 25;
            if (warpmode_counter) {
                return false;
            }
        }

        render_screen = true;
        machine.frontend->render_graphics(pixels);
        frame_count++;
    }

    return render_screen;
}


// Return memory address corresponding to a raster line, for current video mode.
inline uint16_t calcRowAddr(uint8_t raster_line, uint8_t video_attrib)
{
    if (video_attrib & ULA::VideoAttribs::HIRES && raster_line < 200) {
        return 0xa000 + raster_line * 40;			// Hires: return address for hires data for line.
    }
    return 0xbb80 + (raster_line >> 3) * 40;		// Text (lores or > 200): return address to char data for line (>>3).
}


void ULA::update_graphics(uint8_t raster_line)
{
    uint32_t bg_col = colors[0];
    uint32_t fg_col = colors[7];
    text_attrib = 0;
    blink = 0x3f;

    uint32_t* texture_line = (uint32_t*) &pixels[raster_line * Frontend::texture_width * Frontend::texture_bpp];
    uint16_t row = calcRowAddr(raster_line, video_attrib);

    // 40 characters wide, regardless of lores or hires.
    for (uint16_t x = 0; x < 40; x++) {
        bool ctrl_char = false;

        // get char code.
        uint8_t ch = memory.mem[row + x];

        if (!(ch & 0x60)) {
            ctrl_char = true;
            switch(ch & 0x18)
            {
                case 0x00:
                    // Ink color.
                    fg_col = colors[ch & 7];
                    break;
                case 0x08:
                    // Text attributes.
                    text_attrib = ch & 7;
                    blink = ch & 0x04 ? 0x00 : 0x3f;
                    break;
                case 0x10:
                    // Paper color.
                    bg_col = colors[ch & 7];
                    break;
                case 0x18:
                    // Video control attrs.
                    video_attrib = ch & 0x07;
                    row = calcRowAddr(raster_line, video_attrib);
                    break;
            }
        }

        uint32_t _fg_col = fg_col;
        uint32_t _bg_col = bg_col;

        uint8_t mask = frame_count & 0x10 ? 0x3f : blink;

        // Inverse colors if upper bit is set in char code.
        if (ch & 0x80) {
            _fg_col = _fg_col ^ 0x00ffffff;
            _bg_col = _bg_col ^ 0x00ffffff;
        }

        uint8_t chr_dat = 0;
        if (!ctrl_char) {
            if ((video_attrib & VideoAttribs::HIRES) && raster_line < 200) {
                chr_dat = ch & mask;	// Hires, read byte directly.
            }
            else {
                // get char pixel data for read char code. If hires > 200, charmem is at 0x9800.
                uint8_t* char_mem = memory.mem + ((video_attrib & VideoAttribs::HIRES) ? 0x9800 : 0xb400) +
                                    ((text_attrib & TextAttribs::ALTERNATE_CHARSET) ? 128 * 8 : 0);

                uint8_t apan = (text_attrib & TextAttribs::DOUBLE_SIZE) ? ((raster_line >> 1) & 0x07) : (raster_line & 0x07);
                chr_dat = char_mem[((ch & 0x7f) << 3) + apan] & mask;
            }
        }

        // Copy 6 pixels.
        *texture_line++ = (chr_dat & 0x20) ? _fg_col : _bg_col;
        *texture_line++ = (chr_dat & 0x10) ? _fg_col : _bg_col;
        *texture_line++ = (chr_dat & 0x08) ? _fg_col : _bg_col;
        *texture_line++ = (chr_dat & 0x04) ? _fg_col : _bg_col;
        *texture_line++ = (chr_dat & 0x02) ? _fg_col : _bg_col;
        *texture_line++ = (chr_dat & 0x01) ? _fg_col : _bg_col;
    }
}


