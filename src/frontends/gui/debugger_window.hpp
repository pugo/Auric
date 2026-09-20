// =========================================================================
//   Copyright (C) 2009-2026 by Anders Piniesjö <pugo@pugo.org>
//
//   This program is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
// =========================================================================

#ifndef FRONTENDS_GUI_DEBUGGER_WINDOW_H
#define FRONTENDS_GUI_DEBUGGER_WINDOW_H

#include <string>
#include <vector>
#include <imgui.h>

class Oric;

class DebuggerWindow
{
public:
    explicit DebuggerWindow(Oric& oric);

    void render(const ImVec2& window_pos, const ImVec2& window_size);

    void set_visible(bool visible) { window_open = visible; }
    bool is_visible() const { return window_open; }
    void request_input_focus() { focus_input = true; }

    void clear();
    void append_output(const std::string& output);

private:
    static int input_callback(ImGuiInputTextCallbackData* data);
    void submit_command();
    void navigate_history(ImGuiInputTextCallbackData* data);

    Oric& oric;
    std::string output;
    std::string input;
    std::vector<std::string> command_history;
    std::string history_draft;
    int history_position{-1};
    bool window_open{false};
    bool scroll_to_bottom{false};
    bool focus_input{false};
};

#endif // FRONTENDS_GUI_DEBUGGER_WINDOW_H
