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

#include <spdlog/spdlog.h>
#include <machine.hpp>

#include "drive_microdrive.hpp"

#include <chrono>
#include <format>
#include <print>
#include <random>

using namespace std::chrono_literals;

DriveMicrodrive::DriveMicrodrive(Machine& machine) :
    machine(machine),
    wd1793(machine, this)
{
    state.reset();
}

DriveMicrodrive::~DriveMicrodrive()
{
    for (auto& disk_image : disk_images) {
        if (disk_image) {
            disk_image->flush_if_dirty(true);
        }
    }
}

void DriveMicrodrive::State::reset()
{
    drive_number = 0;
    status = 0;
    interrupt_request = 0x80;
    data_request = 0x80;
}

bool DriveMicrodrive::init()
{
    return true;
}

bool DriveMicrodrive::insert_disk(const std::filesystem::path& path, uint8_t drive_number)
{
    if (drive_number >= disk_images.size()) {
        spdlog::warn("Invalid drive number: {}", static_cast<int>(drive_number));
        return false;
    }

    auto& disk_image = disk_images[drive_number];

    if (!std::filesystem::exists(path)) {
        spdlog::warn("Disk image not found: {}", path.string());
        machine.frontend->get_status_bar().show_text_for(std::format("Disk image not found: {}", path.string()), 5s);
        if (disk_image) {
            disk_image->flush_if_dirty(true);
        }
        disk_image = nullptr;
        if (drive_number == state.drive_number) {
            wd1793.media_changed();
        }
        return false;
    }

    if (disk_image) {
        disk_image->flush_if_dirty(true);
    }

    auto new_disk_image = std::make_unique<DiskImage>(path);
    if (!new_disk_image->init()) {
        spdlog::warn("Failed to initialize disk image: {}", path.string());
        disk_image = nullptr;
        if (drive_number == state.drive_number) {
            wd1793.media_changed();
        }
        return false;
    }

    disk_image_path = path;
    disk_image = std::move(new_disk_image);
    if (drive_number == state.drive_number) {
        wd1793.media_changed();
    }

    return true;
}

bool DriveMicrodrive::eject_disk(uint8_t drive_number)
{
    if (drive_number >= disk_images.size()) {
        spdlog::warn("Invalid drive number: {}", static_cast<int>(drive_number));
        return false;
    }

    auto& disk_image = disk_images[drive_number];
    if (disk_image) {
        disk_image->flush_if_dirty(true);
    }

    disk_image = nullptr;
    if (drive_number == state.drive_number) {
        wd1793.selected_drive_changed();
    }

    return true;
}

DiskImage* DriveMicrodrive::get_disk_image()
{
    if (state.drive_number >= disk_images.size()) {
        return nullptr;
    }

    return disk_images[state.drive_number].get();
}

void DriveMicrodrive::reset()
{
    state.reset();
    wd1793.reset();
}

void DriveMicrodrive::print_stat()
{
    spdlog::info("Microdrive status: {:x}", state.status);
}

void DriveMicrodrive::exec(uint8_t cycles)
{
    wd1793.exec(cycles);
}

void DriveMicrodrive::exec_once_per_frame()
{
    for (auto& disk_image : disk_images) {
        if (disk_image) {
            disk_image->flush_if_dirty();
        }
    }
}

void DriveMicrodrive::interrupt_set()
{
    state.interrupt_request = 0x00;

    if (state.status & MdInterruptEnabled) {
        machine.cpu->set_irq_source(IRQ_SOURCE_FDC);
    }
}

void DriveMicrodrive::interrupt_clear()
{
    state.interrupt_request = 0x80;     // Bit 7 in status read
    machine.cpu->clear_irq_source(IRQ_SOURCE_FDC);
}

void DriveMicrodrive::data_request_set()
{
    state.data_request = 0x00;
}

void DriveMicrodrive::data_request_clear()
{
    state.data_request = 0x80;
}

uint8_t DriveMicrodrive::read_byte(uint16_t offset)
{
    // std::println("Microdrive read: {:04x}", offset);

    if (offset == 0x4) {
        return state.interrupt_request | 0x7f;
    }

    if (offset == 0x8) {
        // std::println("--- DRQ READ ---");
        return state.data_request | 0x7f;
    }

    return wd1793.read_byte(offset);
}

void DriveMicrodrive::write_byte(uint16_t offset, uint8_t value)
{
    if (offset == 0x4) {
        const auto old_status = state.status;
        state.status = value;

        const auto old_drive = state.drive_number;
        const auto new_drive = (value & 0x60) >> 5;

        if (new_drive != old_drive) {
            state.drive_number = new_drive;
            wd1793.selected_drive_changed();
        }

        const auto old_side = (old_status & 0x10) ? 1 : 0;
        const auto new_side = (value & 0x10) ? 1 : 0;

        if (new_side != old_side) {
            wd1793.set_side_number(new_side);
        }

        if ((old_status ^ value) & 0x02) {
            machine.set_oric_rom_enabled(value & 0x02);
        }

        if ((old_status ^ value) & 0x80) {
            machine.set_diskdrive_rom_enabled(!(value & 0x80));
        }

        if (state.status & MdInterruptEnabled && state.interrupt_request == 0) {
            machine.cpu->set_irq_source(IRQ_SOURCE_FDC);
        }
        else {
            machine.cpu->clear_irq_source(IRQ_SOURCE_FDC);
        }
        return;
    }

    if (offset == 0x8) {
        data_request_clear();
        return;
    }

    return wd1793.write_byte(offset, value);
}

void DriveMicrodrive::save_to_snapshot(Snapshot& snapshot)
{
    snapshot.drive_microdrive = state;
    for (size_t i = 0; i < disk_images.size(); ++i) {
        if (disk_images[i]) {
            snapshot.disk_images[i] = disk_images[i]->save_to_snapshot();
        }
        else {
            snapshot.disk_images[i].reset();
        }
    }
    wd1793.save_to_snapshot(snapshot);
}

void DriveMicrodrive::load_from_snapshot(Snapshot& snapshot)
{
    state = snapshot.drive_microdrive;

    for (size_t i = 0; i < disk_images.size(); ++i) {
        const auto& saved = snapshot.disk_images[i];
        if (!saved) {
            disk_images[i].reset();
            continue;
        }

        if (!disk_images[i] || disk_images[i]->save_to_snapshot().path != saved->path) {
            disk_images[i] = std::make_unique<DiskImage>(saved->path);
            if (!disk_images[i]->init()) {
                disk_images[i].reset();
                continue;
            }
        }
        disk_images[i]->load_from_snapshot(*saved);
    }

    wd1793.load_from_snapshot(snapshot);
}
