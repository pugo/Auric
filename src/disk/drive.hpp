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

#ifndef DRIVE_H
#define DRIVE_H

#include <filesystem>

class DiskImage;


class Drive
{
public:
    virtual ~Drive() = default;

    /**
     * Initialize drive.
     * @return true on success
     */
    virtual bool init() = 0;

    /**
     * Reset drive.
     */
    virtual void reset() = 0;

    /**
     * Insert disk image.
     * @param path path to disk image
     * @return true on success
     */
    virtual bool insert_disk(const std::filesystem::path& path) = 0;

    /**
     * Get disk image.
     * @return reference to disk image
     */
    virtual std::shared_ptr<DiskImage> get_disk_image() = 0;

    /**
     * Print drive status to console.
     */
    virtual void print_stat() = 0;

    /**
     * Execute one cycle.
     */
    virtual void exec(uint8_t cycles) = 0;

    /**
     * Read register value.
     * @param offset register to read
     * @return value of register
     */
    virtual uint8_t read_byte(uint16_t offset) = 0;

    /**
     * Write register value.
     * @param offset register to write
     * @param value new value
     */
    virtual void write_byte(uint16_t offset, uint8_t value) = 0;
};

#endif // DRIVE_H
