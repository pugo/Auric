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
    enum Register {
    };

    /**
     * State of a WD1793.
     */
    struct State
    {
        // Registers.
        unsigned char data;
        uint8_t track;
        uint8_t sector;
        uint8_t command;
        uint8_t status;

        void reset();
        void print() const;
    };

    explicit WD1793(Machine& a_Machine);
    ~WD1793() = default;

    /**
     * Execute one clock cycle.
     */
    void exec();

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

    Machine& machine;
    WD1793::State state;

    std::map<Register, std::string> register_names;
};


#endif // CHIP_WD1793_H
