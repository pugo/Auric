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

#include "memory_map_window.hpp"
#include <imgui.h>
#include "oric.hpp"
#include "machine.hpp"
#include <cmath>

#include <array>
#include <cstdint>

static constexpr std::array<uint32_t, 256> MEMORY_VALUE_COLORS = {{
    0xFF000000u, 0xFF431532u, 0xFF4A1833u, 0xFF511B34u, 0xFF581E35u, 0xFF5F2136u, 0xFF662437u, 0xFF6D2738u,
    0xFF732A39u, 0xFF792D3Au, 0xFF802F3Bu, 0xFF86323Cu, 0xFF8B353Du, 0xFF91383Eu, 0xFF973B3Fu, 0xFF9C3E3Fu,
    0xFFA24040u, 0xFFA74341u, 0xFFAC4641u, 0xFFB14942u, 0xFFB54B42u, 0xFFBA4E43u, 0xFFBF5144u, 0xFFC35444u,
    0xFFC75644u, 0xFFCB5945u, 0xFFCF5C45u, 0xFFD35E45u, 0xFFD66146u, 0xFFDA6446u, 0xFFDD6646u, 0xFFE06946u,
    0xFFE36B46u, 0xFFE66E47u, 0xFFE97147u, 0xFFEB7347u, 0xFFEE7647u, 0xFFF07847u, 0xFFF27B47u, 0xFFF47D46u,
    0xFFF68046u, 0xFFF88246u, 0xFFFA8546u, 0xFFFB8746u, 0xFFFC8A45u, 0xFFFD8C45u, 0xFFFE8F44u, 0xFFFE9143u,
    0xFFFF9442u, 0xFFFF9641u, 0xFFFF9940u, 0xFFFE9B3Eu, 0xFFFE9E3Du, 0xFFFDA03Bu, 0xFFFCA33Au, 0xFFFBA538u,
    0xFFFAA837u, 0xFFF8AB35u, 0xFFF7AD33u, 0xFFF5AF31u, 0xFFF4B22Fu, 0xFFF2B42Eu, 0xFFF0B72Cu, 0xFFEEB92Au,
    0xFFEBBC28u, 0xFFE9BE27u, 0xFFE7C025u, 0xFFE4C323u, 0xFFE2C522u, 0xFFDFC720u, 0xFFDDC91Fu, 0xFFDACB1Eu,
    0xFFD8CD1Cu, 0xFFD5D01Bu, 0xFFD2D21Au, 0xFFD0D41Au, 0xFFCDD519u, 0xFFCAD718u, 0xFFC8D918u, 0xFFC5DB18u,
    0xFFC2DD18u, 0xFFC0DE18u, 0xFFBDE018u, 0xFFBBE219u, 0xFFB9E319u, 0xFFB6E41Au, 0xFFB4E61Cu, 0xFFB2E71Du,
    0xFFAFE91Fu, 0xFFACEA20u, 0xFFAAEB22u, 0xFFA7EC25u, 0xFFA4EE27u, 0xFFA1EF2Au, 0xFF9EF02Cu, 0xFF9BF12Fu,
    0xFF98F232u, 0xFF94F335u, 0xFF91F438u, 0xFF8EF53Cu, 0xFF8AF63Fu, 0xFF87F743u, 0xFF84F846u, 0xFF80F84Au,
    0xFF7DF94Eu, 0xFF7AFA52u, 0xFF76FA55u, 0xFF73FB59u, 0xFF6FFC5Du, 0xFF6CFC61u, 0xFF69FD65u, 0xFF66FD69u,
    0xFF62FE6Du, 0xFF5FFE71u, 0xFF5CFE75u, 0xFF59FE79u, 0xFF56FF7Du, 0xFF53FF80u, 0xFF51FF84u, 0xFF4EFF88u,
    0xFF4BFF8Bu, 0xFF49FF8Fu, 0xFF47FF92u, 0xFF44FE96u, 0xFF42FE99u, 0xFF40FE9Cu, 0xFF3FFD9Fu, 0xFF3DFDA1u,
    0xFF3CFCA4u, 0xFF3AFCA7u, 0xFF39FBA9u, 0xFF38FBACu, 0xFF37FAAFu, 0xFF36F9B1u, 0xFF36F8B4u, 0xFF35F7B7u,
    0xFF35F6B9u, 0xFF34F5BCu, 0xFF34F4BEu, 0xFF34F3C1u, 0xFF34F1C3u, 0xFF34F0C6u, 0xFF34EFC8u, 0xFF34EDCBu,
    0xFF34ECCDu, 0xFF34EAD0u, 0xFF35E9D2u, 0xFF35E7D4u, 0xFF35E5D7u, 0xFF36E4D9u, 0xFF36E2DBu, 0xFF37E0DDu,
    0xFF37DFDFu, 0xFF37DDE1u, 0xFF38DBE3u, 0xFF38D9E5u, 0xFF39D7E7u, 0xFF39D5E9u, 0xFF39D3EBu, 0xFF3AD1ECu,
    0xFF3ACFEEu, 0xFF3ACDEFu, 0xFF3ACBF1u, 0xFF3AC9F2u, 0xFF3AC7F4u, 0xFF3AC5F5u, 0xFF3AC3F6u, 0xFF3AC1F7u,
    0xFF39BEF8u, 0xFF39BCF9u, 0xFF39BAFAu, 0xFF38B8FBu, 0xFF37B6FBu, 0xFF36B3FCu, 0xFF36B1FCu, 0xFF35AEFDu,
    0xFF34ACFDu, 0xFF33A9FEu, 0xFF32A7FEu, 0xFF31A4FEu, 0xFF30A1FEu, 0xFF2F9EFEu, 0xFF2D9BFEu, 0xFF2C99FEu,
    0xFF2B96FEu, 0xFF2A93FEu, 0xFF2990FEu, 0xFF278DFDu, 0xFF268AFDu, 0xFF2587FCu, 0xFF2384FCu, 0xFF2281FBu,
    0xFF217EFBu, 0xFF1F7BFAu, 0xFF1E78F9u, 0xFF1D75F9u, 0xFF1C72F8u, 0xFF1A6FF7u, 0xFF196CF6u, 0xFF1869F5u,
    0xFF1766F4u, 0xFF1563F3u, 0xFF1460F2u, 0xFF135DF1u, 0xFF125BF0u, 0xFF1158EFu, 0xFF1055EDu, 0xFF0F53ECu,
    0xFF0E50EBu, 0xFF0D4EEAu, 0xFF0C4BE8u, 0xFF0C49E7u, 0xFF0B47E5u, 0xFF0A45E4u, 0xFF0A43E2u, 0xFF0941E1u,
    0xFF083FDFu, 0xFF083DDDu, 0xFF073BDCu, 0xFF0739DAu, 0xFF0637D8u, 0xFF0635D6u, 0xFF0533D4u, 0xFF0531D2u,
    0xFF052FD0u, 0xFF042DCEu, 0xFF042BCCu, 0xFF042ACAu, 0xFF0328C8u, 0xFF0326C5u, 0xFF0325C3u, 0xFF0223C1u,
    0xFF0221BEu, 0xFF0220BCu, 0xFF021EB9u, 0xFF021DB7u, 0xFF011BB4u, 0xFF011AB2u, 0xFF0118AFu, 0xFF0117ACu,
    0xFF0116A9u, 0xFF0114A7u, 0xFF0113A4u, 0xFF0112A1u, 0xFF01109Eu, 0xFF010F9Bu, 0xFF010E98u, 0xFF010D95u,
    0xFF010B92u, 0xFF010A8Eu, 0xFF02098Bu, 0xFF020888u, 0xFF020785u, 0xFF020681u, 0xFF02057Eu, 0xFF03047Au
}};

