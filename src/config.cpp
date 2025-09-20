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

#include <unistd.h>
#include <signal.h>

#include <algorithm>
#include <cstdlib>
#include <print>
#include <vector>
#include <sstream>
#include <string>

#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>

#include "oric.hpp"
#include "memory.hpp"
#include "frontends/sdl/frontend.hpp"

namespace po = boost::program_options;


Config::Config() :
    _start_in_monitor(false),
    _use_oric1_rom(false)
{
}


bool Config::parse(int argc, char **argv)
{
    try {
        po::options_description desc("Allowed options");

        int zoom_arg;

        desc.add_options()
            ("help,?", "produce help message")
            ("zoom,z", po::value<int>(&zoom_arg)->default_value(3), "window zoom 1-10 (default: 3)")
            ("monitor,m", po::bool_switch(&_start_in_monitor), "start in monitor mode")
            ("oric1,1", po::bool_switch(&_use_oric1_rom), "use Oric 1 mode (default: Atmos mode)")
            ("tape,t", po::value<std::filesystem::path>(&_tape_path), "tape file to use");

        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).options(desc).run(), vm);
        po::notify(vm);

        if (vm.count("help")) {
            std::cout << "Usage: oric [options]" << std::endl << desc;
            return false;
        }

        zoom_arg = std::clamp<int>(zoom_arg, 1, 10);
        _zoom = static_cast<uint8_t>(zoom_arg);
        std::println("Used zoom: {}.", _zoom);

    }
    catch(std::exception& e)
    {
        std::println("Argument error: {}", e.what());
        std::cout << e.what() << std::endl;
        return false;
    }
    return true;
}

