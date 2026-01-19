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

void WD1793::State::print() const
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
}

void WD1793::load_from_snapshot(Snapshot& snapshot)
{
}

void WD1793::exec()
{

}

uint8_t WD1793::read_byte(uint16_t offset)
{
    std::println("WD1793::read_byte");

    switch (offset)
    {
        case 0x0:
            return state.status;
        case 0x1:
            return state.track;
        case 0x2:
            return state.sector;
        case 0x3:
            return state.data;
        default:
            break;
    };

    return 0;
}

void WD1793::write_byte(uint16_t offset, uint8_t value)
{
    std::println("WD1793::write_byte");

    switch (offset)
    {
        case 0x00:
            do_command(value);
        case 0x01:
            state.track = value;
        case 0x02:
            state.sector = value;
        case 0x03:
            state.data = value;
        default:
            break;
    };
}

void WD1793::do_command(uint8_t command)
{
    state.command = command;

    switch (command & 0xe0) {
        case 0x00:
            if (command & 0x10) {
                // Seek [Type 1]: 0 0 1 0 h V r₁ r₀
                std::println("WD1793 do command: Seek");
            }
            else {
                // Restore [Type 1]: : 0 0 1 1 h V r₁ r₀
                std::println("WD1793 do command: Restore");
                state.reset();
            }
            break;
        case 0x20:
            // Step [Type 1]: 0 0 1 u h V r₁ r₀
            std::println("WD1793 do command: Step");
            break;
        case 0x40:
            // Step in [Type 1]: 0 1 0 u h V r₁ r₀
            std::println("WD1793 do command: Step in");
            break;
        case 0x60:
            // Step out [Type 1]: 0 1 1 u h V r₁ r₀
            std::println("WD1793 do command: Step out");
            break;
        case 0x80:
            // Read sector [Type 2]: 1 0 0 m F₂ E F₁ 0
            std::println("WD1793 do command: Read sector");
            break;
        case 0xa0:
            // Write sector [Type 2]: 1 0 1 F₂ E F₁ a₀
            std::println("WD1793 do command: Write sector");
            break;
        case 0xc0:
            if (command & 0x10) {
                // Force int [Type 4]: 1 1 0 1 I₃ I₂ I₁ I₀
                std::println("WD1793 do command: Force interrupt");
                machine.cpu->irq();
            }
            else {
                // Read address [Type 3]: 1 1 0 0 0 E 0 0
                std::println("WD1793 do command: Read address");
                state.reset();
            }
            break;
        case 0xe0:
            if (command & 0x10) {
                // Write track [Type 3]: 1 1 1 1 0 E 0 0
                std::println("WD1793 do command: Write track");
            }
            else {
                // Read track [Type 3]: 1 1 1 0 0 E 0 0
                std::println("WD1793 do command: Read track");
                state.reset();
            }
            break;
    }
}
