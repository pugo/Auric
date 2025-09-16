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

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <print>
#include <vector>
#include <string>
#include <boost/assign.hpp>

#include "tape_tap.hpp"


TapeTap::TapeTap(MOS6522& via, const std::string& path) :
    via(via),
    path(path),
    size(0),
    body_start(0),
    delay(0),
    duplicate_bytes(0),
    stopped_mid_byte(false),
    tape_pos(0),
    bit_count(0),
    current_bit(0),
    parity(1),
    tape_cycles_counter(2),
    tape_pulse(0),
    data(nullptr)
{
}

void TapeTap::reset()
{
    motor_running = false;
    delay = 0;
    duplicate_bytes = 0;
    stopped_mid_byte = false;
    tape_pos = 0;
    bit_count = 0;
    current_bit = 0;
    tape_cycles_counter = 2;
    tape_pulse = 0;
    parity = 1;
}


bool TapeTap::init()
{
    reset();
    std::println("Tape: Reading TAP file '{}'", path);

    std::ifstream file (path, std::ios::in | std::ios::binary | std::ios::ate);
    if (file.is_open())
    {
        size = file.tellg();
        memory_vector = std::vector<uint8_t>(size);
        data = memory_vector.data();

        file.seekg(0, std::ios::beg);
        file.read(reinterpret_cast<char*>(data), size);
        file.close();
    }
    else {
        std::println("Tape: unable to open TAP file");
        return false;
    }

    return true;
}

bool TapeTap::read_header()
{
    size_t i{0};

    while (true)
    {
        if (tape_pos + i >= size) {
            return false;
        }

        if (data[tape_pos + i] != 0x16) {
            break;
        }

        ++i;
    }

    std::println("Tape: found {} sync bytes (0x16)", i);

    if (i < 3) {
        std::println("Tape: too few sync bytes, failing.");
        return false;
    }

    if (data[tape_pos + i] != 0x24) {
        std::println("Tape: missing end of sync bytes (0x24), failing.");
        return false;
    }

    i++;

    if (i + 9 >= size) {
        std::println("Tape: too short (no specs and addresses).");
        return false;
    }

    // Skip reserved bytes.
    i += 2;

    uint8_t file_type = data[tape_pos + i];
    switch(file_type)
    {
        case 0x00:
            std::println("Tape: file is BASIC.");
            break;
        case 0x80:
            std::println("Tape: file is machine code.");
            break;
        default:
            std::println("Tape: file is unknown.");
            break;
    }
    i++;

    uint8_t auto_flag = data[tape_pos + i];
    switch(auto_flag)
    {
        case 0x80:
            std::println("Tape: run automatically as BASIC.");
            break;
        case 0xc7:
            std::println("Tape: run automatically as machine code.");
            break;
        default:
            std::println("Tape: Don't run automatically.");
            break;
    }
    i++;

    const bool basic_mode = (file_type == 0x00) || (auto_flag == 0x80);
    size_t desired_sync = basic_mode ? 160 : 96;

    duplicate_bytes = (i < desired_sync) ? (desired_sync - i) : 0;

    uint16_t start_address;
    uint16_t end_address;

    end_address = data[tape_pos + i] << 8 | data[tape_pos + i + 1];
    i += 2;

    start_address = data[tape_pos + i] << 8 | data[tape_pos + i + 1];
    i += 2;

    std::println("Tape: start address: ${:04x}", start_address);
    std::println("Tape:   end address: ${:04x}", end_address);

    // Skip one reserved byte.
    i++;

    std::string name;
    while (true)
    {
        if (tape_pos + i >= size) {
            return false;
        }

        if (data[tape_pos + i] == 0x00) {
            break;
        }

        name += data[tape_pos + i];
        ++i;
    }
    std::println("Tape: file name: {}", name);

    // Store where body starts, to allow delay after header.
    body_start = tape_pos + i + 1;

    return true;
}


void TapeTap::print_stat()
{
    std::println("Current Tape pos: {}", tape_pos);
}


void TapeTap::set_motor(bool motor_on)
{
    if (motor_on == motor_running) {
        return;
    }
    std::println("Tape: motor {}", (motor_on ? "on" : "off"));

    motor_running = motor_on;

    if (!motor_running) {
        if (bit_count > 0) {
            stopped_mid_byte = true;
            bit_count = 0;
        }
        else {
            stopped_mid_byte = false;
        }
    }
    else {
        if (stopped_mid_byte) {
            if (tape_pos < size) {
                ++tape_pos;
            }
            stopped_mid_byte = false;
        }

        if (!read_header()) {
            motor_running = false;
            return;
        }
    }
}


void TapeTap::exec()
{
    if (!motor_running) {
        return;
    }

    if (tape_cycles_counter > 1) {
        tape_cycles_counter--;

//        if (delay > 0) {
//            --delay;
//        }
        return;
    }

    if (delay > 0) {
        if(--delay == 0)
        {
//            if (tape_pulse) {
//                delay = 1;
//            }
//            else {
//                delay = 0;
//            }
        }

        tape_cycles_counter = Pulse_1;
        return;
    }

    tape_pulse ^= 0x01;
    via.write_cb1(tape_pulse);

    if (tape_pulse) {
        // Start of bit, pulse up.
        current_bit = get_current_bit();
        tape_cycles_counter = Pulse_1;
    }
    else {
        // Second part of bit, differently long down period.
        tape_cycles_counter = current_bit ? Pulse_1 : Pulse_0;
    }
}


uint8_t TapeTap::get_current_bit()
{
    if (tape_pos >= size) {
        // Idle the line high and stop “playing”
        motor_running = false;
        return 1;
    }

    const uint8_t current_byte = data[tape_pos];
    uint8_t result{1};

    switch (bit_count) {
        case 0:
            // Start bit (always 0).
            result = 0;
            parity = 0;
            bit_count = 1;
            break;
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
            result = (current_byte & (0x01 << (bit_count - 1))) ? 1 : 0;
            parity ^= result;
            bit_count++;
            break;
        case 9:
            // Parity.
            result = parity ^ 1;
            bit_count++;
            break;
        case 10:
        case 11:
        case 12:
            // 3 stop bits (always 1).
            result = 1;

            bit_count = (bit_count + 1) % 13;

            if (bit_count == 0) {
                if (tape_pos + 1 == body_start) {
                    std::println("---- DELAY ----");
                    delay = 20;
                }

                if (duplicate_bytes > 0) {
                    --duplicate_bytes;
                }
                else {
                    ++tape_pos;
                }

            }
            break;
    }

    return result;
}


