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

#ifndef TAPE_TAP_NORMAL_H
#define TAPE_TAP_NORMAL_H

#include <filesystem>

#include "chip/mos6522.hpp"
#include "tape_tap.hpp"


class TapeTapNormal : public TapeTap
{
public:
    TapeTapNormal(MOS6522& via, const std::filesystem::path& path);

    ~TapeTapNormal() override = default;

    /**
     * Reset tape postion.
     */
    void reset() override;

    void save_to_snapshot(Snapshot& snapshot) const override;
    void load_from_snapshot(const Snapshot& snapshot) override;

    /**
     * Execute one cycle.
     */
    void exec(uint8_t cycles) override;

protected:
    /**
     * Get current bit value.
     * @return current bit value
     */
    uint8_t next_bit();

    uint8_t current_byte;
    uint8_t current_bit;
    uint8_t parity;
    int16_t tape_cycle_counter;
    uint8_t gap_bits_remaining;
    uint8_t line_out;
};

#endif // TAPE_TAP_NORMAL_H
