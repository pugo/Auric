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

#ifndef FRONTENDS_GUI_MEMORY_MAP_WINDOW_H
#define FRONTENDS_GUI_MEMORY_MAP_WINDOW_H

#include <cstdint>
#include <vector>
#include <cstddef>
#include <glad/glad.h>
#include <imgui.h>

class Oric;

/**
 * Memory map visualization window for ImGui.
 * Displays 64KB of memory as a 256x256 pixel grid using a color palette.
 */
class MemoryMapWindow
{
public:
    explicit MemoryMapWindow(Oric& oric);
    ~MemoryMapWindow();

    /**
     * Initialize the memory map window (create GL texture).
     * @return true on success
     */
    bool init();

    /**
     * Close the memory map window (destroy GL texture).
     */
    void close();

    /**
     * Update and render the memory map window.
     * Should be called once per frame from Gui::render().
     */
    void render();

    /**
     * Set the update frequency in Hz (default 10 Hz).
     * @param hz frequency in Hz
     */
    void set_update_frequency(float hz) { update_frequency = hz; }

    /**
     * Set window visibility.
     * @param visible true to show the window, false to hide
     */
    void set_visible(bool visible) { window_open = visible; }

    /**
     * Check if window is visible.
     * @return true if window is visible
     */
    bool is_visible() const { return window_open; }

private:
    /**
     * Update the memory texture from the current machine memory.
     */
    void update_texture();

    /**
     * Generate a color from a memory value using the color palette.
     * @param value memory value (0-255)
     * @return RGBA color as uint32_t
     */
    static uint32_t get_color_for_value(uint8_t value);

    Oric& oric;

    // Texture data
    static constexpr int GRID_WIDTH = 256;
    static constexpr int GRID_HEIGHT = 256;
    static constexpr int TEXTURE_SIZE = GRID_WIDTH * GRID_HEIGHT;

    std::vector<uint32_t> texture_data;
    GLuint gl_texture{0};

    // Update timing
    float update_frequency{10.0f};  // Hz
    float time_accumulator{0.0f};
    float update_interval{0.1f};    // seconds

    // Window state
    bool window_open{true};
    ImVec2 window_pos{600, 10};
};

#endif // FRONTENDS_GUI_MEMORY_MAP_WINDOW_H

