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

#ifndef FRONTENDS_SDL_FRONTEND_H
#define FRONTENDS_SDL_FRONTEND_H

#include <filesystem>
#include <cstdint>
#include <vector>

#include <SDL3/SDL.h>
#include <SDL3/SDL_audio.h>

#include "texture.hpp"

#include "frontends/gui/gui.hpp"

class Oric;
class Memory;


class Frontend
{
public:
    static const uint8_t texture_width = 240;
    static const uint16_t texture_height = 224;
    static const uint8_t texture_bpp = 4;

    explicit Frontend(Oric& oric);
    ~Frontend();

    /**
     * Initialize graphics output.
     * @return true on success
     */
    virtual bool init_graphics();

    /**
     * Initialize sound
     * @return true on success
     */
    virtual bool init_sound();

    /**
     * Pause sound.
     * @param pause_on true if sound should be paused, false otherwise
     */
    virtual void pause_sound(bool pause_on);

    /**
     * Lock audio playback.
     */
    virtual void lock_audio() {
        if (! audio_locked) {
            SDL_LockAudioStream(sound_audio_stream);
            audio_locked = true;
        }
    }

    /**
     * Unlock audio playback.
     */
    virtual void unlock_audio() {
        if (audio_locked) {
            SDL_UnlockAudioStream(sound_audio_stream);
            audio_locked = false;
        }
    }

    /**
     * Perform all tasks happening each frame.
     * @return true if machine should continue.
     */
    virtual bool handle_frame();

    /**
     * Render graphics.
     * @param pixels reference to pixels to render
     */
    virtual void render_graphics(std::vector<uint8_t>& pixels);

    /**
     * Get reference to status bar handler.
     * @return reference to status bar handler
     */
    virtual StatusBar& get_status_bar() { return gui.status_bar(); }

    virtual std::optional<std::filesystem::path> select_file(const std::string& title);

    /**
     * Close sound.
     */
    void close_sound() const;

protected:
    /**
     * Close graphics output.
     */
    void close_graphics();

    /**
     * Close SDL.
     */
    static void close_sdl();

    Oric& oric;

    SDL_Window* sdl_window;
    SDL_GLContext gl_context;

    uint32_t gl_program;
    uint32_t gl_vao;
    uint32_t gl_vbo;
    int32_t gl_u_texture;

    Gui gui;
    Texture oric_texture;

    std::vector<uint8_t> status_pixels;

    SDL_AudioStream* sound_audio_stream;
    bool audio_locked;

    // Variables bound to GL shader.
    int32_t gl_u_enable_scanlines;
    int32_t gl_u_enable_vertical_lines;
    int32_t gl_u_enable_vignette;
    float gl_u_vignette_strength;

    // Current video shader config.
    int32_t enable_scanlines;
    int32_t enable_vertical_lines;
    int32_t enable_vignette;
    float vignette_strength;
};


#endif // FRONTENDS_SDL_FRONTEND_H
