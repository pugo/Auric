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

#ifndef FRONTENDS_SDL_STATUSBAR_H
#define FRONTENDS_SDL_STATUSBAR_H

#include <condition_variable>
#include <filesystem>
#include <memory>
#include <thread>
#include <SDL.h>
#include <SDL_audio.h>
#include <atomic>
#include <mutex>
#include <optional>

#include "frontends/flags.hpp"

/**
 * Handle status bar.
 */
class StatusBar : public Texture
{
public:
    StatusBar(uint16_t width, uint16_t height, uint8_t bpp);
    ~StatusBar();

    /**
     * Init the status bar.
     * @param sdl_renderer SDL renderer
     * @return true on success
     */
    bool init(SDL_Renderer* sdl_renderer, std::filesystem::path font_path);

    /**
     * Notify status bar that the contents should be repainted.
     */
    void paint();

    /**
     * Update the texture from status bar surface.
     * @param sdl_renderer SDL renderer
     */
    void update_texture(SDL_Renderer* sdl_renderer);

    /**
     * True if a requested update is finished.
     * @return true if update is finished
     */
    [[nodiscard]] bool has_update() const { return has_updated; }

    /**
     * Show given string for a certain duration. Triggers an update.
     * @param text text to show
     * @param duration duration to show text
     */
    void show_text_for(const std::string& text, std::chrono::milliseconds duration);

    /**
     * Set flag to wanted state. Triggers update if flags are changed.
     * @param flag flag to set
     * @param on flag state
     */
    void set_flag(uint16_t flag, bool on);

private:
    /**
     * Paint the current text to the current surface.
     */
    void paint_text();

    /**
     * Paint the current flags to the current surface.
     */
    void paint_flags();

    /**
     * Put a character to specified position.
     * @param pos character position (as in character widths, not pixels)
     * @param chr character to put
     */
    void put_char(uint8_t pos, uint8_t chr);

    /**
     * Thread main function.
     */
    void thread_main();

    /**
     * Signal thread to stop.
     */
    void stop_thread();

    uint16_t active_flags;

    bool do_stop_thread;
    bool update_requested;
    std::atomic<bool> has_updated;
    std::mutex update_mutex;
    std::thread update_thread;

    std::condition_variable condition_variable;

    SDL_Surface* front_surface;
    SDL_Surface* back_surface;

    std::string text;
    std::optional<std::chrono::steady_clock::time_point> clear_at;

    uint32_t font_size;
    std::unique_ptr<uint8_t> font_data;
};


#endif // FRONTENDS_SDL_STATUSBAR_H
