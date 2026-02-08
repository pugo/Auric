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

#ifndef CHIP_WD1793_H
#define CHIP_WD1793_H

#include <cstdint>
#include <string>

#include <disk/disk_image.hpp>

class Drive;
class Machine;
class Memory;
class Snapshot;
class WD1793;

class Operation
{
public:
    Operation(WD1793& wd1793) : wd1793(wd1793) { printf("HEEEEJ!\n"); }
    virtual ~Operation() = default;

    virtual uint8_t read_data_reg() const = 0;
    virtual void write_data_reg(uint8_t value) = 0;

protected:
    WD1793& wd1793;
};


class OperationIdle : public Operation
{
public:
    OperationIdle(WD1793& wd1793) : Operation(wd1793) {}
    uint8_t read_data_reg() const override;
    void write_data_reg(uint8_t value) override;
};


class OperationReadSector : public Operation
{
public:
    OperationReadSector(WD1793& wd1793) : Operation(wd1793), multiple_sectors(false) {}

    uint8_t read_data_reg() const override;
    void write_data_reg(uint8_t value) override;

    bool multiple_sectors;
};


class OperationWriteSector : public Operation
{
public:
    OperationWriteSector(WD1793& wd1793) : Operation(wd1793), multiple_sectors(false) {}

    uint8_t read_data_reg() const override;
    void write_data_reg(uint8_t value) override;

    bool multiple_sectors;
};


class OperationReadAddress : public Operation
{
public:
    OperationReadAddress(WD1793& wd1793) : Operation(wd1793) {}
    uint8_t read_data_reg() const override;
    void write_data_reg(uint8_t value) override;
};


class OperationReadTrack : public Operation
{
public:
    OperationReadTrack(WD1793& wd1793) : Operation(wd1793) {}
    uint8_t read_data_reg() const override;
    void write_data_reg(uint8_t value) override;
};


class OperationWriteTrack : public Operation
{
public:
    OperationWriteTrack(WD1793& wd1793) : Operation(wd1793) {}
    uint8_t read_data_reg() const override;
    void write_data_reg(uint8_t value) override;
};



class WD1793
{
public:
    enum Status : uint8_t {
        StatusBusy = 0x01,                 // bit 0: Type I, II and III
        StatusIndex = 0x02,                // bit 1: Type I
        StatusDataRequest = 0x02,          // bit 1: Type II and III
        StatusTrack00 = 0x04,              // bit 2: Type I
        StatusLostData = 0x04,             // bit 2: Type II and III
        StatusCrcError = 0x08,             // bit 3: Type I, II and III
        StatusSeekError = 0x10,            // bit 4: Type I
        StatusRecordNotFound = 0x10,       // bit 4: Type II and III
        StatusHeadLoaded = 0x20,           // bit 5: Type I
        StatusRecordType = 0x20,           // bit 5: Type II and III
        StatusWriteFault = 0x20,           // bit 5: Type II and III
        StatusProtected = 0x40,            // bit 6: Type I
        StatusWriteProtect = 0x40,         // bit 6: Type II and III
        StatusNotReady = 0x80              // bit 7: Type I, II and III
    };

    /**
     * State of a WD1793.
     */
    struct State
    {
        Operation* current_operation;

        // Registers.
        unsigned char data;
        uint8_t drive;
        uint8_t side;
        uint8_t track;                   // Desired track, not the current_track.
        uint8_t sector;                  // Desired sector, not the current_sector.
        uint8_t command;
        uint8_t status;

        uint8_t current_track_number;
        uint8_t current_sector_number;
        uint8_t sector_type;

        int16_t interrupt_counter;       // Counts down cycles to interrupt.
        uint8_t status_at_interrupt;     // Status value at interrupt time.
        bool update_status_at_interrupt; // Whether to update status_at_interrupt on next interrupt.

        int16_t data_request_counter;    // Counts down cycles to data request.

        DiskTrack* current_track;
        DiskSector* current_sector;
        uint16_t offset;                 // Position within current sector data.

        void reset();

        void set_status_at_interrupt(uint8_t status)
        {
            status_at_interrupt = status;
            update_status_at_interrupt = true;
        }

        void print() const;
    };

    explicit WD1793(Machine& a_Machine, Drive* drive);
    ~WD1793() = default;

    /**
     * Execute one clock cycle.
     */
    void exec(uint8_t cycles);

    void set_drive_number(uint8_t drive) { state.drive = drive; }
    void set_side_number(uint8_t side) { state.side = side; }

    /**
     * Save WD1793 state to snapshot.
     * @param snapshot reference to snapshot
     */
    void save_to_snapshot(Snapshot& snapshot);

    /**
     * Load WD1793 state from snapshot.
     * @param snapshot reference to snapshot
     */
    void load_from_snapshot(Snapshot& snapshot);

    /**
     * Read register value.
     * @param offset register to read
     * @return value of register
     */
    uint8_t read_byte(uint16_t offset);

    /**
     * Write register value.
     * @param offset register to write
     * @param value new value
     */
    void write_byte(uint16_t offset, uint8_t value);

    /**
     * Return reference ot current WD1793 state.
     * @return reference to current WD1793 state
     */
    WD1793::State& get_state() { return state; }

    friend class OperationIdle;
    friend class OperationReadSector;
    friend class OperationWriteSector;
    friend class OperationReadAddress;
    friend class OperationReadTrack;
    friend class OperationWriteTrack;

private:
    void do_command(uint8_t command);

    bool set_track(uint8_t track);
    bool set_sector(uint8_t sector);

    Machine& machine;
    Drive* drive;
    State state;

    OperationIdle operation_idle;
    OperationReadSector operation_read_sector;
    OperationWriteSector operation_write_sector;
    OperationReadAddress operation_read_address;
    OperationReadTrack operation_read_track;
    OperationWriteTrack operation_write_track;
};


#endif // CHIP_WD1793_H
