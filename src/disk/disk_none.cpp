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

#include "disk_none.hpp"


bool DiskNone::init()
{
    return true;
}

void DiskNone::reset()
{}

void DiskNone::print_stat()
{
    std::println("Disk None");;
}

void DiskNone::exec()
{}

uint8_t DiskNone::read_byte(uint16_t offset)
{
    return 0x00;
}

void DiskNone::write_byte(uint16_t offset, uint8_t value)
{}
