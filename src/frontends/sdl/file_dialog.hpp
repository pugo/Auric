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

#ifndef FRONTENDS_SDL_FILEDIALOG_H
#define FRONTENDS_SDL_FILEDIALOG_H

#include <filesystem>
#include <vector>

#include <SDL3/SDL.h>


class FileSelectDialog
{
public:
    static std::optional<std::filesystem::path> open(SDL_Window* window, const std::vector<std::string>& extensions)
    {
        result.reset();
        finished = false;

        std::string pattern;

        for (size_t i = 0; i < extensions.size(); i++) {
            if (i > 0) pattern += ";";
            pattern += extensions[i];
        }

        SDL_DialogFileFilter filter{"Files", pattern.c_str()};

        SDL_ShowOpenFileDialog(callback, nullptr, window, &filter, 1, nullptr, false);

        // Wait for dialog to be finished.
        while (!finished)
        {
            SDL_Event e;

            while (SDL_PollEvent(&e))
            {
                if (e.type == SDL_EVENT_QUIT) {
                    finished = true;
                }
            }

            SDL_Delay(10);
        }

        return result;
    }

private:
    static void SDLCALL callback(void*, const char *const *filelist, int)
    {
        if (filelist && filelist[0])
            result = std::string(filelist[0]);

        finished = true;
    }

    static inline std::optional<std::filesystem::path> result;
    static inline bool finished = false;
};


#endif // FRONTENDS_SDL_FILEDIALOG_H
