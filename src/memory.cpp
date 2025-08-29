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

#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sstream>
#include <iomanip>
#include <string.h>
#include <algorithm>

#include "memory.hpp"
#include "snapshot.hpp"

#include "chip/mos6502.hpp"


Memory::Memory(uint32_t size) :
    mem(NULL),
    size(size),
    mempos(0),
    memory(size)
{
    mem = memory.data();

    std::fill(memory.begin(), memory.end(), 0x00);
}

void error_exit(std::string description)
{
    std::cout << std::endl << "!!! Error: " << description << std::endl << std::endl;
    exit(1);
}

void Memory::load(const std::string& path, uint32_t address)
{
    std::cout << "Memory: loading " << path << " -> $" << std::hex << address << std::endl;
    // stat to check existance and size
    struct stat file_info;
    if (stat(path.c_str(), &file_info)) {
        error_exit("no such file: " + path);
    }

    int file_size = file_info.st_size;

    int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0) {
        error_exit("could not open file: " + path);
    }

    ssize_t count = 0;
    int pos = address;
    uint8_t* buff = (uint8_t*)malloc(1);
    while (file_size - count > 0) {
        ssize_t result = read(fd, buff, 1);

        if (result == 0) {
            break;
        }
        if (result == -1) {
            error_exit("error reading file: " + path);
        }

        mem[pos] = *buff;
        pos += result;
        count += result;
    }

    free(buff);
    return;
}


void Memory::save_to_snapshot(Snapshot& snapshot)
{
    snapshot.memory = memory;
}


void Memory::load_from_snapshot(Snapshot& snapshot)
{
    memory = snapshot.memory;
}


void Memory::show(uint32_t pos, uint32_t length)
{
    std::cout << "Showing 0x" << length << " bytes from " << std::hex << pos << std::endl;
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


