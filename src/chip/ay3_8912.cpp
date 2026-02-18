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

#include <machine.hpp>

#include "ay3_8912.hpp"


// Registers
//              0, 1 | Lowest 12 bits = pitch channel A 
//              2, 3 | Lowest 12 bits = pitch channel B
//              4, 5 | Lowest 12 bits = pitch channel C
//                 6 | Lowest 5 bits = pitch of noise channel
//
//                 7 | Enablers: bit 6 = port A output/input
//                   |           bits 5, 4, 3 = ^enable noise channels C B A
//                   |           bits 2, 1, 0 = ^enable tone channels C B A
//
//                 8 | Amplitude channel A: bit 4 = amplitude envelope mode, bits 3-0 = fixed level
//                 9 | Amplitude channel B: bit 4 = amplitude envelope mode, bits 3-0 = fixed level
//                10 | Amplitude channel C: bit 4 = amplitude envelope mode, bits 3-0 = fixed level
//
//                11 | Envelope period fine tune
//                12 | Envelope period coarse tune
//                13 | Envelope shape/cycle control
//
//                14 | I/O port A

//                                     |   7  |   6  |   5  |   4  |   3  |   2  |   1  |   0  |
//     0     Channel A Tone Period     |                  8 bit fine tune A                    |
//     1                               |                           |        coarse tune A      |
//     2     Channel B Tone Period     |                  8 bit fine tune B                    |
//     3                               |                           |        coarse tune B      |
//     4     Channel C Tone Period     |                  8 bit fine tune C                    |
//     5                               |                           |        coarse tune C      |
//     6     Noise Period              |                    |       5 bit period control       |
//     7     Enable (inv)              |    IN~/OUT  |       Noise~       |        Tone~       |
//                                     |  IOB |  IOA |   C  |   B  |   A  |   C  |   B  |   A  |
//     8     Channel A Amplitude       |                    |   M  |  L3  |  L2  |  L1  |  L0  |
//     9     Channel B Amplitude       |                    |   M  |  L3  |  L2  |  L1  |  L0  |
//     A     Channel C Amplitude       |                    |   M  |  L3  |  L2  |  L1  |  L0  |
//     B     Envelope Period           |                  8 bit fine tune E                    |
//     C                               |                  8 bit Coarse tune E                  |
//     D     Envelope shape/cycle      |                           | CONT |  ATT |  ALT | HOLD |
//     E     IO Port A Data Store      |                8 bit parallel IO on port A            |
//     F     IO Port B Data Store      |                8 bit parallel IO on port B            |


// BDIR   BC2   BC1
//   0     1     0     Inactive
//   0     1     1     Read from PSG
//   1     1     0     Write to PSG
//   1     1     1     Latch address


using namespace std;

// Volume table from Oricutron.
uint32_t voltab[] = {0, 513/4, 828/4, 1239/4, 1923/4, 3238/4, 4926/4, 9110/4, 10344/4, 17876/4, 24682/4, 30442/4, 38844/4, 47270/4, 56402/4, 65535/4};

constexpr uint8_t cycle_shift = 12;
constexpr uint32_t cycles_per_second = 998400;
constexpr uint32_t audio_frequency = 44100;


// ------- Channel -------------------------------------------------------------------------

Channel::Channel() :
    volume(0),
    tone_period(0),
    counter(0),
    value(0),
    output_bit(0),
    disabled(1),
    noise_diabled(1),
    use_envelope(false)
{}

void Channel::reset()
{
    volume = 0;
    tone_period = 0;
    counter = 0;
    value = 0;
    output_bit = 0;
    disabled = 1;
    noise_diabled = 1;
    use_envelope = false;
}

// ------- Noise ---------------------------------------------------------------------------

Noise::Noise() :
    period(0),
    counter(0),
    output_bit(0),
    rng(1)
{}

void Noise::reset()
{
    period = 0;
    counter = 0;
    output_bit = 0;
    rng = 1;
}

// ------- Envelope ------------------------------------------------------------------------

