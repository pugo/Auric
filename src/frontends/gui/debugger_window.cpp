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
#include <cstdlib>

#include "oric.hpp"
#include "machine.hpp"
#include "memory_map_window.hpp"
#include "disk/drive_microdrive.hpp"

DebuggerWindow::DebuggerWindow(Oric& oric, MemoryMapWindow& memory_map_window) :
    oric(oric), memory_map_window(memory_map_window)
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

void DebuggerWindow::execute_gui_command(const std::string& command)
{
    const auto result = oric.submit_debugger_command(command);
    append_output(result.output);
}

void DebuggerWindow::render_toolbar()
{
    if (ImGui::Button("Continue")) {
        execute_gui_command("g");
    }
    ImGui::SameLine();
    if (ImGui::Button("Break")) {
        execute_gui_command("b");
    }
    ImGui::SameLine();
    if (ImGui::Button("Step")) {
        execute_gui_command("s");
    }
    ImGui::SameLine();
    if (ImGui::Button("Step over")) {
        execute_gui_command("so");
    }
    ImGui::SameLine();
    if (ImGui::Button("Step out")) {
        execute_gui_command("su");
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(70.0f);
    ImGui::InputText("##RunToCursor", cursor_address, sizeof(cursor_address), ImGuiInputTextFlags_CharsHexadecimal);
    ImGui::SameLine();
    if (ImGui::Button("Run to cursor")) {
        execute_gui_command(std::string("rtc $") + cursor_address);
    }
    ImGui::SameLine();
    ImGui::Checkbox("Disassembly", &show_disassembly);
    ImGui::SameLine();
    ImGui::Checkbox("CPU", &show_cpu);
    ImGui::SameLine();
    ImGui::Checkbox("Breakpoints", &show_breakpoints);
    ImGui::SameLine();
    ImGui::Checkbox("Trace", &show_trace);
}

void DebuggerWindow::render_disassembly(float height)
{
    auto& machine = oric.get_machine();
    const uint16_t pc = machine.cpu->get_pc();
    ImGui::Text("PC: $%04X", pc);
    ImGui::BeginChild("LiveDisassembly", ImVec2(0, height), true, ImGuiWindowFlags_HorizontalScrollbar);
    uint16_t address = pc;
    for (int instruction = 0; instruction < 14; ++instruction) {
        const auto disassembly = machine.get_monitor().disassemble_to_string(address);
        if (address == pc) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.82f, 0.25f, 1.0f));
            ImGui::TextUnformatted(">> ");
            ImGui::SameLine();
            ImGui::TextUnformatted(disassembly.output.c_str());
            ImGui::PopStyleColor();
        }
        else {
            ImGui::TextUnformatted("   ");
            ImGui::SameLine();
            ImGui::TextUnformatted(disassembly.output.c_str());
        }

        if (disassembly.next_address <= address) {
            break;
        }
        address = disassembly.next_address;
    }
    ImGui::EndChild();
}

void DebuggerWindow::render_cpu()
{
    auto& cpu = *oric.get_machine().cpu;
    ImGui::Text("A: %02X   X: %02X   Y: %02X   SP: %02X", cpu.A, cpu.X, cpu.Y, cpu.get_sp());
    ImGui::Text("N: %d  V: %d  B: %d  D: %d  I: %d  Z: %d  C: %d",
                static_cast<int>(cpu.N_INTERN != 0), static_cast<int>(cpu.V), static_cast<int>(cpu.B),
                static_cast<int>(cpu.D), static_cast<int>(cpu.I), static_cast<int>(cpu.Z_INTERN == 0),
                static_cast<int>(cpu.C));
    ImGui::Text("Cycles: %llu", static_cast<unsigned long long>(oric.get_machine().get_total_cycles()));
}

