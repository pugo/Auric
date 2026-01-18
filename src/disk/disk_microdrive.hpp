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

#ifndef DISK_MICRODRIVE_H
#define DISK_MICRODRIVE_H

#include "chip/wd1793.hpp"
#include "disk.hpp"

class Machine;

class DiskMicrodrive : public Disk
{
public:
    explicit DiskMicrodrive(Machine& machine);

    /**
     * Initialize disk.
     * @return true on success
     */
    bool init() override;

    /**
     * Reset disk.
     */
    void reset() override;

    /**
     * Print disk status to console.
     */
    void print_stat() override;

    /**
     * Execute one cycle.
     */
    void exec() override;


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
};

#endif // DISK_MICRODRIVE_H