Envelope::Envelope() :
    shape(0),
    shape_counter(0),
    period(0),
    counter(0)
{}

void Envelope::reset()
{
    shape = 0;
    shape_counter = 0;
    period = 0;
    counter = 0;
}

void Envelope::set_envelope_shape(uint8_t new_shape)
{
    shape = new_shape;
    counter = 0;
    shape_counter = 0;
}

// ------- RegisterChanges -----------------------------------------------------------------

RegisterChanges::RegisterChanges() :
    log_cycle(0),
    new_log_cycle(0),
    update_log_cycle(false)
{
    buffer.set_capacity(register_changes_size);
}

void RegisterChanges::reset()
{
    log_cycle = 0;
    new_log_cycle = 0;
    update_log_cycle = false;
    buffer.set_capacity(register_changes_size);
}

// ------- AY3_8912 ------------------------------------------------------------------------

AY3_8912::SoundState::SoundState() :
    bdir(false),
    bc1(false),
    bc2(false),
    current_register(0),
    audio_out(0),
    cycle_count(0),
    last_cycle(0),
    cycles_per_sample((cycles_per_second << cycle_shift) / audio_frequency)
{
    // Reset all registers.
    for (auto& i : registers) { i = 0; }
    for (auto& i : audio_registers) { i = 0; }

    // Reset all tone and noise periods.
    for (auto& c : channels) { c.reset(); }
    noise.reset();
}

void AY3_8912::SoundState::reset()
{
    bdir = false;
    bc1 = false;
    bc2 = false;

    current_register = 0;
    audio_out = 0;

    cycle_count = 0;
    last_cycle = 0;

    cycles_per_sample = ((cycles_per_second << cycle_shift) / audio_frequency);

    // Reset all registers.
    for (auto& i : registers) { i = 0; }
    for (auto& i : audio_registers) { i = 0; }

    // Reset all tone and noise periods.
    for (auto& c : channels) { c.reset(); }
    noise.reset();
}

void AY3_8912::SoundState::print_status()
{
    std::println("AY-3-8912 state:");
    for(uint8_t c=0; c < 3; c++) {
        channels[c].print_status(c);
    }
    noise.print_status();
}

void AY3_8912::SoundState::write_register_change(uint8_t value)
{
    if (changes.update_log_cycle) {
        changes.log_cycle = changes.new_log_cycle;
        changes.update_log_cycle = false;
    }

    changes.buffer.push_back({changes.log_cycle, current_register, value});
}

void AY3_8912::SoundState::trim_register_changes()
{
    // Change change cycles to be relative to local counting.
    for (auto& rc : changes.buffer) {
        if (rc.cycle > last_cycle)
            rc.cycle -= last_cycle;
        else
            rc.cycle = 0;
    }

    if (changes.buffer.size() > 200) {
        for (auto& rc : changes.buffer) {
            exec_register_change(rc);
        }
        changes.buffer.clear();
    }
}

