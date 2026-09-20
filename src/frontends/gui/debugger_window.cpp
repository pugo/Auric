// =========================================================================
//   Copyright (C) 2009-2026 by Anders Piniesjö <pugo@pugo.org>
//
//   This program is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
// =========================================================================

#include "debugger_window.hpp"

#include <imgui.h>
#include <imgui_stdlib.h>

#include "oric.hpp"

DebuggerWindow::DebuggerWindow(Oric& oric) :
    oric(oric)
{
    append_output("* Oric Monitor *\n\n"
                  "        b <return> : break execution\n"
                  "        g <return> : continue execution\n"
                  "    h <return> : for help (more commands)\n\n");
}

void DebuggerWindow::clear()
{
    output.clear();
    scroll_to_bottom = true;
}

void DebuggerWindow::append_output(const std::string& text)
{
    if (text.empty()) {
        return;
    }

    output += text;
    if (!output.ends_with('\n')) {
        output += '\n';
    }
    scroll_to_bottom = true;
}

void DebuggerWindow::submit_command()
{
    if (!input.empty() && (command_history.empty() || command_history.back() != input)) {
        constexpr size_t max_history_size = 64;
        command_history.push_back(input);
        if (command_history.size() > max_history_size) {
            command_history.erase(command_history.begin());
        }
    }
    history_draft.clear();
    history_position = -1;

    append_output(">> " + input + "\n");
    auto result = oric.submit_debugger_command(input);
    append_output(result.output);
    input.clear();
}

int DebuggerWindow::input_callback(ImGuiInputTextCallbackData* data)
{
    if (data->EventFlag == ImGuiInputTextFlags_CallbackHistory) {
        static_cast<DebuggerWindow*>(data->UserData)->navigate_history(data);
    }
    return 0;
}

void DebuggerWindow::navigate_history(ImGuiInputTextCallbackData* data)
{
    if (command_history.empty()) {
        return;
    }

    if (history_position == -1) {
        history_draft.assign(data->Buf, data->BufTextLen);
        history_position = static_cast<int>(command_history.size());
    }

    if (data->EventKey == ImGuiKey_UpArrow) {
        if (history_position > 0) {
            --history_position;
        }
    }
    else if (data->EventKey == ImGuiKey_DownArrow) {
        if (history_position < static_cast<int>(command_history.size())) {
            ++history_position;
        }
    }

    const std::string& replacement = history_position == static_cast<int>(command_history.size())
        ? history_draft
        : command_history[history_position];
    data->DeleteChars(0, data->BufTextLen);
    data->InsertChars(0, replacement.c_str());
}

void DebuggerWindow::render(const ImVec2& window_pos, const ImVec2& window_size)
{
    if (!window_open) {
        return;
    }

    const bool was_open = window_open;
    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(window_size, ImGuiCond_Always);

    if (ImGui::Begin("Debugger", &window_open)) {
        const float input_height = ImGui::GetFrameHeightWithSpacing();
        const ImVec2 output_size(0.0f, -input_height);

        ImGui::BeginChild("DebuggerOutput", output_size, true, ImGuiWindowFlags_HorizontalScrollbar);
        ImGui::TextUnformatted(output.c_str());
        if (scroll_to_bottom) {
            ImGui::SetScrollHereY(1.0f);
            scroll_to_bottom = false;
        }
        ImGui::EndChild();

        ImGui::SetNextItemWidth(-1.0f);
        if (focus_input) {
            ImGui::SetKeyboardFocusHere();
            focus_input = false;
        }
        constexpr ImGuiInputTextFlags input_flags = ImGuiInputTextFlags_EnterReturnsTrue |
            ImGuiInputTextFlags_CallbackHistory;
        if (ImGui::InputText("##DebuggerInput", &input, input_flags, input_callback, this)) {
            submit_command();
            ImGui::SetKeyboardFocusHere(-1);
        }
    }
    ImGui::End();

    if (was_open && !window_open && oric.is_halted()) {
        oric.continue_execution();
    }
}