uint32_t MemoryMapWindow::get_color_for_value(uint8_t value)
{
    return MEMORY_VALUE_COLORS[value];
}

MemoryMapWindow::MemoryMapWindow(Oric& oric) :
    oric(oric), texture_data(TEXTURE_SIZE, 0)
{
    update_interval = 1.0f / update_frequency;
}

MemoryMapWindow::~MemoryMapWindow()
{
    close();
}

bool MemoryMapWindow::init()
{
    if (gl_texture != 0) {
        return true;
    }

    glGenTextures(1, &gl_texture);

    if (gl_texture == 0) {
        return false;
    }

    glBindTexture(GL_TEXTURE_2D, gl_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, GRID_WIDTH, GRID_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);

    return true;
}

void MemoryMapWindow::close()
{
    if (gl_texture != 0) {
        glDeleteTextures(1, &gl_texture);
        gl_texture = 0;
    }
}

void MemoryMapWindow::update_texture()
{
    uint8_t* mem = oric.get_machine().memory.mem;

    for (int i = 0; i < TEXTURE_SIZE; ++i) {
        texture_data[i] = get_color_for_value(mem[i]);
    }

    glBindTexture(GL_TEXTURE_2D, gl_texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, GRID_WIDTH, GRID_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, texture_data.data());
    glBindTexture(GL_TEXTURE_2D, 0);
}

void MemoryMapWindow::render()
{
    ImGuiIO& io = ImGui::GetIO();
    time_accumulator += io.DeltaTime;

    if (time_accumulator >= update_interval) {
        update_texture();
        time_accumulator -= update_interval;
    }

    if (window_open) {
        ImGui::SetNextWindowPos(window_pos, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(516, 550), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Memory Map", &window_open)) {
            ImGui::Text("Memory Map (64KB as 256x256 grid)");
            ImGui::Separator();

            if (ImGui::SliderFloat("Update Frequency (Hz)", &update_frequency, 1.0f, 60.0f)) {
                update_interval = 1.0f / update_frequency;
            }

            ImGui::Separator();
            if (gl_texture != 0) {
                ImGui::Image(reinterpret_cast<void*>(static_cast<intptr_t>(gl_texture)), ImVec2(512, 512),
                             ImVec2(0, 0), ImVec2(1, 1), ImVec4(1, 1, 1, 1), ImVec4(1, 1, 1, 0.5f));

                if (ImGui::IsItemHovered()) {
                    ImVec2 mouse_pos = ImGui::GetMousePos();
                    ImVec2 image_pos = ImGui::GetItemRectMin();
                    ImVec2 image_size = ImGui::GetItemRectMax();
                    image_size.x -= image_pos.x;
                    image_size.y -= image_pos.y;
                    float x = (mouse_pos.x - image_pos.x) / image_size.x;
                    float y = (mouse_pos.y - image_pos.y) / image_size.y;

                    if (x >= 0 && x <= 1 && y >= 0 && y <= 1) {
                        int mem_x = static_cast<int>(x * GRID_WIDTH);
                        int mem_y = static_cast<int>(y * GRID_HEIGHT);
                        int address = mem_y * GRID_WIDTH + mem_x;
                        ImGui::BeginTooltip();
                        ImGui::Text("Address: $%04X", address);
                        ImGui::Text("Value: $%02X (%d)", oric.get_machine().memory.mem[address],
                                    oric.get_machine().memory.mem[address]);
                        ImGui::EndTooltip();
                    }
                }
            }
        }
        ImGui::End();
    }
}