void AY3_8912::SoundState::exec_register_change(RegisterChange& change)
{
    switch (change.register_index) {
        case CH_A_PERIOD_LOW:
        case CH_A_PERIOD_HIGH:
            audio_registers[change.register_index] = change.value;
            channels[0].tone_period = (((audio_registers[CH_A_PERIOD_HIGH] & 0x0f) << 8) + audio_registers[CH_A_PERIOD_LOW]) * 8;
            if (channels[0].tone_period == 0) { channels[0].tone_period = 1; }
            break;
        case CH_B_PERIOD_LOW:
        case CH_B_PERIOD_HIGH:
            audio_registers[change.register_index] = change.value;
            channels[1].tone_period = (((audio_registers[CH_B_PERIOD_HIGH] & 0x0f) << 8) + audio_registers[CH_B_PERIOD_LOW]) * 8;
            if (channels[1].tone_period == 0) { channels[1].tone_period = 1; }
            break;
        case CH_C_PERIOD_LOW:
        case CH_C_PERIOD_HIGH:
            audio_registers[change.register_index] = change.value;
            channels[2].tone_period = (((audio_registers[CH_C_PERIOD_HIGH] & 0x0f) << 8) + audio_registers[CH_C_PERIOD_LOW]) * 8;
            if (channels[2].tone_period == 0) { channels[2].tone_period = 1; }
            break;

        case NOICE_PERIOD:
            audio_registers[change.register_index] = change.value;
            noise.period = (change.value & 0x1f) * 8;
            break;

        case ENABLE:
            audio_registers[change.register_index] = change.value;
            channels[0].disabled = (change.value & 0x01) ? 1 : 0;
            channels[1].disabled = (change.value & 0x02) ? 1 : 0;
            channels[2].disabled = (change.value & 0x04) ? 1 : 0;
            channels[0].noise_diabled = (change.value & 0x08) ? 1 : 0;
            channels[1].noise_diabled = (change.value & 0x10) ? 1 : 0;
            channels[2].noise_diabled = (change.value & 0x20) ? 1 : 0;
            break;

        case CH_A_AMPLITUDE:
            audio_registers[change.register_index] = change.value;
            channels[0].use_envelope = (change.value & 0x10) != 0;
            if (channels[0].use_envelope) {
                channels[0].volume = voltab[envelope_shapes[envelope.shape][envelope.shape_counter]];
            }
            else {
                channels[0].volume = voltab[change.value & 0x0f];
            }
            break;
        case CH_B_AMPLITUDE:
            audio_registers[change.register_index] = change.value;
            channels[1].use_envelope = (change.value & 0x10) != 0;
            if (channels[1].use_envelope) {
                channels[1].volume = voltab[envelope_shapes[envelope.shape][envelope.shape_counter]];
            }
            else {
                channels[1].volume = voltab[change.value & 0x0f];
            }
            break;
        case CH_C_AMPLITUDE:
            audio_registers[change.register_index] = change.value;
            channels[2].use_envelope = (change.value & 0x10) != 0;
            if (channels[2].use_envelope) {
                channels[2].volume = voltab[envelope_shapes[envelope.shape][envelope.shape_counter]];
            }
            else {
                channels[2].volume = voltab[change.value & 0x0f];
            }
            break;
        case ENV_DURATION_LOW:
        case ENV_DURATION_HIGH:
            audio_registers[change.register_index] = change.value;
            envelope.set_period(((audio_registers[ENV_DURATION_HIGH] << 8) + audio_registers[ENV_DURATION_LOW]) * 16);
            break;
        case ENV_SHAPE:
            if (change.value == 0xff) {
                break;
            }
            audio_registers[change.register_index] = change.value;
            envelope.set_envelope_shape(change.value & 0x0f);

            for (uint8_t channel = 0; channel < 3; channel++) {
                if (channels[channel].use_envelope) {
                    channels[channel].volume = voltab[envelope_shapes[envelope.shape][envelope.shape_counter]];
                }
            }
            break;
    }
}

void AY3_8912::SoundState::exec_audio(uint32_t cycle)
{
    if (cycle <= last_cycle) { return; }

    const uint16_t cycles = cycle - last_cycle;
    int32_t out = 0;

    for (uint16_t c = 0; c < cycles; c++) {
        channels[0].exec_cycle();
        channels[1].exec_cycle();
        channels[2].exec_cycle();
        noise.exec_cycle();

        // Envelope
        if (envelope.exec_cycle()) {
            for (uint8_t channel = 0; channel < 3; channel++) {
                if (channels[channel].use_envelope) {
                    channels[channel].volume = voltab[envelope_shapes[envelope.shape][envelope.shape_counter]];
                }
            }
        }

        for (auto& channel : channels) {
            const uint32_t tone_gate  = (channel.output_bit | channel.disabled);
            const uint32_t noise_gate = (noise.output_bit | channel.noise_diabled);
            channel.value = (tone_gate & noise_gate) ? channel.volume : 0;
            out += channel.value;
        }

    }

    out = out / cycles;

    if (out > 32767) { out = 32767; }
    audio_out = out;

    last_cycle = cycle;
}


