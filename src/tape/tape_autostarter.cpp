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

#include <print>
#include <spdlog/spdlog.h>

#include "tape_autostarter.hpp"

#include "../machine.hpp"


TapeAutostarter::TapeAutostarter() :
    autostart_state(AutostartState::WaitingForBasicInput),
    cload_input_index(0),
    delay_counter(0)
{
    spdlog::info("Tape autostarter: will autostart loaded tape");
}

bool TapeAutostarter::exec(Machine& machine, Tape& tape)
{
    if (!machine.rom_patch || !machine.oric_rom_enabled || machine.disk_rom_enabled) {
        return false;
    }

    // WaitingForBasicInput
    if (autostart_state == AutostartState::WaitingForBasicInput) {
        if (machine.cpu->get_pc() != *machine.rom_patch->basic_line_input_pc) {
            return true;
        }

        spdlog::debug("Tape autostarter: key input ready");
        delay_counter = 3;
        autostart_state = AutostartState::PressKey;
    }

    // PressKey
    else if (autostart_state == AutostartState::PressKey) {
        if (cload_input_index < cload_input_length) {
            if (delay_counter != 0) {
                delay_counter--;
                return true;
            }

            const uint8_t key = cload_input[cload_input_index];
            if (key & cload_input_shift) {
                spdlog::debug("Tape autostarter: pressing shift");
                machine.key_press(36, true);
            }

            spdlog::debug("Tape autostarter: pressing key {}", key & 0x7F);
            machine.key_press(key & 0x7F, true);

            delay_counter = 1;
            autostart_state = AutostartState::ReleaseKey;
            return true;
        }
        else {
            return false;
        }
    }

    // PressKey
    else if (autostart_state == AutostartState::ReleaseKey) {
        if (cload_input_index < cload_input_length) {
            if (delay_counter != 0) {
                delay_counter--;
                return true;
            }

            const uint8_t key = cload_input[cload_input_index];
            if (key & cload_input_shift) {
                spdlog::debug("Tape autostarter: releasing shift");
                machine.key_press(36, false);
            }

            spdlog::debug("Tape autostarter: releasing key {}", key & 0x7F);
            machine.key_press(key & 0x7F, false);
            cload_input_index++;
            delay_counter = 2;
            autostart_state = AutostartState::PressKey;
            return true;
        }
    }

    return true;
}