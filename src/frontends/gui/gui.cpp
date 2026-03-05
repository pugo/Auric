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
#include <imgui_stdlib.h>
#include "oric.hpp"

Gui::Gui(Oric& oric) : oric(oric)
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

void Gui::handle_event(SDL_Event& event, bool& wanted_key, bool& wanted_mouse)
{
    ImGui_ImplSDL2_ProcessEvent(&event);

    const ImGuiIO& io = ImGui::GetIO();
    wanted_key = io.WantCaptureKeyboard;
    wanted_mouse = io.WantCaptureMouse;
}

void Gui::render()
{
    // Start ImGui frame
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
    ImGui::Begin("Main menu", nullptr, ImGuiWindowFlags_NoMove);

    ImGui::Text("Snapshots:");
    if (ImGui::Button("Save snapshot")) {
        oric.get_machine().save_snapshot();
    }
    ImGui::SameLine();
    if (ImGui::Button("Load snapshot")) {
        oric.get_machine().load_snapshot();
    }

    ImGui::Text("Machine:");
    if (ImGui::Button("Reset")) {
        oric.get_machine().reset();
    }
    ImGui::SameLine();
    if (ImGui::Button("NMI")) {
        oric.get_machine().cpu->NMI();
    }
    ImGui::SameLine();
    if (ImGui::Button("Enter debugger")) {
        oric.get_machine().stop();
        oric.do_break();
    }

    ImGui::End();

    // Render ImGui
    ImGui::Render();
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), sdl_renderer);
}
