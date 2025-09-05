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

#ifndef SNAPSHOT_H
#define SNAPSHOT_H

#include <memory>
#include <vector>

#include "chip/mos6522.hpp"
#include "chip/ay3_8912.hpp"


/**
 * State for MOS6502 (CPU).
 */
class MOS6502_state
{
public:
    uint8_t A;
    uint8_t X;
    uint8_t Y;

    uint8_t N_INTERN; // negative
    uint8_t Z_INTERN; // zero
    bool V; // overflow
    bool B; // break
    bool D; // decimal
    bool I; // interrupt
    bool C; // carry

    uint16_t PC;
    uint8_t SP;

    bool irq_flag;
    bool nmi_flag;
    bool do_interrupt;
    bool do_nmi;

    bool instruction_load;
    uint8_t instruction_cycles;
    uint8_t current_instruction;
    uint8_t current_cycle;
};


/**
 * Snapshot of states for whole machine.
 */
class Snapshot
{
public:
    Snapshot();
    ~Snapshot();

    MOS6502_state mos6502;
    MOS6522::State mos6522;
    AY3_8912::SoundState ay3_8919;

    std::vector<uint8_t> memory;
};


#endif // SNAPSHOT_H
