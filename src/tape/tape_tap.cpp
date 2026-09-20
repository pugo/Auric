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
#include <algorithm>
#include <fstream>
#include <cstdlib>
#include <format>
#include <optional>
#include <print>
#include <vector>
#include <string>

#include "tape_tap.hpp"
#include "snapshot.hpp"


TapeTap::TapeTap(MOS6522& via, const std::filesystem::path& path) :
    path(path),
    via(via),
    tape_size(0),
    tape_state(TapeState::Idle),
    sync_end(0),
    body_start(0),
    body_remaining(0),
    leader_count(0),
    tape_pos(0),
    bit_index(0),
    stopped_mid_byte(false),
    data(nullptr)
{
}


TapeTap::~TapeTap()
{
    close_tap_write_file();
}


void TapeTap::reset()
{
    close_tap_write_file();

    motor_running = false;
    tape_state = TapeState::Idle;
    sync_end = 0;
    body_start = 0;
    body_remaining = 0;
    leader_count = 0;
    tape_pos = 0;
    bit_index = 0;
    stopped_mid_byte = false;
}


void TapeTap::save_to_snapshot(Snapshot& snapshot) const
{
    snapshot.tape.kind = TapeSnapshotKind::TapNormal;
    snapshot.tape.path = path.string();
    snapshot.tape.data = memory_vector;
    snapshot.tape.motor_running = motor_running;
    snapshot.tape.tape_state = static_cast<uint8_t>(tape_state);
    snapshot.tape.sync_end = sync_end;
    snapshot.tape.body_start = body_start;
    snapshot.tape.body_remaining = body_remaining;
    snapshot.tape.leader_count = leader_count;
    snapshot.tape.tape_pos = tape_pos;
    snapshot.tape.bit_index = bit_index;
    snapshot.tape.stopped_mid_byte = stopped_mid_byte;
}


void TapeTap::load_from_snapshot(const Snapshot& snapshot)
{
    close_tap_write_file();

    const auto& saved = snapshot.tape;
    path = saved.path;
    memory_vector = saved.data;
    data = memory_vector.data();
    tape_size = memory_vector.size();
    motor_running = saved.motor_running;
    tape_state = static_cast<TapeState>(saved.tape_state);
    sync_end = saved.sync_end;
    body_start = saved.body_start;
    body_remaining = saved.body_remaining;
    leader_count = saved.leader_count;
    tape_pos = saved.tape_pos;
    bit_index = saved.bit_index;
    stopped_mid_byte = saved.stopped_mid_byte;
}


bool TapeTap::init()
{
    reset();
    spdlog::info("Tape: Reading TAP file '{}'", path.string());

    if (!std::filesystem::exists(path)) {
        spdlog::info("Tape: creating TAP file '{}'", path.string());
        std::ofstream create_file(path, std::ios::binary);
        if (!create_file) {
            spdlog::warn("Tape: unable to create TAP file");
            return false;
        }
    }

    tape_size = std::filesystem::file_size(path);

    std::ifstream file(path, std::ios::binary);
    if (!file) {
        spdlog::warn("Tape: unable to open TAP file");
        return false;
    }

    memory_vector.assign(std::istreambuf_iterator<char>(file), {});
    data = memory_vector.data();

    return true;
}


void TapeTap::print_stat()
{
    std::println("Current Tape pos: {}", tape_pos);
}


void TapeTap::motor_on(bool motor_on)
{
    if (motor_on == motor_running) {
        return;
    }
    spdlog::debug("Tape: motor {}", motor_on ? "on" : "off");

    motor_running = motor_on;

    if (motor_on) {
        if (stopped_mid_byte) {
            ++tape_pos;
            stopped_mid_byte = false;
        }
        tape_state = TapeState::ParseHeader;
    }
    else {
        close_tap_write_file();

        if (bit_index > 0) {
            // stopped mid-byte: drop the partial byte on resume
            spdlog::debug("Skipped one byte at resume (pos now {})", tape_pos);
            stopped_mid_byte = true;
            bit_index = 0;
        }
    }
}

bool TapeTap::has_tap_data() const
{
    return data != nullptr && tape_size > 0;
}


bool TapeTap::has_tap_header_at_current_pos() const
{
    if (!has_tap_data() || tape_pos >= tape_size) {
        return false;
    }

    size_t sync_len = 0;
    while (tape_pos + sync_len < tape_size && data[tape_pos + sync_len] == 0x16) {
        ++sync_len;
    }

    if (sync_len < 3) {
        return false;
    }

    const size_t header_marker_pos = tape_pos + sync_len;
    if (header_marker_pos >= tape_size || data[header_marker_pos] != 0x24) {
        return false;
    }

    return sync_len + 10 < tape_size - tape_pos;
}


bool TapeTap::seek_next_sync()
{
    if (!has_tap_data()) {
        return false;
    }

    const uint32_t original_tape_pos = tape_pos;

    while (tape_pos < tape_size) {
        if (data[tape_pos] != 0x16) {
            ++tape_pos;
            continue;
        }

        const size_t sync_start = tape_pos;
        size_t sync_len = 0;

        while (sync_start + sync_len < tape_size && data[sync_start + sync_len] == 0x16) {
            ++sync_len;
        }

        if (sync_len >= 3 && sync_start + sync_len < tape_size && data[sync_start + sync_len] == 0x24) {
            tape_pos = sync_start + sync_len;
            spdlog::debug("Tape: turbo sync at ${:x}", tape_pos);
            return true;
        }

        tape_pos = sync_start + 1;
    }

    tape_pos = original_tape_pos;
    return false;
}


