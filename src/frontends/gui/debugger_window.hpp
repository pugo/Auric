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
#include <cstdint>
#include <imgui.h>

class Oric;
class MemoryMapWindow;

class DebuggerWindow
{
public:
    DebuggerWindow(Oric& oric, MemoryMapWindow& memory_map_window);

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
    void render_disassembly(float height);
    void render_cpu();
    void render_breakpoints(float height);
    void render_watchpoints();
    void render_memory();
    void render_console();
    void render_trace();
    void render_hardware();
    void render_toolbar();
    void execute_gui_command(const std::string& command);

    Oric& oric;
    MemoryMapWindow& memory_map_window;
    std::string output;
    std::string input;
    std::vector<std::string> command_history;
    std::string history_draft;
    int history_position{-1};
    bool window_open{false};
    bool scroll_to_bottom{false};
    bool focus_input{false};
    bool show_disassembly{true};
    bool show_cpu{true};
    bool show_breakpoints{true};
    bool show_watchpoints{true};
    bool show_memory{true};
    bool show_console{true};
    bool show_trace{false};
    uint16_t memory_address{0};
    uint16_t watch_start{0};
    uint16_t watch_end{0};
    bool watch_read{false};
    bool watch_write{true};
    char cursor_address[7]{"0000"};
};

#endif // FRONTENDS_GUI_DEBUGGER_WINDOW_H
