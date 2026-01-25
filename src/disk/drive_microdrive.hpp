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

#ifndef DRIVE_MICRODRIVE_H
#define DRIVE_MICRODRIVE_H

#include <filesystem>

#include "chip/wd1793.hpp"
#include "drive.hpp"
#include "disk_image.hpp"

class Machine;

class DriveMicrodrive : public Drive
{
public:
    explicit DriveMicrodrive(Machine& machine);

    /**
     * Initialize drive.
     * @return true on success
     */
    bool init() override;

    /**
     * Reset drive.
     */
    void reset() override;

    /**
     * Insert disk image.
     * @param path path to disk image
     * @return true on success
     */
    bool insert_disk(const std::filesystem::path& path) override;

    /**
     * Print drive status to console.
     */
    void print_stat() override;

    /**
     * Execute one cycle.
     */
    void exec(uint8_t cycles) override;

    /**
     * Read register value.
     * @param offset register to read
     * @return value of register
     */
    uint8_t read_byte(uint16_t offset) override;

    /**
     * Write register value.
     * @param offset register to write
     * @param value new value
     */
    void write_byte(uint16_t offset, uint8_t value) override;

protected:
    Machine& machine;
    WD1793 wd1793;

    uint8_t status;


    std::filesystem::path disk_image_path;
    std::shared_ptr<DiskImage> disk_image;
};

#endif // DRIVE_MICRODRIVE_H
