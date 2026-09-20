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

#ifndef TAPE_H
#define TAPE_H

#include <cstdint>
#include <string>
#include <vector>

class Memory;
class MOS6502;
class Snapshot;

enum class TapeSnapshotKind : uint8_t {
    Blank,
    TapNormal,
    TapTurbo
};

struct TapeSnapshotState
{
    TapeSnapshotKind kind = TapeSnapshotKind::Blank;
    std::string path;
    std::vector<uint8_t> data;

    bool motor_running = false;
    uint8_t tape_state = 0;
    uint32_t sync_end = 0;
    uint32_t body_start = 0;
    uint32_t body_remaining = 0;
    uint16_t leader_count = 0;
    uint32_t tape_pos = 0;
    uint8_t bit_index = 0;
    bool stopped_mid_byte = false;

    uint8_t current_byte = 0;
    uint8_t current_bit = 0;
    uint8_t parity = 0;
    int16_t tape_cycle_counter = 0;
    uint8_t gap_bits_remaining = 0;
    uint8_t line_out = 0;

    bool turbo_loading = false;
    bool turbo_saving = false;
};


class Tape
{
public:
    Tape() :
        motor_running(false)
    {}

    virtual ~Tape() = default;

    /**
     * Initialize tape.
     * @return true on success
     */
    virtual bool init() = 0;

    /**
     * Reset tape postion.
     */
    virtual void reset() = 0;

    /**
     * Print tape status to console.
     */
    virtual void print_stat() = 0;

    /**
     * Set motor state.
     * @param motor_on true if motor is set to on
     */
    virtual void motor_on(bool motor_on) = 0;

    /**
     * Execute one cycle.
     */
    virtual void exec(uint8_t cycles) = 0;

    /**
     * Intercept a ROM tape routine before CPU opcode fetch. Allows turbo loading and saving.
     * @param cpu reference to CPU
     * @param ram reference to RAM
     * @param oric_rom_enabled true if Oric ROM is enabled
     * @return true if the tape handled this CPU step.
     */
    virtual bool intercept(MOS6502& cpu, Memory& ram, bool oric_rom_enabled) = 0;

    virtual void save_to_snapshot(Snapshot& snapshot) const;
    virtual void load_from_snapshot(const Snapshot& snapshot);

    /**
     * Check if motor is running.
     * @return true if motor is running.
     */
    bool is_motor_running() { return motor_running; };

protected:
    bool motor_running;
};

#endif // TAPE_TAP_H
