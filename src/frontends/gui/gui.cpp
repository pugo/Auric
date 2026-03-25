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
#include <imgui_impl_sdl3.h>
#include <imgui_impl_opengl3.h>
#include <imgui_stdlib.h>
#include "oric.hpp"

Gui::Gui(Oric& oric) :
    oric(oric), sdl_window(nullptr), gl_context(nullptr), _status_bar(0, 0)
{
}

void Gui::init(SDL_Window* sdl_window, SDL_GLContext gl_context)
{
    if (initialized) {
        return;
    }

    this->sdl_window = sdl_window;
    this->gl_context = gl_context;

    // Initialize Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Setup ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForOpenGL(sdl_window, gl_context);
    ImGui_ImplOpenGL3_Init("#version 150");
    initialized = true;
}

void Gui::close()
{
    if (!initialized) {
        return;
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    initialized = false;
}

void Gui::handle_event(SDL_Event& event, bool& wanted_key, bool& wanted_mouse)
{
    ImGui_ImplSDL3_ProcessEvent(&event);

    const ImGuiIO& io = ImGui::GetIO();
    wanted_key = io.WantCaptureKeyboard;
    wanted_mouse = io.WantCaptureMouse;
}

void Gui::render()
{
    // Start ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    if (show_gui) {
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
        ImGui::Begin("Main menu", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::Text("Media:");
        if (ImGui::Button("Insert tape")) {
            auto result = oric.get_frontend().select_file("Choose tape file");
            if (result.has_value()) {
                oric.get_machine().insert_tape(result.value());
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Insert disk")) {
            auto result = oric.get_frontend().select_file("Choose disk file");
            if (result.has_value()) {
                oric.get_machine().insert_disk(result.value());
            }
        }

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

        ImGui::Text("Video:");
        if (ImGui::Button("Video Settings")) {
            ImVec2 pos  = ImGui::GetWindowPos();
            ImVec2 size = ImGui::GetWindowSize();
            video_window_pos = ImVec2(pos.x + size.x + 10.0f, pos.y);

            show_video_window = true;
        }

        if (show_video_window) {
            ImGui::SetNextWindowPos(video_window_pos, ImGuiCond_Once);

            ImGui::Begin("Video Settings", &show_video_window, ImGuiWindowFlags_AlwaysAutoResize);

            if (ImGui::Checkbox("Enable scanlines", &enable_scanlines)) {
                oric.get_frontend().set_enable_artifact_lines(enable_scanlines, enable_vertical_lines);
            }

            if (ImGui::Checkbox("Enable vertical lines", &enable_vertical_lines)) {
                oric.get_frontend().set_enable_artifact_lines(enable_scanlines, enable_vertical_lines);

            }

            if (ImGui::Checkbox("Enable vignette", &enable_vignette)) {
                oric.get_frontend().set_vignette(enable_vignette, vignette_strength);
            }

            if (ImGui::SliderFloat("Vignette strength", &vignette_strength, 0.0f, 1.0f)) {
                oric.get_frontend().set_vignette(enable_vignette, vignette_strength);
            }

            ImGui::End();
        }

        ImGui::End();
    }

    _status_bar.render();

    // Render ImGui
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Gui::set_video_params(bool enable_scanlines, bool enable_vertical_lines, bool enable_vignette, float vignette_strength)
{
    this->enable_scanlines = enable_scanlines;
    this->enable_vertical_lines = enable_vertical_lines;
    this->enable_vignette = enable_vignette;
    this->vignette_strength = vignette_strength;
}
