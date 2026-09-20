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
#include <vector>

#include "chip/mos6502.hpp"
#include "memory.hpp"
#include "tape_tap_turbo.hpp"
#include "snapshot.hpp"


TapeTapTurbo::TapeTapTurbo(MOS6522& via, const std::filesystem::path& path, const RomPatch& patch) :
    TapeTapNormal(via, path),
    patch(patch),
    turbo_loading(false),
    turbo_saving(false)
{
    spdlog::info("Tape: turbo supported for {}", patch.name);
}


void TapeTapTurbo::reset()
{
    turbo_loading = false;
    turbo_saving = false;

    TapeTapNormal::reset();
}


void TapeTapTurbo::save_to_snapshot(Snapshot& snapshot) const
{
    TapeTapNormal::save_to_snapshot(snapshot);
    snapshot.tape.kind = TapeSnapshotKind::TapTurbo;
    snapshot.tape.turbo_loading = turbo_loading;
    snapshot.tape.turbo_saving = turbo_saving;
}


void TapeTapTurbo::load_from_snapshot(const Snapshot& snapshot)
{
    TapeTapNormal::load_from_snapshot(snapshot);
    turbo_loading = snapshot.tape.turbo_loading;
    turbo_saving = snapshot.tape.turbo_saving;
}


void TapeTapTurbo::motor_on(bool motor_on)
{
    if (!motor_on) {
        if (turbo_loading || turbo_saving) {
            reset_normal_timing_state();
        }
        turbo_loading = false;
        turbo_saving = false;
    }

    TapeTapNormal::motor_on(motor_on);
}


void TapeTapTurbo::exec(uint8_t cycles)
{
    if ((turbo_loading || turbo_saving) && motor_running) {
        via.write_cb1(true);
        return;
    }

    if (motor_running && tape_state == TapeState::ParseHeader && !has_tap_header_at_current_pos()) {
        via.write_cb1(true);
        return;
    }

    TapeTapNormal::exec(cycles);
}


bool TapeTapTurbo::intercept(MOS6502& cpu, Memory& ram, bool oric_rom_enabled)
{
    if (!motor_running || !oric_rom_enabled) {
        return false;
    }

    if (has_tap_data() && intercept_sync(cpu)) {
        return true;
    }

    if (has_tap_data() && intercept_read_byte(cpu, ram)) {
        return true;
    }

    if (intercept_write_block(cpu, ram)) {
        return true;
    }

    return intercept_write_byte(cpu, ram);
}


void TapeTapTurbo::reset_normal_timing_state()
{
    current_byte = 0;
    current_bit = 0;
    parity = 0;
    tape_cycle_counter = 0;
    gap_bits_remaining = 0;
    line_out = 0;
    bit_index = 0;
    stopped_mid_byte = false;
}


bool TapeTapTurbo::intercept_sync(MOS6502& cpu)
{
    const uint16_t pc = cpu.get_pc();
    if (pc != patch.get_sync_pc && pc != patch.get_sync_loop_pc) {
        return false;
    }

    if (!seek_next_sync()) {
        turbo_loading = false;
        return false;
    }

    reset_normal_timing_state();
    turbo_loading = true;
    cpu.set_pc(patch.get_sync_end_pc);
    return true;
}


bool TapeTapTurbo::intercept_read_byte(MOS6502& cpu, Memory& ram)
{
    if (cpu.get_pc() != patch.read_byte_pc) {
        return false;
    }

    auto byte = read_next_tap_byte();
    if (!byte) {
        turbo_loading = false;
        return false;
    }

    cpu.A = *byte;
    cpu.N_INTERN = *byte;
    cpu.Z_INTERN = *byte;
    cpu.C = patch.read_byte_set_carry;

    if (patch.read_byte_store_byte_addr) {
        ram.mem[*patch.read_byte_store_byte_addr] = *byte;
    }

    if (patch.read_byte_store_zero_addr) {
        ram.mem[*patch.read_byte_store_zero_addr] = 0x00;
    }

    cpu.set_pc(patch.read_byte_end_pc);
    return true;
}


bool TapeTapTurbo::intercept_write_block(MOS6502& cpu, Memory& ram)
{
    if (cpu.get_pc() != patch.write_block_pc) {
        return false;
    }

    std::vector<uint8_t> block;
    std::vector<uint8_t> header;
    header.reserve(9);

    for (uint8_t i = 9; i > 0; --i) {
        header.push_back(ram.mem[patch.write_header_base_addr + i]);
    }

    const uint8_t file_type = header[2];
    const uint8_t auto_flag = header[3];
    const bool basic_mode = (file_type == 0x00) || (auto_flag == 0x80);
    block.insert(block.end(), basic_mode ? 192 : 112, 0x16);
    block.push_back(0x24);
    block.insert(block.end(), header.begin(), header.end());

    const uint32_t memory_size = ram.get_size();
    bool name_terminated = false;
    for (uint32_t offset = 0; offset < memory_size; ++offset) {
        const uint8_t name_byte = ram.mem[(patch.write_name_addr + offset) % memory_size];
        block.push_back(name_byte);
        if (name_byte == 0x00) {
            name_terminated = true;
            break;
        }
    }
    if (!name_terminated) {
        spdlog::warn("Tape: unable to write TAP data, unterminated file name");
        return false;
    }

    const uint16_t start_addr = ram.mem[patch.write_data_start_addr]
        | (ram.mem[patch.write_data_start_addr + 1] << 8);
    const uint16_t end_addr = ram.mem[patch.write_data_end_addr]
        | (ram.mem[patch.write_data_end_addr + 1] << 8);

    for (uint32_t addr = start_addr; addr <= end_addr; ++addr) {
        block.push_back(ram.mem[addr]);
    }

    if (!write_tap_bytes(block)) {
        return false;
    }

    turbo_saving = true;
    cpu.C = true;
    cpu.set_pc(patch.write_block_end_pc);
    return true;
}


bool TapeTapTurbo::intercept_write_byte(MOS6502& cpu, Memory& ram)
{
    if (cpu.get_pc() != patch.write_byte_pc) {
        return false;
    }

    const uint8_t byte = cpu.A;
    if (!write_tap_byte(byte)) {
        return false;
    }

    turbo_saving = true;
    ram.mem[0x002f] = byte;
    cpu.A = cpu.X;
    cpu.N_INTERN = cpu.A;
    cpu.Z_INTERN = cpu.A;
    cpu.C = true;
    cpu.set_pc(patch.write_byte_end_pc);
    return true;
}
