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
#include <fstream>
#include <cstdlib>

#include "tape_tap_normal.hpp"
#include "snapshot.hpp"


TapeTapNormal::TapeTapNormal(MOS6522& via, const std::filesystem::path& path) :
    TapeTap(via, path),
    current_byte(0),
    current_bit(0),
    parity(0),
    tape_cycle_counter(0),
    line_out(0)
{

}


void TapeTapNormal::reset()
{
    current_byte = 0;
    current_bit = 0;
    parity = 0;
    tape_cycle_counter = 0;
    line_out = 0;

    ::TapeTap::reset();
}


void TapeTapNormal::save_to_snapshot(Snapshot& snapshot) const
{
    TapeTap::save_to_snapshot(snapshot);
    snapshot.tape.current_byte = current_byte;
    snapshot.tape.current_bit = current_bit;
    snapshot.tape.parity = parity;
    snapshot.tape.tape_cycle_counter = tape_cycle_counter;
    snapshot.tape.gap_bits_remaining = gap_bits_remaining;
    snapshot.tape.line_out = line_out;
}


void TapeTapNormal::load_from_snapshot(const Snapshot& snapshot)
{
    TapeTap::load_from_snapshot(snapshot);
    current_byte = snapshot.tape.current_byte;
    current_bit = snapshot.tape.current_bit;
    parity = snapshot.tape.parity;
    tape_cycle_counter = snapshot.tape.tape_cycle_counter;
    gap_bits_remaining = snapshot.tape.gap_bits_remaining;
    line_out = snapshot.tape.line_out;
}


void TapeTapNormal::exec(uint8_t cycles)
{
    if (!motor_running) {
        return;
    }

    if (tape_state == TapeState::Idle || tape_state == TapeState::Fail) {
        return;
    }

    if (tape_state == TapeState::ParseHeader) {
        if (!parse_header()) {
            spdlog::error("Tape: failed to read header, stopping.");
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
    if (tape_cycle_counter > cycles) {
        tape_cycle_counter -= cycles;
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
            if (gap_bits_remaining < cycles) {
                gap_bits_remaining = 0;
                tape_state = TapeState::Body;
            }
            else {
                gap_bits_remaining -= cycles;
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

// Tape output is a delicate thing on the Oric. The below is not exactly what the ROM routines expect,
// but since many games use their own loader routined and expect slightly different timings and bit output
// this is a pattern that seems to work. The use of two initial bits is influenced by Oricutron.

uint8_t TapeTapNormal::next_bit()
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

