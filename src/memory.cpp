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

#include <boost/log/trivial.hpp>
#include <fcntl.h>
#include <format>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <print>
#include <stdexcept>
#include <sstream>
#include <unistd.h>

#include "memory.hpp"
#include "snapshot.hpp"


Memory::Memory(size_t size) :
    mem(nullptr),
    size(size),
    mempos(0),
    memory(size)
{
    mem = memory.data();
    std::fill(memory.begin(), memory.end(), 0x00);
}


void Memory::load(const std::filesystem::path& path, uint32_t address)
{
    BOOST_LOG_TRIVIAL(debug) << std::format("Memory: loading {} -> ${:04X}", path.string(), address);

    if (! std::filesystem::exists(path)) {
        throw(std::runtime_error(std::format("no such file: {}", path.string())));
    }

    auto file_size = std::filesystem::file_size(path);

    if (address + file_size > memory.size()) {
        throw(std::runtime_error(std::format("trying to read outside memory area.")));
    }

    auto fd = open(path.c_str(), O_RDONLY);
    if (fd < 0) {
        throw(std::runtime_error(std::format("could not open file: {}", path.string())));
    }

    std::ifstream in{path, std::ios::binary};
    in.read(reinterpret_cast<char *>(mem + address), file_size);
}


void Memory::save_to_snapshot(Snapshot& snapshot)
{
    snapshot.memory = memory;
}


void Memory::load_from_snapshot(Snapshot& snapshot)
{
    memory = snapshot.memory;
}


void Memory::show(uint32_t pos, uint32_t length) const
{
    std::println("Showing 0x{:04X} bytes from ${:x}", length, pos);
    std::ostringstream chars;

    for (uint32_t i=0; i < length; i++) {
        if ((i % 16) == 0) {
            std::cout << "    " << chars.str() << std::endl << "[" << pos + i << "] " << std::hex;
            chars.str("");
        }

        std::cout << std::setw(2) << std::setfill('0') << (unsigned int)mem[pos + i] << " ";
        if ((mem[pos + i] & 0x7f) >= 32) {
            chars << (char)(mem[pos + i] & 0x7f) << " ";
        }
        else {
            chars << "  ";
        }
    }
    std::cout << std::endl;
}


