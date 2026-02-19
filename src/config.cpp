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

#include <print>

#include <boost/log/trivial.hpp>
#include <boost/log/core.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/expressions.hpp>
#include <boost/program_options.hpp>

#include "oric.hpp"

namespace po = boost::program_options;
namespace logging = boost::log;


static const char* severity_color(logging::trivial::severity_level lvl) {
    switch (lvl) {
        case logging::trivial::trace:   return "\033[37m";
        case logging::trivial::debug:   return "\033[36m";
        case logging::trivial::info:    return "\033[32m";
        case logging::trivial::warning: return "\033[33m";
        case logging::trivial::error:   return "\033[31m";
        case logging::trivial::fatal:   return "\033[41m";
    }
    return "\033[0m";
}


Config::Config() :
    _start_in_monitor{false},
    _use_oric1_rom{false},
    _zoom{3},
    _verbose{false},
    _roms_path{"./ROMS"},
    _rom_names{{RomType::Oric1, "basic10.rom"},
               {RomType::OricAtmos, "basic11b.roms"},
               {RomType::Microdisk, "microdis.rom"}},
    _fonts_path{"./fonts"},
    _images_path{"./images"}
{
}


bool Config::parse(int argc, char **argv)
{
    try {
        po::options_description desc("Allowed options");

        int zoom_arg;

        desc.add_options()
            ("help,?", "produce help message")
            ("zoom,z", po::value<int>(&zoom_arg), "window zoom 1-10")
            ("monitor,m", po::bool_switch(&_start_in_monitor), "start in monitor mode")
            ("oric1,1", po::bool_switch(&_use_oric1_rom), "use Oric 1 mode (default: Atmos mode)")
            ("disk,d", po::value<std::filesystem::path>(&_disk_path), "disk image file to use")
            ("tape,t", po::value<std::filesystem::path>(&_tape_path), "tape image file to use")
            ("verbose,v", po::bool_switch(&_verbose), "verbose output");

        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).options(desc).run(), vm);
        po::notify(vm);

        if (vm.count("help")) {
            std::println("Usage: oric [options]\n");
            desc.print(std::cout);
            return false;
        }

        // Read zoom only if provided.
        if (!vm["zoom"].empty()) {
            zoom_arg = std::clamp<int>(zoom_arg, 1, 10);  // Clamp zoom to 1-10.
            _zoom = static_cast<uint8_t>(zoom_arg);
        }

        if (_verbose) {
            boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::debug);
        }
        else {
            boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::info);
        }

        // Format logging output.
        auto sink = boost::log::add_console_log(std::clog);

        sink->set_formatter([](const logging::record_view& rec, logging::formatting_ostream& strm) {
            auto lvl = rec[logging::trivial::severity];
            if (lvl) {
                strm << severity_color(*lvl) << "[" << *lvl << "]  ";
            }
            strm << rec[boost::log::expressions::smessage] << "\033[0m";
        });
    }
    catch(std::exception& e)
    {
        std::println("Argument error: {}", e.what());
        return false;
    }
    return true;
}



bool Config::read_config_file(std::filesystem::path config_path)
{
    YAML::Node yaml_config;
    try {
        yaml_config = YAML::LoadFile(config_path);
    }
    catch (YAML::Exception& e) {
        std::println("Error parsing config file: {}", e.what());
        return false;
    }

    if (yaml_config["foo"]) {
        std::println("Found foo in config file");
    }
    else {
        std::println("No foo in config file");
    }

    if (yaml_config["roms"]["roms_directory"]) {
        _roms_path = yaml_config["roms"]["roms_directory"].as<std::string>();

        if (!is_directory(_roms_path)) {
            std::println("ROMs directory '{}' is not a directory", _roms_path.string());
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

    if (yaml_config["video"]["zoom"]) {
        int zoom_arg = yaml_config["video"]["zoom"].as<int>();
        zoom_arg = std::clamp<int>(zoom_arg, 1, 10);
        _zoom = static_cast<uint8_t>(zoom_arg);
    }

    return true;
}