// =========================================================================
//   Copyright (C) 2009-2024 by Anders Piniesjö <pugo@pugo.org>
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
    tape_pos(0),
    bit_count(0),
    current_bit(0),
    parity(1),
    tape_cycles_counter(2),
    tape_pulse(0)
{
}


TapeTap::~TapeTap()
{
}


void TapeTap::reset()
{
    motor_running = false;
    delay = 0;
    duplicate_bytes = 0;
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
    std::cout << "Tape: Reading TAP file '" << path << "'" << std::endl;

    std::ifstream file (path, std::ios::in | std::ios::binary | std::ios::ate);
    if (file.is_open())
    {
        size = file.tellg();
        data = new uint8_t[size];
        file.seekg(0, std::ios::beg);
        file.read(reinterpret_cast<char*>(data), size);
        file.close();
    }
    else {
        std::cout << "Tape: unable to open file";
        return false;
    }

    return true;
}

bool TapeTap::read_header()
{
    size_t i;

    for (i = 0; data[tape_pos + i] == 0x16; i++)
    {
        if (i >= size) {
            return false;
        }
    }

    std::cout << "Tape: found " << i << " sync bytes (0x16)" << std::endl;
    if (i < 3) {
        std::cout << "Tape: too few sync bytes, failing." << std::endl;
        return false;
    }

    if (data[tape_pos + i] != 0x24) {
        std::cout << "Tape: missing end of sync bytes (0x24), failing." << std::endl;
        return false;
    }

    i++;

    if (i + 9 >= size) {
        std::cout << "Tape: too short (no specs and addresses)." << std::endl;
        return false;
    }

    // Skip reserved bytes.
    i += 2;

    switch(data[tape_pos + i])
    {
        case 0x00:
            std::cout << "Tape: file is BASIC." << std::endl;
            break;
        case 0x80:
            std::cout << "Tape: file is machine code." << std::endl;
            break;
        default:
            std::cout << "Tape: file is unknown." << std::endl;
            break;
    }
    i++;

    switch(data[tape_pos + i])
    {
        case 0x80:
            std::cout << "Tape: run automatically as BASIC." << std::endl;
            break;
        case 0xc7:
            std::cout << "Tape: run automatically as machine code." << std::endl;
            break;
        default:
            std::cout << "Tape: Don't run automatically." << std::endl;
            break;
    }
    i++;

    uint16_t start_address;
    uint16_t end_address;

    end_address = data[tape_pos + i] << 8 | data[tape_pos + i + 1];
    i += 2;

    start_address = data[tape_pos + i] << 8 | data[tape_pos + i + 1];
    i += 2;

    std::cout << "Tape: start address: " << std::hex << (int)start_address << std::endl;
    std::cout << "Tape:   End address: " << std::hex << (int)end_address << std::endl;

    // Skip one reserved byte.
    i++;

    std::string name;
    while (data[tape_pos + i] != 0x00)
    {
        name += data[tape_pos + i];
        if (i >= size) {
            return false;
        }
        i++;
    }
    std::cout << "Tape: file name: " << name << std::endl;

    // Store where body starts, to allow delay after header.
    body_start = i + 1;
    duplicate_bytes = 80;

    return true;
}


void TapeTap::print_stat()
{
    std::cout << "Current Tape pos: " << tape_pos << std::endl;
}


void TapeTap::set_motor(bool motor_on)
{
    if (motor_on == motor_running) {
        return;
    }
    std::cout << "Tape: motor " << (motor_on ? "on" : "off") << std::endl;

    motor_running = motor_on;

    if (!motor_running)
    {
        if (bit_count > 0) {
            tape_pos++;
            bit_count = 0;
        }
    }

    else {
        read_header();
    }
}


void TapeTap::exec()
{
    if (!motor_running) {
        return;
    }

    if (tape_cycles_counter > 1) {
        tape_cycles_counter--;

        if (delay > 0) {
            --delay;
        }
        return;
    }

    tape_pulse ^= 0x01;
    via.write_cb1(tape_pulse);

    if (delay > 0) {
        if(--delay == 0)
        {
            if (tape_pulse) {
                delay = 1;
            }
            else {
                delay = 0;
            }
        }

        tape_cycles_counter = Pulse_1;
        return;
    }

    if (tape_pulse) {
        // Start of bit, pulse up.
        current_bit = get_current_bit();
        tape_cycles_counter = current_bit ? Pulse_1 : Pulse_0;
    }
    else {
        // Second part of bit, differently long down period.
        tape_cycles_counter = current_bit ? Pulse_1 : Pulse_0;
    }
}


uint8_t TapeTap::get_current_bit()
{
    uint8_t current_byte = data[tape_pos];

    uint8_t result;
    switch (bit_count) {
        case 0:
            result = 1;
            bit_count++;
            break;
        case 1:
            // Start bit (always 0).
            result = 0;
            parity = 1;
            bit_count++;
            break;
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
            result = (current_byte & (0x01 << (bit_count - 2))) ? 1 : 0;
            parity ^= result;
            bit_count++;
            break;
        case 10:
            // Parity.
            result = parity;
            bit_count++;
            break;
        case 11:
        case 12:
        case 13:
            // 3 stop bits (always 1).
            result = 1;

            bit_count = (bit_count + 1) % 14;

            if (bit_count == 0) {
                if (duplicate_bytes > 0) {
                    duplicate_bytes--;
                }
                else {
                    tape_pos++;
                }

                if (tape_pos == body_start) {
                    delay = 1281;
                }
            }
            break;
    }

    return result;
}


