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

#ifndef FRONTENDS_SDL_GL_LOADER_H
#define FRONTENDS_SDL_GL_LOADER_H

#include <SDL3/SDL.h>

#if __has_include(<glad/gl.h>)
#include <glad/gl.h>
inline bool load_gl_functions()
{
    return gladLoadGL(reinterpret_cast<GLADloadfunc>(SDL_GL_GetProcAddress)) != 0;
}
#elif __has_include(<glad/glad.h>)
#include <glad/glad.h>
inline bool load_gl_functions()
{
    return gladLoadGLLoader(reinterpret_cast<GLADloadproc>(SDL_GL_GetProcAddress)) != 0;
}
#else
#error "No glad header found (expected <glad/gl.h> or <glad/glad.h>)."
#endif

#endif // FRONTENDS_SDL_GL_LOADER_H
