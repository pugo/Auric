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

#include <iostream>
#include <print>
#include <string>
#include <vector>

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include "oric.hpp"

#ifndef AURIC_VERSION
#define AURIC_VERSION "0.0.0"
#endif


Config::Config() :
    parsed_arguments("auric", AURIC_VERSION, argparse::default_arguments::none),
    _config_path{"auric.yaml"},
    _start_in_monitor{false},
    _use_oric1_rom{false},
    _window_width{1024},
    _window_height{768},
    _zoom{3},
    _verbose{false},
    _roms_path{"./ROMS"},
    _rom_names{{RomType::Oric1, "basic10.rom"},
               {RomType::OricAtmos, "basic11b.rom"},
               {RomType::Microdisk, "microdis.rom"}},
    _fonts_path{"./fonts"},
    _images_path{"./images"},
    _snapshots_path{"./snapshots"},
    _enable_scanlines{true},
    _enable_vertical_lines{true},
    _enable_vignette{true},
    _vignette_strength{0.2f},
    _tape_turbo_enabled{true},
    _tape_autostart_enabled{true}
{
}


bool Config::parse_command_line(int argc, char **argv)
{
    try {
        parsed_arguments.add_argument("-?", "--help").help("produce help message").default_value(false).implicit_value(true);
        parsed_arguments.add_argument("-c", "--config").help("path to configuration file");

        parsed_arguments.add_argument("-1", "--oric1").help("use Oric 1 mode (default: Atmos mode)").default_value(false).implicit_value(true);
        parsed_arguments.add_argument("-w", "--width").help("window width in pixels").scan<'i', uint16_t>();
        parsed_arguments.add_argument("-h", "--height").help("window height in pixels").scan<'i', uint16_t>();

        parsed_arguments.add_argument("-t", "--tape").help("tape image file to use");
        parsed_arguments.add_argument("--tape-autostart-off").help("turn off tape autostart").default_value(false).implicit_value(true);
        parsed_arguments.add_argument("--tape-turbo-off").help("turn off tape turbo mode").default_value(false).implicit_value(true);

        parsed_arguments.add_argument("-d", "--disk1").help("disk image file to use for drive 1");
        parsed_arguments.add_argument("--disk2").help("disk image file to use for drive 2");
        parsed_arguments.add_argument("--disk3").help("disk image file to use for drive 3");
        parsed_arguments.add_argument("--disk4").help("disk image file to use for drive 4");

        parsed_arguments.add_argument("-m", "--monitor").help("start with GUI debugger open").default_value(false).implicit_value(true);
        parsed_arguments.add_argument("-v", "--verbose").help("verbose output").default_value(false).implicit_value(true);

        std::vector<std::string> args(argv, argv + argc);
        for (auto& arg : args) {
            if (arg == "-1") {
                arg = "--oric1";
            }
        }

        parsed_arguments.parse_args(args);

        if (parsed_arguments.get<bool>("--help")) {
            std::cout << parsed_arguments;
            return false;
        }

        if (auto cp = parsed_arguments.present<std::string>("--config")) {
            _config_path = *cp;
        }
    }
    catch(std::exception& e)
    {
        std::println("Argument error: {}", e.what());
        return false;
    }

    return true;
}