std::optional<uint8_t> TapeTap::read_next_tap_byte()
{
    if (!has_tap_data() || tape_pos >= tape_size) {
        return std::nullopt;
    }

    return data[tape_pos++];
}


bool TapeTap::write_tap_byte(uint8_t byte)
{
    return write_tap_bytes(std::span<const uint8_t>(&byte, 1));
}


bool TapeTap::write_tap_bytes(std::span<const uint8_t> bytes)
{
    if (!write_file.is_open()) {
        write_file.clear();
        write_file.open(path, std::ios::binary | std::ios::in | std::ios::out);
    }

    if (!write_file) {
        spdlog::warn("Tape: unable to open TAP file for writing");
        return false;
    }

    const size_t write_pos = tape_pos;
    const size_t write_end = write_pos + bytes.size();

    write_file.seekp(static_cast<std::streamoff>(write_pos), std::ios::beg);
    write_file.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!write_file) {
        spdlog::warn("Tape: unable to write TAP data");
        return false;
    }

    if (memory_vector.size() < write_pos) {
        memory_vector.resize(write_pos, 0x00);
    }
    if (memory_vector.size() < write_end) {
        memory_vector.resize(write_end);
    }

    std::copy(bytes.begin(), bytes.end(), memory_vector.begin() + write_pos);
    data = memory_vector.data();
    tape_size = memory_vector.size();
    tape_pos = static_cast<uint32_t>(write_end);
    return true;
}


void TapeTap::close_tap_write_file()
{
    if (write_file.is_open()) {
        write_file.close();
    }
}


bool TapeTap::parse_header()
{
    size_t i = count_sync_bytes();
    uint16_t sync_len = i;
    sync_end = tape_pos + i;

    if (!validate_header_start(i)) {
        return false;
    }

    // Skip end of sync and reserved bytes.
    i += 1 + 2;

    auto file_type = data[tape_pos + i++];
    uint8_t auto_flag = data[tape_pos + i++];
    log_file_type(file_type, auto_flag);

    const bool basic_mode = (file_type == 0x00) || (auto_flag == 0x80);
    size_t desired_sync = basic_mode ? 192 : 112;

    const uint16_t end_address = data[tape_pos + i] << 8 | data[tape_pos + i + 1];
    i += 2;
    const uint16_t start_address = data[tape_pos + i] << 8 | data[tape_pos + i + 1];
    i += 2;

    spdlog::debug("Tape: start address: ${:04x}", start_address);
    spdlog::debug("Tape:   end address: ${:04x}", end_address);

    // Skip one reserved byte.
    i++;

    // Read variable-length name
    const std::string name = read_null_terminated_string(i);
    spdlog::info("Tape: file name: {}", name);

    // Store where body starts, to allow delay after header.
    body_start = tape_pos + i + 1;
    body_remaining = uint32_t(end_address) - size_t(start_address) + 1;
    leader_count = (sync_len < desired_sync) ? (desired_sync - sync_len) : 0;

    return true;
}


size_t TapeTap::count_sync_bytes()
{
    size_t i{0};

    while (true) {
        if (tape_pos + i >= tape_size) {
            return false;
        }

        if (data[tape_pos + i] != 0x16) {
            break;
        }
        ++i;
    }

    return i;
}


bool TapeTap::validate_header_start(size_t pos)
{
    spdlog::debug("Tape: found {} sync bytes (0x16)", pos);

    if (pos < 3) {
        spdlog::warn("Tape: too few sync bytes, failing.");
        return false;
    }

    if (data[tape_pos + pos] != 0x24) {
        spdlog::warn("Tape: missing end of sync bytes (0x24), failing.");
        return false;
    }

    ++pos;

    if (pos + 9 >= tape_size) {
        spdlog::warn("Tape: too short (no specs and addresses).");
        return false;
    }

    return true;
}


void TapeTap::log_file_type(uint8_t file_type, uint8_t auto_flag)
{
    switch(file_type)
    {
        case 0x00:
            spdlog::debug("Tape: file is BASIC.");
            break;
        case 0x80:
            spdlog::debug("Tape: file is machine code.");
            break;
        default:
            spdlog::debug("Tape: file is unknown.");
            break;
    }

    switch(auto_flag)
    {
        case 0x80:
            spdlog::debug("Tape: run automatically as BASIC.");
            break;
        case 0xc7:
            spdlog::debug("Tape: run automatically as machine code.");
            break;
        default:
            spdlog::debug("Tape: Don't run automatically.");
            break;
    }
}


std::string TapeTap::read_null_terminated_string(size_t& offset) const
{
    std::string result;

    while (true) {
        if (tape_pos + offset >= tape_size) {
            return result;
        }

        if (data[tape_pos + offset] == 0x00) {
            break;
        }

        result += data[tape_pos + offset];
        ++offset;
    }

    return result;
}
