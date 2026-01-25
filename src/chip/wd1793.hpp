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
#include <map>
#include <string>

class Machine;
class Memory;
class Snapshot;


class WD1793
{
public:
    enum Status : uint8_t {
        StatusBusy = 0x01,                // bit 0: Type I, II and III
        StatusIndex = 0x02,               // bit 1: Type I
        StatusDataRequest = 0x02,         // bit 1: Type II and III
        StatusTrack00 = 0x04,             // bit 2: Type I
        StatusLostData = 0x04,            // bit 2: Type II and III
        StatusCrcError = 0x08,            // bit 3: Type I, II and III
        StatusSeekError = 0x10,           // bit 4: Type I
        StatusRecordNotFound = 0x10,      // bit 4: Type II and III
        StatusHeadLoaded = 0x20,          // bit 5: Type I
        StatusRecordTypeWiteFault = 0x20, // bit 5: Type II and III
        StatusProtected = 0x40,           // bit 6: Type I
        StatusWriteProtect = 0x40,        // bit 6: Type II and III
        StatusNotReady = 0x80             // bit 7: Type I, II and III
    };


    /**
     * State of a WD1793.
     */
    struct State
    {
        // Registers.
        unsigned char data;
        uint8_t drive;
        uint8_t side;
        uint8_t track;
        uint8_t sector;
        uint8_t command;
        uint8_t status;

        bool interrupts_enabled;       // Set by bit 1 in the FDC control register to enable or disable CPU IRQs.

        int16_t interrupt_counter;     // Counts down cycles to interrupt.
        bool irq_flag;                 // Interrupt request flag.

        int16_t data_request_counter;  // Counts down cycles to data request.
        bool data_request_flag;        // Set when data is available on read or missing on write.

        void reset();
        void print() const;
    };

    explicit WD1793(Machine& a_Machine);
    ~WD1793() = default;

    /**
     * Execute one clock cycle.
     */
    void exec(uint8_t cycles);

    void set_interrupts_enabled(bool enabled) { state.interrupts_enabled = enabled; }
    void set_drive(uint8_t drive) { state.drive = drive; }
    void set_side(uint8_t side) { state.side = side; }

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


private:
    void do_command(uint8_t command);

    void interrupt_set();
    void interrupt_clear();

    void data_request_set();
    void data_request_clear();

    Machine& machine;
    WD1793::State state;
};


#endif // CHIP_WD1793_H
