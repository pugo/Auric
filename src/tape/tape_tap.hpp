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

#ifndef TAPE_TAP_H
#define TAPE_TAP_H

#include <memory>

#include "chip/mos6522.hpp"
#include "tape.hpp"


class TapeTap : public Tape
{
    enum class TapeState {
        Idle,
        ParseHeader,
        Leader,
        Header,
        Gap,
        Body,
        EndOfBlock,
        Fail
    };

public:
    TapeTap(MOS6522& via, const std::string& path);

    virtual ~TapeTap() = default;

    /**
     * Initialize tape.
     * @return true on success
     */
    bool init() override;

    /**
     * Reset tape postion.
     */
    void reset() override;

    /**
     * Print tape status to console.
     */
    void print_stat() override;

    /**
     * Set motor state.
     * @param motor_on true if motor is on
     */
    void motor_on(bool motor_on) override;

    /**
     * Execute one cycle.
     */
    void exec() override;

protected:
    /**
     * Read tape header.
     * @return true if header is valid
     */
    bool parse_header();

    /**
     * Get current bit value.
     * @return current bit value
     */
    uint8_t next_bit();

    std::string path;
    MOS6522& via;
    size_t tape_size;

    TapeState tape_state;

    uint32_t sync_end;
    uint32_t body_start;
    uint32_t body_remaining;
    bool stopped_mid_byte;

    uint16_t leader_count;
    uint8_t  gap_bits_remaining;

    uint32_t tape_pos;
    uint8_t bit_index;
    uint8_t current_byte;
    uint8_t current_bit;
    uint8_t parity;

    int16_t tape_cycle_counter;
    uint8_t line_out;

    std::vector<uint8_t> memory_vector;
    uint8_t* data;

    static const int Pulse_1 = 208;
    static const int Pulse_0 = 416;
};

#endif // TAPE_TAP_H
