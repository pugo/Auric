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
#include <disk/drive.hpp>
#include "wd1793.hpp"


// ===== Operations ==========================================

// ----- OperationIdle -----

uint8_t OperationIdle::read_data_reg() const
{
    return 0x00;
}

void OperationIdle::write_data_reg(uint8_t value)
{
}

// ----- OperationReadSector -----

uint8_t OperationReadSector::read_data_reg() const
{
    return 0x00;
}

void OperationReadSector::write_data_reg(uint8_t value)
{
}

// ----- OperationWriteSector -----

uint8_t OperationWriteSector::read_data_reg() const
{
    return 0x00;
}

void OperationWriteSector::write_data_reg(uint8_t value)
{
}

// ----- OperationReadAddress -----

uint8_t OperationReadAddress::read_data_reg() const
{
    return 0x00;
}

void OperationReadAddress::write_data_reg(uint8_t value)
{
}

// ----- OperationReadTrack -----

uint8_t OperationReadTrack::read_data_reg() const
{
    return 0x00;
}

void OperationReadTrack::write_data_reg(uint8_t value)
{
}

// ----- OperationWriteTrack -----

uint8_t OperationWriteTrack::read_data_reg() const
{
    return 0x00;
}

void OperationWriteTrack::write_data_reg(uint8_t value)
{
}



// ===== WD1793 =============================================

void WD1793::State::reset()
{
    current_operation = &operation_idle;

    data = 0x00;
    drive = 0x00;
    side = 0x00;
    track = 0x00;
    sector = 0x00;
    command = 0x00;
    status = 0x00;

    interrupts_enabled = false;
    interrupt_counter = 0;
    irq_flag = false;

    data_request_counter = 0;
    data_request_flag = false;
}

void WD1793::State::print() const
{
}

// ===== MOS6522 =====

WD1793::WD1793(Machine& a_Machine, Drive* drive) :
    machine(a_Machine),
    drive(drive),
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

void WD1793::exec(uint8_t cycles)
{
    if (state.interrupt_counter > 0) {
        state.interrupt_counter -= cycles;
        if (state.interrupt_counter <= 0) {
            state.interrupt_counter = 0;
            std::println("WD1793 *IRQ*");

            interrupt_set();
        }
    }

    if (state.data_request_counter > 0) {
        state.data_request_counter -= cycles;
        if (state.data_request_counter <= 0) {
            state.data_request_counter = 0;
            std::println("WD1793 *DRQ*");
            data_request_set();
        }
    }
}

uint8_t WD1793::read_byte(uint16_t offset)
{
    std::println("WD1793::read_byte");

    switch (offset)
    {
        case 0x0:
            std::println("WD1793 *IRQ clear*");
            interrupt_clear();
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
    switch (offset)
    {
        case 0x00:
            std::println("WD1793::write_byte - command {:02x}", value);
            interrupt_clear();
            do_command(value);
            break;
        case 0x01:
            std::println("WD1793::write_byte - track {:02x}", value);
            state.track = value;
            break;
        case 0x02:
            std::println("WD1793::write_byte - sector {:02x}", value);
            state.sector = value;
            break;
        case 0x03:
            std::println("WD1793::write_byte - value {:02x}", value);
            state.data = value;
            break;
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
                state.status = Status::StatusBusy;
                if (command & 0x08) { state.status |= Status::StatusHeadLoaded; }
                state.current_operation = &state.operation_idle;
            }
            else {
                // Restore [Type 1]: : 0 0 1 1 h V r₁ r₀
                std::println("WD1793 do command: Restore");
                state.status = Status::StatusBusy;
                if (command & 0x08) { state.status |= Status::StatusHeadLoaded; }
                state.current_operation = &state.operation_idle;
            }
            break;
        case 0x20:
            // Step [Type 1]: 0 0 1 u h V r₁ r₀
            std::println("WD1793 do command: Step");
            state.status = Status::StatusBusy;
            if (command & 0x08) { state.status |= Status::StatusHeadLoaded; }
            state.current_operation = &state.operation_idle;
            break;
        case 0x40:
            // Step in [Type 1]: 0 1 0 u h V r₁ r₀
            std::println("WD1793 do command: Step in");
            state.status = Status::StatusBusy;
            if (command & 0x08) { state.status |= Status::StatusHeadLoaded; }
            state.current_operation = &state.operation_idle;
            break;
        case 0x60:
            // Step out [Type 1]: 0 1 1 u h V r₁ r₀
            std::println("WD1793 do command: Step out");
            state.status = Status::StatusBusy;
            if (command & 0x08) { state.status |= Status::StatusHeadLoaded; }
            state.current_operation = &state.operation_idle;
            break;
        case 0x80:
            // Read sector [Type 2]: 1 0 0 m F₂ E F₁ 0
            std::println("WD1793 do command: Read sector");
            state.status = Status::StatusBusy | StatusNotReady;
            state.data_request_counter = 60;
            state.operation_read_sector.multiple_sectors = command & 0x10;
            state.current_operation = &state.operation_read_sector;
            break;
        case 0xa0:
            // Write sector [Type 2]: 1 0 1 F₂ E F₁ a₀
            std::println("WD1793 do command: Write sector");
            state.status = Status::StatusBusy | StatusNotReady;
            state.data_request_counter = 500;
            state.operation_write_sector.multiple_sectors = command & 0x10;
            state.current_operation = &state.operation_write_sector;
            break;
        case 0xc0:
            if (command & 0x10) {
                // Force int [Type 4]: 1 1 0 1 I₃ I₂ I₁ I₀
                std::println("WD1793 do command: Force interrupt");
                state.status = 0;
                state.interrupt_counter = 0;
                interrupt_set();
                state.current_operation = &state.operation_idle;
            }
            else {
                // Read address [Type 3]: 1 1 0 0 0 E 0 0
                std::println("WD1793 do command: Read address");
                state.status = Status::StatusBusy | StatusNotReady | StatusDataRequest;
                state.reset();
                state.current_operation = &state.operation_read_address;
            }
            break;
        case 0xe0:
            if (command & 0x10) {
                // Write track [Type 3]: 1 1 1 1 0 E 0 0
                std::println("WD1793 do command: Write track");
                state.status = Status::StatusBusy | StatusNotReady;
                state.current_operation = &state.operation_write_track;
            }
            else {
                // Read track [Type 3]: 1 1 1 0 0 E 0 0
                std::println("WD1793 do command: Read track");
                state.reset();
                state.current_operation = &state.operation_read_track;
            }
            break;
    }
}

void WD1793::interrupt_set()
{
    state.irq_flag = true;
    if (state.interrupts_enabled) {
        std::println("--- WD1793 IRQ SET ---");

        machine.cpu->irq();
    }
}

void WD1793::interrupt_clear()
{
    state.irq_flag = false;
    machine.cpu->irq_clear();
}


void WD1793::data_request_set()
{
    state.data_request_flag = true;
    state.status |= Status::StatusDataRequest;
}

void WD1793::data_request_clear()
{
    state.data_request_flag = false;
    state.status &= ~Status::StatusDataRequest;
}