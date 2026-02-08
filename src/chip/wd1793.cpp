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
    std::println("OperationIdle::read_data_reg()");
    return 0x00;
}

void OperationIdle::write_data_reg(uint8_t value)
{
    std::println("OperationIdle::write_data_reg(): {}", value);
}

// ----- OperationReadSector -----

uint8_t OperationReadSector::read_data_reg() const
{
    // std::println("OperationReadSector::read_data_reg()");
    if (wd1793.state.current_sector == nullptr) {
        wd1793.state.current_operation = &wd1793.operation_idle;
        wd1793.state.status &= ~WD1793::Status::StatusBusy;
        wd1793.state.status |= WD1793::Status::StatusRecordNotFound;
        wd1793.drive->data_request_clear();
        wd1793.drive->interrupt_set();
        std::println("OperationReadSector::read_data_reg() - Record not found!");
        return 0x00;
    }

    auto data_span = wd1793.state.current_sector->data;
    // std::println("OperationReadSector::read_data_reg() span size: {}", data_span.size());
    // std::println("OperationReadSector::read_data_reg() reading offset: {}", wd1793.state.offset);

    uint8_t v = data_span[wd1793.state.offset++];

    wd1793.state.status &= ~WD1793::Status::StatusDataRequest;
    wd1793.drive->data_request_clear();

    if (wd1793.state.offset >= data_span.size()) {
        if (multiple_sectors) {
            std::println("OperationReadSector::read_data_reg() - Multiple sectors, reading next");
            wd1793.state.sector += 1;
            wd1793.set_sector(wd1793.state.sector);
            wd1793.state.data_request_counter = 180;
            return 0x00;
        }

        std::println("OperationReadSector::read_data_reg() - No more data!");
        // Sista byten levererad -> command complete
        // wd1793.state.status &= ~WD1793::Status::StatusBusy;
        wd1793.state.status &= ~WD1793::Status::StatusDataRequest;
        // wd1793.state.status &= ~WD1793::Status::StatusLostData;
        wd1793.drive->data_request_clear();

        wd1793.state.interrupt_counter = 32;
        wd1793.state.set_status_at_interrupt(wd1793.state.current_sector->sector_mark == 0xfb ? 0 : WD1793::StatusRecordType);

        wd1793.state.data_request_counter = 0;
        wd1793.state.current_operation = &wd1793.operation_idle;
        printf("STATUS AFTER READ: %02X\n", wd1793.state.status);
    }
    else {
        wd1793.state.data_request_counter = 32; // eller 0 för "direkt"
    }

    return v;
}

void OperationReadSector::write_data_reg(uint8_t value)
{
    std::println("OperationReadSector::write_data_reg(): {}", value);
}

// ----- OperationWriteSector -----

uint8_t OperationWriteSector::read_data_reg() const
{
    std::println("OperationWriteSector::read_data_reg()");
    return 0x00;
}

void OperationWriteSector::write_data_reg(uint8_t value)
{
    std::println("OperationWriteSector::write_data_reg(): {}", value);
}

// ----- OperationReadAddress -----

uint8_t OperationReadAddress::read_data_reg() const
{
    std::println("OperationReadAddress::read_data_reg()");
    return 0x00;
}

void OperationReadAddress::write_data_reg(uint8_t value)
{
    std::println("OperationReadAddress::write_data_reg(): {}", value);
}

// ----- OperationReadTrack -----

uint8_t OperationReadTrack::read_data_reg() const
{
    std::println("////////////////////////////////////////////");
    std::println("OperationReadTrack::read_data_reg()");
    uint8_t data = wd1793.state.current_track->data[wd1793.state.offset++];
    wd1793.state.status &= ~WD1793::Status::StatusDataRequest;
    return data;
}

void OperationReadTrack::write_data_reg(uint8_t value)
{
    std::println("OperationReadTrack::write_data_reg(): {}", value);
}

// ----- OperationWriteTrack -----