void DebuggerWindow::render_breakpoints(float height)
{
    auto& machine = oric.get_machine();
    ImGui::BeginChild("BreakpointList", ImVec2(0, height), true);
    static char breakpoint_address[7] = "0000";
    ImGui::SetNextItemWidth(70.0f);
    ImGui::InputText("Address##Breakpoint", breakpoint_address, sizeof(breakpoint_address), ImGuiInputTextFlags_CharsHexadecimal);
    ImGui::SameLine();
    if (ImGui::Button("Add breakpoint")) {
        char* end = nullptr;
        const auto address = std::strtoul(breakpoint_address, &end, 16);
        if (end != breakpoint_address && *end == '\0' && address <= 0xFFFF) {
            machine.cpu->set_breakpoint(static_cast<uint16_t>(address));
        }
    }

    for (const uint16_t address : machine.cpu->get_breakpoints()) {
        ImGui::PushID(static_cast<int>(address));
        bool enabled = machine.cpu->breakpoint_enabled(address);
        ImGui::Checkbox("##enabled", &enabled);
        if (enabled != machine.cpu->breakpoint_enabled(address)) {
            machine.cpu->set_breakpoint_enabled(address, enabled);
        }
        ImGui::SameLine();
        ImGui::Text("$%04X", address);
        ImGui::SameLine();
        if (ImGui::SmallButton("Clear")) {
            machine.cpu->clear_breakpoint(address);
        }
        ImGui::PopID();
    }
    ImGui::EndChild();
}

void DebuggerWindow::render_watchpoints()
{
    auto& machine = oric.get_machine();
    ImGui::Text("New watchpoint");
    ImGui::SetNextItemWidth(70.0f);
    ImGui::InputScalar("Start", ImGuiDataType_U16, &watch_start, nullptr, nullptr, "%04X", ImGuiInputTextFlags_CharsHexadecimal);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(70.0f);
    ImGui::InputScalar("End", ImGuiDataType_U16, &watch_end, nullptr, nullptr, "%04X", ImGuiInputTextFlags_CharsHexadecimal);
    ImGui::Checkbox("Read", &watch_read);
    ImGui::SameLine();
    ImGui::Checkbox("Write", &watch_write);
    ImGui::SameLine();
    if (ImGui::Button("Add watchpoint")) {
        machine.add_watchpoint(watch_start, watch_end, watch_read, watch_write);
    }

    for (size_t index = 0; index < machine.get_watchpoints().size(); ++index) {
        const auto& watchpoint = machine.get_watchpoints()[index];
        ImGui::PushID(static_cast<int>(index));
        bool enabled = watchpoint.enabled;
        ImGui::Checkbox("##enabled", &enabled);
        if (enabled != watchpoint.enabled) {
            machine.set_watchpoint_enabled(index, enabled);
        }
        ImGui::SameLine();
        ImGui::Text("$%04X-$%04X %s%s", watchpoint.start, watchpoint.end,
                    watchpoint.on_read ? "R" : "", watchpoint.on_write ? "W" : "");
        ImGui::SameLine();
        if (ImGui::SmallButton("Remove")) {
            machine.remove_watchpoint(index);
            ImGui::PopID();
            break;
        }
        ImGui::PopID();
    }
}

