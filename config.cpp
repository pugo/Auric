// =========================================================================
//   Copyright (C) 2009-2024 by Anders Piniesjö <pugo@pugo.org>
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

#include <sstream>
#include <cstdlib>
#include <vector>
#include <string>

#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>

#include "oric.hpp"
#include "memory.hpp"
#include "frontends/sdl/frontend.hpp"

namespace po = boost::program_options;


Config::Config() :
    _start_in_monitor(false),
    _use_atmos_rom(false)
{
}


bool Config::parse(int argc, char **argv)
{
    try {
        po::options_description desc("Allowed options");

        desc.add_options()
            ("help,?", "produce help message")
            ("monitor,m", po::bool_switch(&_start_in_monitor), "start in monitor mode")
            ("atmos,a", po::bool_switch(&_use_atmos_rom), "use Atmos ROM")
            ("tape,t", po::value<std::filesystem::path>(&_tape_path), "Tape file to use");

        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).options(desc).run(), vm);

        if (vm.count("help")) {
            std::cout << "Usage: oric [options]" << std::endl << desc;
            return false;
        }

        po::notify(vm);
    }
    catch(std::exception& e)
    {
        std::cout << e.what() << std::endl;
        return false;
    }
    return true;
}

