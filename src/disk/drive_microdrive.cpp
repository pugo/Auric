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

#include "drive_microdrive.hpp"

#include <print>
#include <random>


DriveMicrodrive::DriveMicrodrive(Machine& machine) :
    machine(machine),
    wd1793(machine, this),
    status(0)
{

}

bool DriveMicrodrive::init()
{
    return true;
}

bool DriveMicrodrive::insert_disk(const std::filesystem::path& path)
{
    if (!std::filesystem::exists(path)) {
        BOOST_LOG_TRIVIAL(warning) << "Disk image not found: " << path.string();
        return false;
    }

    disk_image_path = path;
    disk_image = std::make_shared<DiskImage>(disk_image_path);
    disk_image->init();

    return true;
}

std::shared_ptr<DiskImage> DriveMicrodrive::get_disk_image()
{
    return disk_image;
}

void DriveMicrodrive::reset()
{
    status = 0;
}

void DriveMicrodrive::print_stat()
{
    std::println("Disk None");;
}

void DriveMicrodrive::exec(uint8_t cycles)
{
    wd1793.exec(cycles);
}

uint8_t DriveMicrodrive::read_byte(uint16_t offset)
{
    std::println("Microdrive read: {:04x}", offset);

    if (offset == 0x4) {
        return wd1793.get_state().irq_flag ? 0x7f : 0xff;
    }

    if (offset == 0x8) {
        uint8_t result = wd1793.get_state().data_request_flag ? 0x7f : 0xff;
        wd1793.get_state().data_request_flag = false;
        return result;
    }

    return wd1793.read_byte(offset);
}

void DriveMicrodrive::write_byte(uint16_t offset, uint8_t value)
{
    std::println("Microdrive write: {:04x} <- {:02x}", offset, value);

    if (offset == 0x4) {
        status = value;

        wd1793.set_interrupts_enabled(value & 0x01);
        wd1793.set_side(value & 0x10 >> 4);
        wd1793.set_drive(value & 0x60 >> 5);
        machine.set_oric_rom_enabled(value & 0x02);
        machine.set_diskdrive_rom_enabled(!(value & 0x80));
    }

    if (offset == 0x8) {
        wd1793.get_state().data_request_flag = false;
    }

    return wd1793.write_byte(offset, value);
}
