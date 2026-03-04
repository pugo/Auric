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

#include <SDL.h>

class Gui
{
public:
    Gui();
    ~Gui() = default;

    void init(SDL_Window* sdl_window, SDL_Renderer* sdl_renderer);
    void close();
    void handle_event(SDL_Event& event);
    void render();

private:
    SDL_Window* sdl_window;
    SDL_Renderer* sdl_renderer;
    bool foo;
};


#endif // FRONTENDS_GUI_GUI_H
