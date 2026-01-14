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

#include <utility>
#include <print>

#include <machine.hpp>
#include "wd1793.hpp"


void WD1793::State::reset()
{
    data = 0x00;
    track = 0x00;
    sector = 0x00;
    command = 0x00;
    status = 0x00;
}


void MOS6522::State::print() const
{
}

// ===== MOS6522 =====

WD1793::WD1793(Machine& a_Machine) :
    machine(a_Machine),
    state()
{
    state.reset();
}


void WD1793::save_to_snapshot(Snapshot& snapshot)
{
    // snapshot.mos6522 = state;
}

void WD1793::load_from_snapshot(Snapshot& snapshot)
{
    // state = snapshot.mos6522;
}


void WD1793::exec()
{

}

uint8_t WD1793::read_byte(uint16_t offset)
{
    return 0;
}

void WD1793::write_byte(uint16_t offset, uint8_t value)
{

}
