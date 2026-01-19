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

#include <boost/log/trivial.hpp>
#include <machine.hpp>

#include "disk_microdrive.hpp"

#include <print>
#include <random>


DiskMicrodrive::DiskMicrodrive(Machine& machine) :
    machine(machine),
    wd1793(machine)
{

}

bool DiskMicrodrive::init()
{
    return true;
}

bool DiskMicrodrive::insert_disk(const std::filesystem::path& path)
{
    if (!std::filesystem::exists(path)) {
        BOOST_LOG_TRIVIAL(warning) << "Disk image not found: " << path.string();
        return false;
    }

    disk_image_path = path;
    return true;
}

void DiskMicrodrive::reset()
{}

void DiskMicrodrive::print_stat()
{
    std::println("Disk None");;
}

void DiskMicrodrive::exec()
{}

uint8_t DiskMicrodrive::read_byte(uint16_t offset)
{
    std::println("DiskMicrodrive::read_byte");
    return wd1793.read_byte(offset);
}

void DiskMicrodrive::write_byte(uint16_t offset, uint8_t value)
{
    std::println("Microdrive write: {:04x} <- {:02x}", offset, value);

    if (offset == 0x4) {
        machine.set_oric_rom_enabled(value & 0x02);
    }

    return wd1793.write_byte(offset, value);
}
