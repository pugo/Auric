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

#include <boost/log/trivial.hpp>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <format>
#include <print>
#include <vector>
#include <string>
#include <boost/assign.hpp>

#include "tape_tap.hpp"


TapeTap::TapeTap(MOS6522& via, const std::filesystem::path& path) :
    via(via),
    path(path),
    tape_size(0),
    tape_state(TapeState::Idle),
    sync_end(0),
    body_start(0),
    body_remaining(0),
    stopped_mid_byte(false),
    leader_count(0),
    tape_pos(0),
    bit_index(0),
    current_byte(0),
    current_bit(0),
    parity(0),
    tape_cycle_counter(0),
    line_out(0),
    data(nullptr)
{
}

void TapeTap::reset()
{
    motor_running = false;
    tape_state = TapeState::Idle;
    sync_end = 0;
    body_start = 0;
    body_remaining = 0;
    stopped_mid_byte = false;
    leader_count = 0;
    tape_pos = 0;
    bit_index = 0;
    current_byte = 0;
    current_bit = 0;
    parity = 0;
    tape_cycle_counter = 0;
    line_out = 0;
}


bool TapeTap::init()
{
    reset();
    BOOST_LOG_TRIVIAL(info) << "Tape: Reading TAP file '" << path << "'";

    std::ifstream file (path, std::ios::in | std::ios::binary | std::ios::ate);
    if (file.is_open())
    {
        tape_size = file.tellg();
        memory_vector = std::vector<uint8_t>(tape_size);
        data = memory_vector.data();

        file.seekg(0, std::ios::beg);
        file.read(reinterpret_cast<char*>(data), tape_size);
        file.close();
    }
    else {
        BOOST_LOG_TRIVIAL(warning) << "Tape: unable to open TAP file";
        return false;
    }

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
    BOOST_LOG_TRIVIAL(debug) << "Tape: motor " << (motor_on ? "on" : "off");

    motor_running = motor_on;

    if (motor_on) {
        if (stopped_mid_byte) {
            ++tape_pos;
            stopped_mid_byte = false;
        }
        tape_state = TapeState::ParseHeader;
    }
    else {
        if (bit_index > 0) {
            // stopped mid-byte: drop the partial byte on resume
            BOOST_LOG_TRIVIAL(debug) << "Skipped one byte at resume (pos now " << tape_pos << ")";
            stopped_mid_byte = true;
            bit_index = 0;
        }
    }
}


void TapeTap::exec()
{
    if (!motor_running) {
        return;
    }

    if (tape_state == TapeState::Idle || tape_state == TapeState::Fail) {
        return;
    }

    if (tape_state == TapeState::ParseHeader) {
        if (!parse_header()) {
            BOOST_LOG_TRIVIAL(error) << "Tape: failed to read header, stopping.";
            motor_running = false;
            tape_state = TapeState::Fail;
            return;
        }
        via.write_cb1(true);
        line_out = 1;
        tape_state = TapeState::Leader;
        return;
    }

    // End-of-block: hold idle high
    if (tape_state == TapeState::EndOfBlock) {
        via.write_cb1(true);
        line_out = 1;
        tape_cycle_counter = Pulse_1;
        return;
    }

    // Count down the cycle counter. This ensures that the line_out toggles according to expected bit output.
    if (tape_cycle_counter > 1) {
        --tape_cycle_counter;
        return;
    }

    // At the end of the above cycle count, toggle the output line.
    line_out ^= 0x01;
    via.write_cb1(line_out);

    // In state Gap we emit a series of bits to allow the reader routine to catch up.
    if (tape_state == TapeState::Gap) {
        if (line_out) {
            tape_cycle_counter = Pulse_1;
            return;
        } else {
            tape_cycle_counter = Pulse_1;
            if (--gap_bits_remaining == 0) {
                tape_state = TapeState::Body;
            }
            return;
        }
    }

    if (line_out) {
        // Start of bit, pulse up.
        if (bit_index == 0) {
            switch (tape_state) {
                case TapeState::Leader:
                    current_byte = 0x16;
                    break;
                case TapeState::Header:
                    current_byte = data[tape_pos];
                    break;
                case TapeState::Gap:
                    current_byte = 0xFF;
                    break;
                case TapeState::Body:
                    current_byte = data[tape_pos];
                    break;
                default:
                    current_byte = 0xFF;
                    break;
            }
        }

        // Get next bit to be output and update cycle counter accordingly.
        current_bit = next_bit();
        tape_cycle_counter = Pulse_1;

        // Update tape position and possibly switch state.
        if (bit_index == 0) {
            switch (tape_state) {
                case TapeState::Leader:
                    if (tape_pos < sync_end) {
                        // consumed one real 0x16 from file
                        ++tape_pos;
                    }
                    else if (leader_count > 0) {
                        // emitted a duplicated 0x16; stay on same tape_pos
                        --leader_count;
                    }

                    if (tape_pos >= sync_end && leader_count == 0) {
                        tape_state = TapeState::Header;
                    }
                    break;

                case TapeState::Header:
                    ++tape_pos;  // consumed one header/filename byte
                    if (tape_pos == body_start) {
                        gap_bits_remaining   = 10;   // emit 10 full '1' bits (tap2wav-style)
                        tape_state = TapeState::Gap;
                    }
                    break;

                case TapeState::Body:
                    ++tape_pos;       // consumed one body byte
                    if (--body_remaining == 0) {
                        // Body done: go idle-high and wait for next motor off/on
                        tape_state = TapeState::EndOfBlock;
                    }
                    break;

                default:
                    break;
            }
        }
    }
    else {
        // Second part of bit, differently long down period.
        tape_cycle_counter = current_bit ? Pulse_1 : Pulse_0;
    }
}