AY3_8912::AY3_8912(Machine& machine) :
    machine(machine),
    m_read_data_handler(nullptr)
{}

void AY3_8912::reset()
{
    state.reset();
}

void AY3_8912::print_status()
{
    state.print_status();
}

void AY3_8912::save_to_snapshot(Snapshot& snapshot)
{
    snapshot.ay3_8919 = state;
}

void AY3_8912::load_from_snapshot(Snapshot& snapshot)
{
    state = snapshot.ay3_8919;
}

void AY3_8912::exec(uint8_t cycles)
{
    state.changes.exec(cycles);
}

void AY3_8912::update_state()
{
    if (state.bdir) {
        if (state.bc1) {  // 1 ? 1
            // Latch address: read address from data bus.
            if (uint8_t new_curr = m_read_data_handler(machine); new_curr < NUM_REGS) {
                state.current_register = new_curr;
            }
        }
        else {  // 1 ? 0
            uint8_t value = m_read_data_handler(machine);
            // Write to PSG: read value from data bus to current register.

            state.registers[state.current_register] = value;

            switch (state.current_register) {
                case ENABLE:
                case CH_A_PERIOD_LOW:
                case CH_A_PERIOD_HIGH:
                case CH_B_PERIOD_LOW:
                case CH_B_PERIOD_HIGH:
                case CH_C_PERIOD_LOW:
                case CH_C_PERIOD_HIGH:
                case NOICE_PERIOD:
                case CH_A_AMPLITUDE:
                case CH_B_AMPLITUDE:
                case CH_C_AMPLITUDE:
                case ENV_DURATION_LOW:
                case ENV_DURATION_HIGH:
                case ENV_SHAPE:
                    if (! machine.warpmode_on) {
                        machine.frontend->lock_audio();
                        state.write_register_change(value);
                        machine.frontend->unlock_audio();
                    }
                    break;
                case IO_PORT_A:
                    break;
            }

        }
    }
    else {
        if (state.bc1) {
            // Read from PSG. -- not yet implemented
        }
    }
}

void AY3_8912::set_bdir(bool value)
{
    state.bdir = value;
}

void AY3_8912::set_bc1(bool value)
{
    state.bc1 = value;
}

void AY3_8912::set_bc2(bool value)
{
    state.bc2 = value;
}

void AY3_8912::set_bdir_callback(Machine& machine, bool value) {
    machine.ay3->set_bdir(value);
}

void AY3_8912::set_bc1_callback(Machine& machine, bool value)
{
    machine.ay3->set_bc1(value);
}

void AY3_8912::set_bc2_callback(Machine& machine, bool value)
{
    machine.ay3->set_bc2(value);
}

void AY3_8912::update_state_callback(Machine& machine)
{
    machine.ay3->update_state();
}

void AY3_8912::audio_callback(void* user_data, uint8_t* raw_buffer, int len)
{
    AY3_8912* ay = reinterpret_cast<AY3_8912*>(user_data);
    uint16_t* buffer = (uint16_t*)raw_buffer;
    uint16_t samples = len/4;

    uint32_t current_sample = 0;

    if (ay->machine.warpmode_on) return;

    for (size_t sample = 0; sample < samples; sample++) {
        uint32_t current_cycle = ay->state.cycle_count >> cycle_shift;

        ay->state.exec_register_changes(current_cycle);
        ay->state.exec_audio(current_cycle);

        buffer[current_sample++] = ay->state.audio_out;
        buffer[current_sample++] = ay->state.audio_out;

        ay->state.cycle_count += ay->state.cycles_per_sample;
    }

    ay->state.trim_register_changes();

    ay->state.cycle_count -= ay->state.last_cycle << cycle_shift;
    ay->state.last_cycle = 0;

    ay->state.changes.new_log_cycle = ay->state.cycle_count >> cycle_shift;
    ay->state.changes.update_log_cycle = true;
}
