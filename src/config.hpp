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

#include <argparse/argparse.hpp>
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
    bool parse_command_line(int argc, char **argv);

    /**
     * Apply command line arguments to configuration.
     */
    void apply_command_line();

    /**
     * Read configuration file.
     * @param config_path path to configuration file
     */
    void read_config_file(std::filesystem::path config_path);

    /**
     * Path to configuration file.
     * @return path to configuration file
     */
    std::filesystem::path& config_path() { return _config_path; }

    /**
     * Path to disk image.
     * @return path to disk image
     * @param drive drive number (0-3)
     */
    std::filesystem::path& disk_path(uint8_t drive) { return _disk_paths[drive % 4]; }

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
     * Return window width in pixels.
     * @return window width in pixels
     */
    uint16_t window_width() const { return _window_width; }

    /**
     * Return window height in pixels.
     * @return window height in pixels
     */
    uint16_t window_height() const { return _window_height; }

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

    /**
     * Return the directory used for persistent snapshots.
     * @return snapshot directory path
     */
    std::filesystem::path snapshots_path() const { return _snapshots_path; }

    bool enable_scanlines() const { return _enable_scanlines; }
    bool enable_vertical_lines() const { return _enable_vertical_lines; }
    bool enable_vignette() const { return _enable_vignette; }

    float vignette_strength() const { return _vignette_strength; }

    bool tape_turbo_enabled() const { return _tape_turbo_enabled; }
    bool tape_autostart_enabled() const { return _tape_autostart_enabled; }

protected:
    argparse::ArgumentParser parsed_arguments;

    std::filesystem::path _config_path;
    bool _start_in_monitor;
    bool _use_oric1_rom;
    std::filesystem::path _disk_paths[4];
    std::filesystem::path _tape_path;
    uint16_t _window_width;
    uint16_t _window_height;
    uint8_t _zoom;
    bool _verbose;

    // ROMS
    std::filesystem::path _roms_path;
    std::map<RomType, std::string> _rom_names;

    // Media
    std::filesystem::path _fonts_path;
    std::filesystem::path _images_path;
    std::filesystem::path _snapshots_path;

    // Video
    bool _enable_scanlines;
    bool _enable_vertical_lines;
    bool _enable_vignette;

    float _vignette_strength;

    // Tape
    bool _tape_turbo_enabled;
    bool _tape_autostart_enabled;
};

#endif // CONFIG_H
