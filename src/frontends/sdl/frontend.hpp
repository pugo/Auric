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

#ifndef FRONTENDS_SDL_FRONTEND_H
#define FRONTENDS_SDL_FRONTEND_H

#include <map>
#include <memory>
#include <iostream>
#include <ostream>
#include <vector>
#include <SDL.h>
#include <SDL_audio.h>

#include "texture.hpp"
#include "status_bar.hpp"

class Oric;
class Memory;

typedef std::map<int32_t, uint8_t> KeyMap_t;
typedef std::pair<int32_t, bool> KeyPress_t;
typedef std::map<KeyPress_t, KeyPress_t> KeyTranslation_t;


class Frontend
{
public:
    static const uint8_t texture_width = 240;
    static const uint16_t texture_height = 224;
    static const uint8_t texture_bpp = 4;

    Frontend(Oric* oric);
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
            SDL_LockAudioDevice(sound_audio_device_id);
            audio_locked = true;
        }
    }

    /**
     * Unlock audio playback.
     */
    virtual void unlock_audio() {
        if (audio_locked) {
            SDL_UnlockAudioDevice(sound_audio_device_id);
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
     * @param pixels refernce to pixels to render
     */
    virtual void render_graphics(std::vector<uint8_t>& pixels);

    /**
     * Get reference to status bar handler.
     * @return reference to status bar handler
     */
    virtual StatusBar& get_status_bar() { return status_bar; }

    /**
     * Close sound.
     */
    void close_sound();

protected:
    /**
     * Close graphics output.
     */
    void close_graphics();

    /**
     * Close SDL.
     */
    void close_sdl();

    Oric* oric;

    SDL_Window* sdl_window;
    SDL_Renderer* sdl_renderer;
    SDL_AudioDeviceID sound_audio_device_id;

    Texture oric_texture;
    StatusBar status_bar;

    KeyMap_t key_map;
    KeyTranslation_t key_translations;

    std::vector<uint8_t> status_pixels;

    bool audio_locked;
};


#endif // FRONTENDS_SDL_FRONTEND_H
