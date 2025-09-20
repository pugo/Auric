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

#ifndef CONFIG_H
#define CONFIG_H

#include <iostream>
#include <memory>
#include <filesystem>

#include "machine.hpp"


class Config
{
public:
    Config();

    /**
     * Parse command line.
     * @param argc argc from program start
     * @param argv argv from program start
     * @return false if program should exit
     */
    bool parse(int argc, char **argv);

    /**
     * Path to tape image.
     * @return path to tape image
     */
    std::filesystem::path& tape_path() { return _tape_path; }

    /**
     * Check if emulator should start in monitor mode.
     * @return true if emulator should start in monitor mode
     */
    bool start_in_monitor() const { return _start_in_monitor; }

    /**
     * Check if emulator should start in Oric 1 mode.
     * @return true if emulator should start in Oric 1 mode
     */
    bool use_oric1_rom() const { return _use_oric1_rom; }

    /**
     * Return window zoom level.
     * @return window zoom level
     */
    uint8_t zoom() const { return _zoom; }

    /**
     * Return verbose mode.
     * @return true if verbose mode is enabled
     */
    bool verbose() const { return _verbose; }


protected:
    bool _start_in_monitor;
    bool _use_oric1_rom;
    std::filesystem::path _tape_path;
    uint8_t _zoom;
    bool _verbose;
};

#endif // CONFIG_H
