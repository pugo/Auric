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

#ifndef CONFIG_H
#define CONFIG_H

#include <filesystem>
#include <yaml-cpp/yaml.h>


/**
 * Enum representing different types of ROMs supported.
 */
enum class RomType
{
    Oric1,
    OricAtmos,
    Microdisk
};


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
     * Read configuration file.
     * @param config_path path to configuration file
     * @return false if program should exit
     */
    bool read_config_file(std::filesystem::path config_path);

    /**
     * Path to disk image.
     * @return path to disk image
     */
    std::filesystem::path& disk_path() { return _disk_path; }

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

    /**
     * Return ROMS directory path.
     * @return path to ROMS directory
     */
    std::filesystem::path roms_path() const { return _roms_path; }

    /**
     * Return ROM name for given `RomType`.
     * @param type type of ROM to get name for
     * @return name of requested ROM.
     */
    std::string rom_name(RomType type) const { return _rom_names.at(type); }

    /**
     * Return fonts directory path.
     * @return fonts directory path
     */
    std::filesystem::path fonts_path() const { return _fonts_path; }

    /**
     * Return images directory path.
     * @return images directory path
     */
    std::filesystem::path images_path() const { return _images_path; }

protected:
    bool _start_in_monitor;
    bool _use_oric1_rom;
    std::filesystem::path _disk_path;
    std::filesystem::path _tape_path;
    uint8_t _zoom;
    bool _verbose;

    // ROMS
    std::filesystem::path _roms_path;
    std::map<RomType, std::string> _rom_names;

    // Media
    std::filesystem::path _fonts_path;
    std::filesystem::path _images_path;
};

#endif // CONFIG_H