bool TapeTap::parse_header()
{
    size_t i{0};

    while (true)
    {
        if (tape_pos + i >= tape_size) {
            return false;
        }

        if (data[tape_pos + i] != 0x16) {
            break;
        }

        ++i;
    }

    BOOST_LOG_TRIVIAL(debug) << "Tape: found " << i << " sync bytes (0x16)";
    uint16_t sync_len = i;
    sync_end = tape_pos + i;

    if (i < 3) {
        BOOST_LOG_TRIVIAL(warning) << "Tape: too few sync bytes, failing.";
        return false;
    }

    if (data[tape_pos + i] != 0x24) {
        BOOST_LOG_TRIVIAL(warning) << "Tape: missing end of sync bytes (0x24), failing.";
        return false;
    }

    ++i;

    if (i + 9 >= tape_size) {
        BOOST_LOG_TRIVIAL(warning) << "Tape: too short (no specs and addresses).";
        return false;
    }

    // Skip reserved bytes.
    i += 2clo;

    auto file_type = data[tape_pos + i];
    switch(file_type)
    {
        case 0x00:
            BOOST_LOG_TRIVIAL(debug) << "Tape: file is BASIC.";
            break;
        case 0x80:
            BOOST_LOG_TRIVIAL(debug) << "Tape: file is machine code.";
            break;
        default:
            BOOST_LOG_TRIVIAL(debug) << "Tape: file is unknown.";
            break;
    }
    i++;

    uint8_t auto_flag = data[tape_pos + i];
    switch(auto_flag)
    {
        case 0x80:
            BOOST_LOG_TRIVIAL(debug) << "Tape: run automatically as BASIC.";
            break;
        case 0xc7:
            BOOST_LOG_TRIVIAL(debug) << "Tape: run automatically as machine code.";
            break;
        default:
            BOOST_LOG_TRIVIAL(debug) << "Tape: Don't run automatically.";
            break;
    }
    i++;

    const bool basic_mode = (file_type == 0x00) || (auto_flag == 0x80);
    size_t desired_sync = basic_mode ? 192 : 112;

    uint16_t start_address;
    uint16_t end_address;

    end_address = data[tape_pos + i] << 8 | data[tape_pos + i + 1];
    i += 2;

    start_address = data[tape_pos + i] << 8 | data[tape_pos + i + 1];
    i += 2;

    BOOST_LOG_TRIVIAL(debug) << std::format("Tape: start address: ${:04x}", start_address);
    BOOST_LOG_TRIVIAL(debug) << std::format("Tape:   end address: ${:04x}", end_address);

    // Skip one reserved byte.
    i++;

    std::string name;
    while (true)
    {
        if (tape_pos + i >= tape_size) {
            return false;
        }

        if (data[tape_pos + i] == 0x00) {
            break;
        }

        name += data[tape_pos + i];
        ++i;
    }
    BOOST_LOG_TRIVIAL(info) << "Tape: file name: " << name;

    // Store where body starts, to allow delay after header.
    body_start = tape_pos + i + 1;
    body_remaining = uint32_t(end_address) - size_t(start_address) + 1;

    leader_count = (sync_len < desired_sync) ? (desired_sync - sync_len) : 0;

    return true;
}

// Tape output is a delicate thing on the Oric. The below is not exactly what the ROM routines expect,
// but since many games use their own loader routined and expect slightly different timings and bit output
// this is a pattern that seems to work. The use of two initial bits is influenced by Oricutron.

uint8_t TapeTap::next_bit()
{
    if (bit_index == 0) {
        // Start bit (always 0).
        parity=1;
        bit_index = 1;
        return 1;
    }

    if (bit_index == 1) {
        bit_index = 2;
        return 0;
    }

    if (bit_index <= 9) {
        uint8_t b = (current_byte >> (bit_index - 2)) & 0x01;
        parity ^= b;
        bit_index++;
        return b;
    }

    if (bit_index == 10) {
        // Parity bit calculated over data bits.
        bit_index++;
        return parity;
    }

    if (bit_index == 11) {
        bit_index++;
        return 1;
    }

    if (bit_index == 12) {
        bit_index++;
        return 1;
    }

    bit_index = 0;
    return 1; // last stop bit, next call starts new frame
}


