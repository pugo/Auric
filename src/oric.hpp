// =========================================================================
//   Copyright (C) 2009-2025 by Anders Piniesjö <pugo@pugo.org>
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

#ifndef ORIC_H
#define ORIC_H

#include <memory>
#include <filesystem>

#include "machine.hpp"
#include "config.hpp"

class Frontend;

class Oric
{
public:
    enum State
    {
        STATE_RUN,
        STATE_MON,
        STATE_QUIT
    };

    explicit Oric(Config& config);
    ~Oric();

    /**
     * Initialize Oric.
     */
    void init();

    /**
     * Initialize Machine.
     */
    void init_machine();

    /**
     * Get current config.
     * @return reference to config
     */
    Config& get_config() { return config; }

    /**
     * Ger current machine.
     * @return reference to machine
     */
    Machine& get_machine() { return *machine; }

    /**
     * Run Oric.
     */
    void run();

    /**
     * Break Oric.
     */
    void do_break();

    /**
     * Quit Oric.
     */
    void do_quit();

protected:
    State handle_command(std::string& command_line);
    uint16_t string_to_word(std::string& addr);

    Config& config;
    State state;
    std::unique_ptr<Frontend> frontend;
    std::unique_ptr<Machine> machine;
    std::string last_command;
    uint16_t last_address;
};

#endif // ORIC_H