void Config::apply_command_line()
{
    _use_oric1_rom = parsed_arguments.get<bool>("--oric1");
    _start_in_monitor = parsed_arguments.get<bool>("--monitor");
    _verbose = parsed_arguments.get<bool>("--verbose");

    if (auto width = parsed_arguments.present<uint16_t>("--width")) {
        _window_width = *width;
    }
    if (auto height = parsed_arguments.present<uint16_t>("--height")) {
        _window_height = *height;
    }

    if (parsed_arguments.get<bool>("--tape-turbo-off")) {
        _tape_turbo_enabled = false;
    }

    if (parsed_arguments.get<bool>("--tape-autostart-off")) {
        _tape_autostart_enabled = false;
    }

    if (auto tape = parsed_arguments.present<std::string>("--tape")) {
        _tape_path = *tape;
    }

    if (auto disk1 = parsed_arguments.present<std::string>("--disk1")) {
        _disk_paths[0] = *disk1;
    }
    if (auto disk2 = parsed_arguments.present<std::string>("--disk2")) {
        _disk_paths[1] = *disk2;
    }
    if (auto disk3 = parsed_arguments.present<std::string>("--disk3")) {
        _disk_paths[2] = *disk3;
    }
    if (auto disk4 = parsed_arguments.present<std::string>("--disk4")) {
        _disk_paths[3] = *disk4;
    }

    const bool has_disk = ! _disk_paths[0].empty() ||
                          ! _disk_paths[1].empty() ||
                          ! _disk_paths[2].empty() ||
                          ! _disk_paths[3].empty();

    if (! parsed_arguments.present<std::string>("--tape") ||
        _start_in_monitor || has_disk) {
        _tape_autostart_enabled = false;
    }

    auto logger = spdlog::get("auric");
    if (!logger) {
        logger = spdlog::stderr_color_mt("auric");
    }
    logger->set_pattern("%^[%l]  %v%$");
    spdlog::set_default_logger(logger);
    spdlog::set_level(_verbose ? spdlog::level::debug : spdlog::level::info);
}


void Config::read_config_file(std::filesystem::path config_path)
{
    YAML::Node yaml_config;
    try {
        yaml_config = YAML::LoadFile(config_path.string());
    }
    catch (YAML::Exception& e) {
        throw std::runtime_error(std::format("Failed to parse config file '{}':\n\n{}", config_path.string(), e.what()));
    }

    if (yaml_config["roms"]["roms_directory"]) {
        _roms_path = yaml_config["roms"]["roms_directory"].as<std::string>();

        if (!is_directory(_roms_path)) {
            throw std::runtime_error(std::format("ROMs directory '{}' is not a directory.", _roms_path.string()));
        }

        _rom_names[RomType::Oric1] = yaml_config["roms"]["file_names"]["oric_1"].as<std::string>();
        _rom_names[RomType::OricAtmos] = yaml_config["roms"]["file_names"]["oric_atmos"].as<std::string>();
        _rom_names[RomType::Microdisk] = yaml_config["roms"]["file_names"]["microdisk"].as<std::string>();

        std::println("Found roms directory: {}", _roms_path.string());
    }

    if (yaml_config["media"]["fonts_path"]) {
        _fonts_path = yaml_config["media"]["fonts_path"].as<std::string>();
    }

    if (yaml_config["media"]["images_path"]) {
        _images_path = yaml_config["media"]["images_path"].as<std::string>();
    }

    if (yaml_config["media"]["snapshots_path"]) {
        _snapshots_path = yaml_config["media"]["snapshots_path"].as<std::string>();
    }

    if (yaml_config["video"]) {
        if (yaml_config["video"]["window_width"]) {
            _window_width = yaml_config["video"]["window_width"].as<uint16_t>();
        }

        if (yaml_config["video"]["window_height"]) {
            _window_height = yaml_config["video"]["window_height"].as<uint16_t>();
        }

        if (yaml_config["video"]["enable_scanlines"]) {
            _enable_scanlines = yaml_config["video"]["enable_scanlines"].as<bool>();
        }

        if (yaml_config["video"]["enable_vertical_lines"]) {
            _enable_vertical_lines = yaml_config["video"]["enable_vertical_lines"].as<bool>();
        }

        if (yaml_config["video"]["enable_vignette"]) {
            _enable_vignette = yaml_config["video"]["enable_vignette"].as<bool>();
        }

        if (yaml_config["video"]["vignette_strength"]) {
            float vignette_arg = yaml_config["video"]["vignette_strength"].as<float>();
            vignette_arg = std::clamp<float>(vignette_arg, 0, 1);
            _vignette_strength = vignette_arg;
        }
    }

    if (yaml_config["tape"]) {
        if (yaml_config["tape"]["turbo_load_enabled"]) {
            _tape_turbo_enabled = yaml_config["tape"]["turbo_load_enabled"].as<bool>();
        }

        if (yaml_config["tape"]["autostart_enabled"]) {
            _tape_autostart_enabled = yaml_config["tape"]["autostart_enabled"].as<bool>();
        }
    }
}
