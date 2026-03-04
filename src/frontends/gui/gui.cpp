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

#include "gui.hpp"

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>

Gui::Gui() : foo(false)
{
}

void Gui::init(SDL_Window* sdl_window, SDL_Renderer* sdl_renderer)
{
    this->sdl_window = sdl_window;
    this->sdl_renderer = sdl_renderer;

    // Initialize Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Setup ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForSDLRenderer(sdl_window, sdl_renderer);
    ImGui_ImplSDLRenderer2_Init(sdl_renderer);
}

void Gui::close()
{
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}

void Gui::handle_event(SDL_Event& event)
{
    ImGui_ImplSDL2_ProcessEvent(&event);
}

void Gui::render()
{
    // Start ImGui frame
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    // Render ImGui window with "Hello world"
    ImGui::Begin("Hello ImGui");
    ImGui::Text("I like fistelspricka!");
    ImGui::Checkbox("Demo Window", &foo);
    ImGui::End();

    ImGui::Begin("Tomten");
    ImGui::Text("Finns inte.");
    ImGui::End();

    // Render ImGui
    ImGui::Render();
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), sdl_renderer);
}
