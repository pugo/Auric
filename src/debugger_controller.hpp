// =========================================================================
//   Copyright (C) 2009-2026 by Anders Piniesjö <pugo@pugo.org>
//
//   This program is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
// =========================================================================

#ifndef DEBUGGER_CONTROLLER_H
#define DEBUGGER_CONTROLLER_H

#include <cstdint>
#include <optional>
#include <string>

class Machine;

class DebuggerController
{
public:
    enum class Action
    {
        Stay,
        Break,
        Continue,
        Quit
    };

    struct Result
    {
        Action action{Action::Stay};
        std::string output;
    };

    explicit DebuggerController(Machine& machine);

    void reset();
    Result execute(std::string command_line);

private:
    std::optional<uint16_t> string_to_word(const std::string& addr) const;
    std::optional<size_t> string_to_count(const std::string& count) const;
    std::string help_text() const;
    std::string step(size_t count);

    Machine& machine;
    std::string last_command;
    std::optional<uint16_t> last_address;
};

#endif // DEBUGGER_CONTROLLER_H
