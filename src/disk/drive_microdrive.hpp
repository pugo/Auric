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
    enum MicroDriveStatus : uint8_t {
        MdInterruptEnabled = 0x01,          // Bit 0
        MdBasicRomEnabled = 0x02,           // Bit 1
        MdDataSeparatorClockDivisor = 0x04, // Bit 2
        MdDensityFlag = 0x08,               // Bit 3
        MdSideControl = 0x10,               // Bit 4
        MdDriveNumber = 0x60,               // Bits 5-6
        MdRomEnable = 0x80                  // Bit 7
    };

    struct State
    {
        uint8_t status;
        uint8_t interrupt_request;
        uint8_t data_request;

        void reset();
    };

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
     * Get disk image.
     * @return reference to disk image
     */
    DiskImage* get_disk_image() override;

    /**
     * Print drive status to console.
     */
    void print_stat() override;

    /**
     * Execute one cycle.
     */
    void exec(uint8_t cycles) override;

    /**
     * Set interrupt request. Sets CPU interrupt flag if interrupts are enabled in status.
     */
    void interrupt_set() override;

    /**
     * Clear interrupt request. Clears CPU interrupt flag.
     */
    void interrupt_clear() override;

    /**
     * Set data request flag.
     */
    void data_request_set() override;

    /**
     * Clear data request flag.
     */
    void data_request_clear() override;

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

    /**
     * Save DriveMicrodrive state to snapshot.
     * @param snapshot reference to snapshot
     */
    void save_to_snapshot(Snapshot& snapshot) override;

    /**
     * Load DriveMicrodrive state from snapshot.
     * @param snapshot reference to snapshot
     */
    void load_from_snapshot(Snapshot& snapshot) override;

protected:
    Machine& machine;
    WD1793 wd1793;

    State state;

    std::filesystem::path disk_image_path;
    std::unique_ptr<DiskImage> disk_image;
};

#endif // DRIVE_MICRODRIVE_H