uint8_t OperationWriteTrack::read_data_reg() const
{
    return 0x00;
}

void OperationWriteTrack::write_data_reg(uint8_t value)
{
    std::println("OperationWriteTrack::write_data_reg(): {}", value);
}



// ===== WD1793 =============================================

void WD1793::State::reset()
{
    data = 0x00;
    drive = 0x00;
    side = 0x00;
    track = 0x00;
    sector = 0x00;
    command = 0x00;
    status = 0x00;

    current_track_number = 0x00;
    current_sector_number = 0x00;
    sector_type = 0x00;
    offset = 0x0000;

    interrupt_counter = 0;
    status_at_interrupt = 0x00;
    update_status_at_interrupt = false;

    data_request_counter = 0;

    current_track = nullptr;
}

void WD1793::State::print() const
{
}

// ===== WD1793 =====

WD1793::WD1793(Machine& a_Machine, Drive* drive) :
    machine(a_Machine),
    drive(drive),
    state(),
    operation_idle(*this),
    operation_read_sector(*this),
    operation_write_sector(*this),
    operation_read_address(*this),
    operation_read_track(*this),
    operation_write_track(*this)
{
    state.reset();
    state.current_operation = &operation_idle;
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

            // Delayed update of status register at interrupt time.
            if (state.update_status_at_interrupt) {
                state.status = state.status_at_interrupt;
                state.status_at_interrupt = 0;
                state.update_status_at_interrupt = false;
            }

            drive->interrupt_set();
        }
    }

    if (state.data_request_counter > 0) {
        state.data_request_counter -= cycles;
        if (state.data_request_counter <= 0) {
            state.data_request_counter = 0;
            // std::println("WD1793 *DRQ*");
            state.status |= Status::StatusDataRequest;
            drive->data_request_set();
        }
    }
}

