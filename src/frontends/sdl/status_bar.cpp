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

#include <filesystem>

#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <condition_variable>

#include <SDL_image.h>

#include "frontend.hpp"


std::filesystem::path font_path = "fonts/light.bin";

const uint8_t ascii_to_glyph[128] = {
    // Control characters 0–31 (map to themselves or custom handling)
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,

    // Printable ASCII 32–63 (space to '?')
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,  //  !"#$%&'
    0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0xff,  // ()*+,-./
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,  // 01234567
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,  // 89:;<=>?

    // ASCII 64–95 ('@' to '_') — same in PETSCII
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,  // @ABCDEFG
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,  // HIJKLMNO
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,  // PQRSTUVW
    0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x64,  // XYZ[\]^_

    // ASCII 96–127 ('`' to DEL)
    0x27, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,  // `abcdefg
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,  // hijklmno
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,  // pqrstuvw
    0x18, 0x19, 0x1a, 0xff, 0xff, 0xff, 0xff, 0xff   // xyz{|}~
};

constexpr uint8_t margin_x = 8;
constexpr uint8_t margin_y = 3;

constexpr uint8_t font_width = 8;
constexpr uint8_t font_height = 8;
constexpr uint8_t bpp = 4;


StatusBar::StatusBar(uint16_t width, uint16_t height, uint8_t bpp) :
    Texture(width, height, bpp),
    active_flags(0),
    do_stop_thread(false),
    update_requested(false),
    has_updated(false),
    update_thread(),
    front_surface(nullptr),
    back_surface(nullptr),
    font_size(0),
    font_data(nullptr)
{}


StatusBar::~StatusBar()
{
    stop_thread();

    if (front_surface) {
        SDL_FreeSurface(front_surface);
    }

    if (back_surface) {
        SDL_FreeSurface(back_surface);
    }
}


bool StatusBar::init(SDL_Renderer* sdl_renderer)
{
    std::cout << "Status bar: Reading font: '" << font_path << "'" << std::endl;

    std::ifstream file (font_path, std::ios::in | std::ios::binary | std::ios::ate);
    if (file.is_open())
    {
        file.seekg (0, file.end);
        font_size = file.tellg();
        font_data = std::unique_ptr<uint8_t>(new uint8_t[font_size]);
        file.seekg(0, std::ios::beg);
        file.read(reinterpret_cast<char*>(font_data.get()), font_size);
        file.close();
    }
    else {
        std::cout << "Status bar: unable to open font file";
        return false;
    }

    front_surface = SDL_CreateRGBSurfaceWithFormat(0xff, width, height, 32, SDL_PIXELFORMAT_RGBA32);
    back_surface = SDL_CreateRGBSurfaceWithFormat(0xff, width, height, 32, SDL_PIXELFORMAT_RGBA32);
    if (front_surface == nullptr || back_surface == nullptr) {
        return false;
    }

    update_thread = std::thread(&StatusBar::thread_main, this);

    paint();
    return update_texture(sdl_renderer);
}


void StatusBar::set_text(const std::string& text)
{
    bool update = this->text != text;
    this->text = text;

    if (update) {
        paint();
    }
}


void StatusBar::show_text_for(const std::string& text, std::chrono::milliseconds duration)
{
    std::lock_guard<std::mutex> guard(update_mutex);
    this->text = text;
    clear_at = std::chrono::steady_clock::now() + duration;

    update_requested = true;
    condition_variable.notify_one();
}


void StatusBar::set_flag(uint16_t flag, bool on)
{
    uint16_t old_flags = active_flags;

    active_flags += on ? flag : -flag;

    if (active_flags != old_flags) {
        paint();
    }
}


void StatusBar::paint()
{
    std::lock_guard<std::mutex> guard(update_mutex);
    update_requested = true;
    condition_variable.notify_one();
}


bool StatusBar::update_texture(SDL_Renderer* sdl_renderer)
{
    has_updated = false;
    texture = SDL_CreateTextureFromSurface(sdl_renderer, front_surface);
    return texture != nullptr;
}


void StatusBar::paint_text()
{
    auto pos = 0;
    for (auto c : text) {
        if (c > 127) {
            continue;
        }
        put_char(pos++, ascii_to_glyph[c]);
    }
}


void StatusBar::paint_flags()
{
    std::string flags_string{""};

    if (active_flags & StatusbarFlags::loading) {
        flags_string.append("[Tape]");
    }
    if (active_flags & StatusbarFlags::warp_mode) {
        flags_string.append("[Warp]");
    }

    uint16_t pos = width/font_width - flags_string.length() - 2;
    for (auto c : flags_string) {
        if (c > 127) {
            continue;
        }
        put_char(pos++, ascii_to_glyph[c]);
    }
}


void StatusBar::put_char(uint8_t pos, uint8_t chr)
{
    if (font_height * chr > font_size) {
        return;
    }

    uint8_t* chr_ptr = font_data.get() + (font_height * chr);

    for (auto y = 0; y < font_height; ++y) {
        for (auto x = 0; x < font_width; ++x) {
            if (*chr_ptr & (1 << x)) {
                uint32_t* pixel = (uint32_t*)((uint8_t*)back_surface->pixels +
                                              (margin_y + y) * back_surface->pitch +
                                              (margin_x + (pos * font_width) + font_width - x) * bpp);

                *pixel = 0xffffffff;
            }
        }
        ++chr_ptr;
    }
}


void StatusBar::thread_main()
{
    std::unique_lock<std::mutex> lock(update_mutex);

    while(! do_stop_thread) {
        auto deadline = clear_at ? *clear_at : std::chrono::steady_clock::time_point::max();
        bool woke = condition_variable.wait_until(lock, deadline, [&] { return update_requested || do_stop_thread; });

        if (do_stop_thread) {
            break;
        }

        if (!woke) {
            if (clear_at && std::chrono::steady_clock::now() >= *clear_at) {
                text.clear();
                clear_at.reset();
                update_requested = true;
            }
        }

        if (update_requested) {
            SDL_FillRect(back_surface, NULL, 0x000000ff);
            update_requested = false;
            lock.unlock();

            paint_text();
            paint_flags();

            lock.lock();
            std::swap(front_surface, back_surface);
            has_updated = true;
        }
    }
}


void StatusBar::stop_thread()
{
    {
        std::lock_guard<std::mutex> guard(update_mutex);
        do_stop_thread = true;
    }

    condition_variable.notify_all();
    if (update_thread.joinable()) update_thread.join();
}
