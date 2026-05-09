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

#ifndef FRONTENDS_GUI_GUI_H
#define FRONTENDS_GUI_GUI_H

#include <SDL3/SDL.h>
#include <imgui.h>

#include "frontends/gui/status_bar.hpp"
#include "frontends/gui/memory_map_window.hpp"

class Oric;

class Gui
{
public:
    Gui(Oric& oric);
    ~Gui() = default;

    void init(SDL_Window* sdl_window, SDL_GLContext gl_context);
    void close();
    void handle_event(SDL_Event& event, bool& wanted_key, bool& wanted_mouse);
    void render();

    void set_video_params(bool enable_scanlines, bool enable_vertical_lines, bool enable_vignette, float vignette_strength);

    StatusBar& status_bar() { return _status_bar; }

    void toggle_gui() { show_gui = !show_gui; }

    MemoryMapWindow& get_memory_map_window() { return memory_map_window; }

private:
    Oric& oric;

    SDL_Window* sdl_window;
    SDL_GLContext gl_context;

    StatusBar _status_bar;
    MemoryMapWindow memory_map_window;

    bool show_gui{false};
    bool initialized{false};
    bool show_video_window{false};
    bool show_memory_map_window{false};

    bool enable_scanlines{false};
    bool enable_vertical_lines{false};
    bool enable_vignette{false};
    float vignette_strength{0.2f};

    ImVec2 video_window_pos;
};


#endif // FRONTENDS_GUI_GUI_H