uint8_t WD1793::read_byte(uint16_t offset)
{
    switch (offset)
    {
        case 0x0:
            // std::println("WD1793::read_byte(0) - *IRQ clear*");
            drive->interrupt_clear();
            return state.status;
        case 0x1:
            std::println("WD1793::read_byte(0x01) - track: {}", state.track);
            return state.track;
        case 0x2:
            std::println("WD1793::read_byte(0x02) - sector: {}", state.sector);
            return state.sector;
        case 0x3:
            // std::println("WD1793::read_byte(0x03) - data");
            return state.current_operation->read_data_reg();
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
            drive->interrupt_clear();
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
            state.current_operation->write_data_reg(value);
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
                state.current_operation = &operation_idle;
                set_track(state.data);
            }
            else {
                // Restore [Type 1]: : 0 0 1 1 h V r₁ r₀
                std::println("WD1793 do command: Restore");
                state.status = Status::StatusBusy;
                if (command & 0x08) { state.status |= Status::StatusHeadLoaded; }
                state.current_operation = &operation_idle;
                set_track(0);
            }
            break;
        case 0x20:
            // Step [Type 1]: 0 0 1 u h V r₁ r₀
            std::println("WD1793 do command: Step");
            state.status = Status::StatusBusy;
            if (command & 0x08) { state.status |= Status::StatusHeadLoaded; }
            state.current_operation = &operation_idle;
            break;
        case 0x40:
            // Step in [Type 1]: 0 1 0 u h V r₁ r₀
            std::println("WD1793 do command: Step in");
            state.status = Status::StatusBusy;
            if (command & 0x08) { state.status |= Status::StatusHeadLoaded; }
            set_track(state.current_track_number + 1);
            state.current_operation = &operation_idle;
            break;
        case 0x60:
            // Step out [Type 1]: 0 1 1 u h V r₁ r₀
            std::println("WD1793 do command: Step out");
            state.status = Status::StatusBusy;
            if (command & 0x08) { state.status |= Status::StatusHeadLoaded; }
            set_track(state.current_track_number - 1);
            state.current_operation = &operation_idle;
            break;
        case 0x80:
            // Read sector [Type 2]: 1 0 0 m F₂ E F₁ 0
            std::println("WD1793 do command: Read sector: {:02x}", command);
            state.status = Status::StatusBusy | StatusNotReady;
            state.offset = 0;
            state.data_request_counter = 60;
            operation_read_sector.multiple_sectors = command & 0x10;
            state.current_operation = &operation_read_sector;
            set_sector(state.sector);
            break;
        case 0xa0:
            // Write sector [Type 2]: 1 0 1 F₂ E F₁ a₀
            std::println("WD1793 do command: Write sector");
            state.status = Status::StatusBusy | StatusNotReady;
            state.data_request_counter = 500;
            operation_write_sector.multiple_sectors = command & 0x10;
            state.current_operation = &operation_write_sector;
            break;
        case 0xc0:
            if (command & 0x10) {
                // Force int [Type 4]: 1 1 0 1 I₃ I₂ I₁ I₀
                std::println("WD1793 do command: Force interrupt");
                state.status = 0;
                drive->data_request_clear();
                state.interrupt_counter = 0;
                state.data_request_counter = 0;
                drive->interrupt_set();
                state.current_operation = &operation_idle;
            }
            else {
                // Read address [Type 3]: 1 1 0 0 0 E 0 0
                std::println("WD1793 do command: Read address");
                state.status = Status::StatusBusy | StatusNotReady | StatusDataRequest;
                state.current_operation = &operation_read_address;
            }
            break;
        case 0xe0:
            if (command & 0x10) {
                // Write track [Type 3]: 1 1 1 1 0 E 0 0
                std::println("WD1793 do command: Write track");
                state.status = Status::StatusBusy | StatusNotReady;
                state.current_operation = &operation_write_track;
                state.offset = 0;
                state.data_request_counter = 500;
            }
            else {
                // Read track [Type 3]: 1 1 1 0 0 E 0 0
                std::println("WD1793 do command: Read track");
                state.status = Status::StatusBusy | StatusNotReady;
                state.current_operation = &operation_read_track;
                state.offset = 0;
                state.data_request_counter = 60;
            }
            break;
    }
}


bool WD1793::set_track(uint8_t track)
{
    std::println("WD1793 set track: set to: {}", track);
    auto* disk_image = drive->get_disk_image();
    if (! disk_image) {
        std::println("WD1793 set track: No disk image!");
        drive->interrupt_set();
        state.current_track_number = 0;
        return false;
    }
    if (track < disk_image->tracks_count()) {
        state.set_status_at_interrupt(StatusIndex | StatusHeadLoaded);
    }
    else {
        track = disk_image->tracks_count() - 1;
        state.set_status_at_interrupt(StatusSeekError | StatusHeadLoaded);
    }

    auto* track_ptr = disk_image->get_track(state.side, track);
    if (track_ptr == nullptr) {
        std::println("WD1793 set track: Unable to get track data!");
        return false;
    }
    state.current_track = track_ptr;

    std::println("WD1793 set track: OK");
    std::println("Current track sector count: {}", state.current_track->sector_count());

    state.track = track;
    state.current_track_number = track;
    state.current_sector_number = 0;

    state.interrupt_counter = 20;
    if (state.current_track_number == 0) {
        state.status_at_interrupt |= StatusTrack00;
    }

    return true;
}

bool WD1793::set_sector(uint8_t sector)
{
    if (! state.current_track) {
        std::println("WD1793 set sector: no track selected!");
        drive->interrupt_set();
        state.current_track_number = 0;
        state.current_sector_number = 0;
        return false;
    }

    auto* sector_ptr = state.current_track->get_sector(sector);
    if (sector_ptr == nullptr) {
        std::println("WD1793 set track: Unable to get sector data!");
        return false;
    }
    state.current_sector = sector_ptr;
    state.current_sector_number = sector;
    std::println("WD1793 set sector: sector set to: {}", sector);

    return true;
}
