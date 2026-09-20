// =========================================================================
//   Copyright (C) 2009-2026 by Anders Piniesjö <pugo@pugo.org>
//
//   This program is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
// =========================================================================

#include "debugger_controller.hpp"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <format>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "machine.hpp"

namespace
{

void trim(std::string& str)
{
    const auto is_not_space = [](unsigned char ch) {
        return !std::isspace(ch);
    };

    str.erase(str.begin(), std::find_if(str.begin(), str.end(), is_not_space));
    str.erase(std::find_if(str.rbegin(), str.rend(), is_not_space).base(), str.end());
}

std::vector<std::string> split_words(const std::string& str)
{
    std::vector<std::string> parts;
    auto current = str.begin();

    while (current != str.end()) {
        current = std::find_if(current, str.end(), [](unsigned char ch) {
            return !std::isspace(ch);
        });

        const auto next = std::find_if(current, str.end(), [](unsigned char ch) {
            return std::isspace(ch);
        });

        if (current != next) {
            parts.emplace_back(current, next);
        }

        current = next;
    }

    return parts;
}

}

DebuggerController::DebuggerController(Machine& machine) :
    machine(machine)
{
}

void DebuggerController::reset()
{
    last_command.clear();
    last_address.reset();
}

std::optional<uint16_t> DebuggerController::string_to_word(const std::string& text) const
{
    std::string_view value = text;
    if (value.starts_with('$')) {
        value.remove_prefix(1);
    }
    else if (value.starts_with("0x") || value.starts_with("0X")) {
        value.remove_prefix(2);
    }

    if (value.empty()) {
        return std::nullopt;
    }

    uint32_t parsed = 0;
    const auto result = std::from_chars(value.data(), value.data() + value.size(), parsed, 16);
    if (result.ec != std::errc{} || result.ptr != value.data() + value.size() || parsed > std::numeric_limits<uint16_t>::max()) {
        return std::nullopt;
    }

    return static_cast<uint16_t>(parsed);
}

std::optional<size_t> DebuggerController::string_to_count(const std::string& text) const
{
    size_t parsed = 0;
    const auto result = std::from_chars(text.data(), text.data() + text.size(), parsed, 10);
    constexpr size_t max_step_count = 1'000'000;
    if (result.ec != std::errc{} || result.ptr != text.data() + text.size() || parsed == 0 || parsed > max_step_count) {
        return std::nullopt;
    }

    return parsed;
}

std::string DebuggerController::help_text() const
{
    return
        "Available monitor commands:\n\n"
        "ay              : print AY-3-8912 sound chip info\n"
        "b, break        : break execution\n"
        "bs <address>    : set breakpoint at hexadecimal address\n"
        "bl              : list breakpoints\n"
        "bc <address>    : clear breakpoint\n"
        "bc *            : clear all breakpoints\n"
        "d               : disassemble from last address or PC\n"
        "d <address> <n> : disassemble n hexadecimal bytes\n"
        "                  (example: d c000 10, or d $c000 $10)\n"
        "debug           : show debug output at run time\n"
        "g [address]     : go (continue), optionally setting PC\n"
        "h, help         : show this help\n"
        "i, regs         : print machine info and registers\n"
        "m <address> <n> : dump n hexadecimal bytes from address\n"
        "                  (example: m 1f00 20)\n"
        "pc <address>    : set program counter to address\n"
        "quiet           : prevent debug output at run time\n"
        "q, quit         : quit\n"
        "s [n]           : step one or n decimal instructions\n"
        "sr, softreset   : soft reset oric\n"
        "v               : print VIA (6522) info\n";
}

std::string DebuggerController::step(size_t count)
{
    std::ostringstream out;

    for (size_t i = 0; i < count; ++i) {
        bool brk = false;
        machine.cpu->time_instruction();
        while (!machine.cpu->exec(false, brk, true)) {}
        if (brk) {
            out << "Instruction BRK executed.\n";
            break;
        }
    }

    out << machine.format_stat();
    return out.str();
}