void DebuggerWindow::render_memory()
{
    auto& machine = oric.get_machine();
    ImGui::SetNextItemWidth(90.0f);
    ImGui::InputScalar("Address", ImGuiDataType_U16, &memory_address, nullptr, nullptr, "%04X", ImGuiInputTextFlags_CharsHexadecimal);
    ImGui::BeginChild("HexEditor", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
    for (uint16_t row = 0; row < 16; ++row) {
        const uint16_t address = static_cast<uint16_t>(memory_address + row * 16);
        ImGui::Text("%04X:", address);
        ImGui::SameLine();
        for (uint16_t column = 0; column < 16; ++column) {
            const uint16_t current_address = static_cast<uint16_t>(address + column);
            uint8_t value = machine.peek_byte(current_address);
            ImGui::PushID(static_cast<int>(current_address));
            ImGui::SetNextItemWidth(24.0f);
            if (ImGui::InputScalar("##byte", ImGuiDataType_U8, &value, nullptr, nullptr, "%02X", ImGuiInputTextFlags_CharsHexadecimal)) {
                machine.poke_byte(current_address, value);
            }
            ImGui::PopID();
            if (column != 15) {
                ImGui::SameLine(0.0f, 2.0f);
            }
        }
    }
    ImGui::EndChild();
}

void DebuggerWindow::render_console()
{
    const float input_height = ImGui::GetFrameHeightWithSpacing();
    ImGui::BeginChild("DebuggerOutput", ImVec2(0.0f, -input_height), true, ImGuiWindowFlags_HorizontalScrollbar);
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

void DebuggerWindow::render_trace()
{
    auto& machine = oric.get_machine();
    bool tracing = machine.trace_is_enabled();
    if (ImGui::Checkbox("Enable trace", &tracing)) {
        machine.set_trace_enabled(tracing);
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear trace")) {
        machine.clear_trace();
    }
    ImGui::Separator();
    ImGui::BeginChild("TraceLog", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
    for (const auto& entry : machine.get_trace()) {
        ImGui::Text("%010llu  $%04X  (%u)  %s",
                    static_cast<unsigned long long>(entry.cycle), entry.pc,
                    static_cast<unsigned int>(entry.cycles), entry.instruction.c_str());
    }
    ImGui::EndChild();
}

void DebuggerWindow::render_hardware()
{
    auto& machine = oric.get_machine();
    const auto& via = machine.mos_6522->get_state();
    ImGui::Text("VIA 6522");
    ImGui::Text("ORA $%02X  ORB $%02X  DDRA $%02X  DDRB $%02X", via.ora, via.orb, via.ddra, via.ddrb);
    ImGui::Text("T1 $%04X  T2 $%04X  IFR $%02X  IER $%02X", via.t1_counter, via.t2_counter, via.ifr, via.ier);
    ImGui::Separator();
    ImGui::Text("AY-3-8912");
    const auto& ay = machine.ay3->get_state();
    ImGui::Text("Register: $%02X  Enable: $%02X  Noise: $%02X", ay.current_register,
                ay.registers[AY3_8912::ENABLE], ay.registers[AY3_8912::NOICE_PERIOD]);
    for (int channel = 0; channel < 3; ++channel) {
        ImGui::Text("Channel %c: period %u  volume %u", 'A' + channel,
                    static_cast<unsigned int>(ay.channels[channel].tone_period),
                    static_cast<unsigned int>(ay.channels[channel].volume));
    }
    ImGui::Separator();
    ImGui::Text("Storage");
    if (const auto* drive = dynamic_cast<const DriveMicrodrive*>(machine.get_disk_drive())) {
        const auto& wd = drive->get_wd1793_state();
        ImGui::Text("WD1793: command $%02X status $%02X track %u sector %u offset %u",
                    wd.command, wd.status, wd.track, wd.sector, wd.offset);
        ImGui::Text("WD1793: current track %u current sector %u DRQ counter %d",
                    wd.current_track_number, wd.current_sector_number, wd.data_request_counter);
    }
    else {
        ImGui::Text("WD1793: not initialized");
    }
    ImGui::Text("Tape: %s", machine.get_tape()->debug_status().c_str());
}

void DebuggerWindow::render(const ImVec2& window_pos, const ImVec2& window_size)
{
    if (!window_open) {
        return;
    }

    const bool was_open = window_open;
    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(window_size, ImGuiCond_Always);

    constexpr ImGuiWindowFlags workspace_flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
    if (ImGui::Begin("Debugger Workspace", &window_open, workspace_flags)) {
        render_toolbar();
        ImGui::Separator();
        if (ImGui::BeginTable("DebuggerWorkspaceLayout", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV)) {
            ImGui::TableNextColumn();
            const float left_column_height = ImGui::GetContentRegionAvail().y;
            const bool has_lower_panes = show_cpu || show_breakpoints;
            const float disassembly_height = has_lower_panes
                ? std::max(160.0f, left_column_height * 0.42f)
                : 0.0f;

            if (show_disassembly && ImGui::CollapsingHeader("Live disassembly", ImGuiTreeNodeFlags_DefaultOpen)) {
                render_disassembly(disassembly_height);
            }
            if (show_cpu && ImGui::CollapsingHeader("CPU registers", ImGuiTreeNodeFlags_DefaultOpen)) {
                render_cpu();
            }
            if (show_breakpoints && ImGui::CollapsingHeader("Breakpoints", ImGuiTreeNodeFlags_DefaultOpen)) {
                render_breakpoints(0.0f);
            }

            ImGui::TableNextColumn();
            if (ImGui::BeginTabBar("DebuggerPanes")) {
                if (show_watchpoints && ImGui::BeginTabItem("Watchpoints")) {
                    render_watchpoints();
                    ImGui::EndTabItem();
                }
                if (show_memory && ImGui::BeginTabItem("Hex editor")) {
                    render_memory();
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Memory map")) {
                    memory_map_window.render_contents();
                    ImGui::EndTabItem();
                }
            if (show_console && ImGui::BeginTabItem("Console")) {
                    render_console();
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Hardware")) {
                    render_hardware();
                    ImGui::EndTabItem();
                }
                if (show_trace && ImGui::BeginTabItem("Trace")) {
                    render_trace();
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
            ImGui::EndTable();
        }
    }
    ImGui::End();

    if (was_open && !window_open && oric.is_halted()) {
        oric.continue_execution();
    }
}