DebuggerController::Result DebuggerController::execute(std::string command_line)
{
    trim(command_line);

    if (command_line.empty()) {
        if (last_command.empty()) {
            return {Action::Stay, ""};
        }
        command_line = last_command;
    }
    else {
        last_command = command_line;
    }

    std::vector<std::string> parts = split_words(command_line);
    const std::string& cmd = parts[0];

    if (cmd == "h" || cmd == "help") {
        return {Action::Stay, help_text()};
    }
    if (cmd == "ay") {
        return {Action::Stay, machine.ay3->status_string()};
    }
    if (cmd == "b" || cmd == "break") {
        return {Action::Break, "Exec break\n"};
    }
    if (cmd == "bs") {
        if (parts.size() != 2) {
            return {Action::Stay, "Error: missing address\n"};
        }

        const auto addr = string_to_word(parts[1]);
        if (!addr) {
            return {Action::Stay, std::format("Error: invalid address \"{}\"\n", parts[1])};
        }

        const bool inserted = machine.cpu->set_breakpoint(*addr);
        return {Action::Stay, inserted
            ? std::format("Set breakpoint at ${:04X}\n", *addr)
            : std::format("Breakpoint already exists at ${:04X}\n", *addr)};
    }
    if (cmd == "bl") {
        if (parts.size() != 1) {
            return {Action::Stay, "Use: bl\n"};
        }

        const auto& breakpoints = machine.cpu->get_breakpoints();
        if (breakpoints.empty()) {
            return {Action::Stay, "No breakpoints set\n"};
        }

        std::ostringstream output;
        output << "Breakpoints:\n";
        for (const uint16_t address : breakpoints) {
            output << std::format("  ${:04X}\n", address);
        }

        return {Action::Stay, output.str()};
    }
    if (cmd == "bc") {
        if (parts.size() != 2) {
            return {Action::Stay, "Use: bc <address> or bc *\n"};
        }

        if (parts[1] == "*") {
            machine.cpu->clear_breakpoints();
            return {Action::Stay, "Cleared all breakpoints\n"};
        }

        const auto addr = string_to_word(parts[1]);
        if (!addr) {
            return {Action::Stay, std::format("Error: invalid address \"{}\"\n", parts[1])};
        }

        if (!machine.cpu->clear_breakpoint(*addr)) {
            return {Action::Stay, std::format("No breakpoint at ${:04X}\n", *addr)};
        }

        return {Action::Stay, std::format("Cleared breakpoint at ${:04X}\n", *addr)};
    }
    if (cmd == "d" || cmd == "disassemble") {
        if (parts.size() == 1) {
            const uint16_t addr = last_address.value_or(machine.cpu->get_pc());
            auto result = machine.get_monitor().disassemble_to_string(addr, 30);
            last_address = result.next_address;
            return {Action::Stay, result.output};
        }
        if (parts.size() != 3) {
            return {Action::Stay, "Use: d <start address> <length>\n"};
        }

        const auto addr = string_to_word(parts[1]);
        const auto length = string_to_word(parts[2]);
        if (!addr || !length || *length == 0) {
            return {Action::Stay, "Error: address and length must be valid non-zero hexadecimal values\n"};
        }

        auto result = machine.get_monitor().disassemble_to_string(*addr, *length);
        last_address = result.next_address;
        return {Action::Stay, result.output};
    }
    if (cmd == "debug") {
        machine.set_disassemble_execution(true);
        return {Action::Stay, "Debug mode enabled\n"};
    }
    if (cmd == "g" || cmd == "go" || cmd == "continue") {
        if (parts.size() > 2) {
            return {Action::Stay, "Use: g [address]\n"};
        }

        if (parts.size() == 2) {
            const auto addr = string_to_word(parts[1]);
            if (!addr) {
                return {Action::Stay, std::format("Error: invalid address \"{}\"\n", parts[1])};
            }
            machine.cpu->set_pc(*addr);
        }

        return {Action::Continue, ""};
    }
    if (cmd == "i" || cmd == "info" || cmd == "regs" || cmd == "registers") {
        if (parts.size() != 1) {
            return {Action::Stay, "Use: i\n"};
        }

        return {Action::Stay, std::format("PC: ${:04X}\n{}", machine.cpu->get_pc(), machine.format_stat())};
    }
    if (cmd == "m" || cmd == "memory") {
        if (parts.size() != 3) {
            return {Action::Stay, "Use: m <start address> <length>\n"};
        }

        const auto addr = string_to_word(parts[1]);
        const auto length = string_to_word(parts[2]);
        if (!addr || !length || *length == 0 || static_cast<uint32_t>(*addr) + *length > machine.memory.get_size()) {
            return {Action::Stay, "Error: address and length must describe a range inside memory\n"};
        }
        return {Action::Stay, machine.memory.dump_string(*addr, *length)};
    }
    if (cmd == "pc") {
        if (parts.size() != 2) {
            return {Action::Stay, "Error: missing address\n"};
        }

        const auto addr = string_to_word(parts[1]);
        if (!addr) {
            return {Action::Stay, std::format("Error: invalid address \"{}\"\n", parts[1])};
        }

        machine.cpu->set_pc(*addr);
        return {Action::Stay, machine.format_stat()};
    }
    if (cmd == "q" || cmd == "quit") {
        if (parts.size() != 1) {
            return {Action::Stay, "Use: q\n"};
        }

        return {Action::Quit, "quit\n"};
    }
    if (cmd == "quiet") {
        machine.set_disassemble_execution(false);
        return {Action::Stay, "Quiet mode enabled\n"};
    }
    if (cmd == "s" || cmd == "step") {
        if (parts.size() > 2) {
            return {Action::Stay, "Use: s [count]\n"};
        }

        const auto count = parts.size() == 2 ? string_to_count(parts[1]) : std::optional<size_t>{1};
        if (!count) {
            return {Action::Stay, "Error: step count must be a positive decimal number (maximum 1000000)\n"};
        }

        return {Action::Stay, step(*count)};
    }
    if (cmd == "sr" || cmd == "softreset") {
        machine.cpu->NMI();
        return {Action::Stay, "NMI triggered\n"};
    }
    if (cmd == "v") {
        return {Action::Stay, machine.mos_6522->get_state().to_string()};
    }

    return {Action::Stay, std::format("Unknown command \"{}\". Use command \"h\" to get help.\n", cmd)};
}
