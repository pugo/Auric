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

#ifndef AY3_8912_H
#define AY3_8912_H

#include <print>
#include <boost/circular_buffer.hpp>

class Snapshot;
class Machine;

typedef uint8_t (*f_read_data_handler)(Machine &oric);
typedef uint8_t (*f_write_data_handler)(Machine &oric);

constexpr size_t register_changes_size = 32768;


// ------- Channel -------------------------------------------------------------------------

class Channel
{
public:
    Channel();

    void reset();

    void exec_cycle()
    {
        if (tone_period == 0) {
            output_bit = 1;
            return;
        }

        if (++counter >= tone_period) {
            counter = 0;
            output_bit ^= 1;
        }
    }

    void print_status(uint8_t channel) const {
        std::println(" ------- Channel {} -------------------------", channel);
        std::println("           Volume: {} ", volume);
        std::println("      Tone period: {} ", tone_period);
        std::println("          Counter: {} ", counter);
        std::println("            Value: {} ", value);
        std::println("         Disabled: {} ", disabled);
        std::println("   Noise disabled: {} ", noise_diabled);
        std::println("     Use envelope: {} ", use_envelope);
        std::println();
    }

    uint16_t volume;
    uint32_t tone_period;
    uint32_t counter;
    uint16_t value;
    uint16_t output_bit;
    uint8_t disabled;
    uint16_t noise_diabled;
    bool use_envelope;
};

// ------- Noise ---------------------------------------------------------------------------

class Noise
{
public:
    Noise();

    void reset();

    void exec_cycle()
    {
        if (++counter >= period) {
            counter = 0;

            uint32_t right_bit = (rng & 1) ^ ((rng >> 2) & 1);
            rng = (rng >> 1) | (right_bit << 16);
            output_bit ^= (right_bit & 1);
        }
    }

    void print_status() const {
        std::println(" ------- Noise -------------------------");
        std::println("      Period: {}", period);
        std::println("     Counter: {}", counter);
        std::println("  Output bit: {}", output_bit);
        std::println("         Rng: {}", rng);
        std::println();
    }

    uint16_t output_bit;
    uint16_t period;

private:
        uint16_t counter;
    uint32_t rng;
};


// ------- Envelope ------------------------------------------------------------------------

constexpr uint8_t env_goto = 0x80;
using envelope_shape_t = std::vector<uint8_t>;

static const std::vector<envelope_shape_t> envelope_shapes = {
    // 0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F  10
    // CONTINUE ATTACK ALTERNATE HOLD
    // 0 0 X X
    { 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0,     env_goto | 0xf },
    { 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0,     env_goto | 0xf },
    { 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0,     env_goto | 0xf },
    { 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0,     env_goto | 0xf },
    // 0 1 X X
    {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,  0, env_goto | 0x10 },
    {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,  0, env_goto | 0x10 },
    {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,  0, env_goto | 0x10 },
    {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,  0, env_goto | 0x10 },
    // 1 0 0 0
    { 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0,     env_goto | 0},
    // 1 0 0 1
    { 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0,     env_goto | 0xf },
    // 1 0 1 0
    { 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, env_goto | 0 },
    // 1 0 1 1
    { 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0, 15, env_goto | 0x10 },
    // 1 1 0 0
    {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,     env_goto | 0 },
    // 1 1 0 1
    {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,     env_goto | 0xf },
    // 1 1 1 0
    {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0, env_goto | 0 },
    // 1 1 1 1
    {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,  0, env_goto | 0x10 },
};

class Envelope
{
public:
    Envelope();

    void reset();

    bool exec_cycle()
    {
        if (++counter >= period) {
            counter = 0;

            ++shape_counter;
            if (envelope_shapes[shape][shape_counter] & env_goto) {
                shape_counter = envelope_shapes[shape][shape_counter] & 0x7f;
            }
            return true;
        }
        return false;
    }

    void set_period(uint16_t value)
    {
        period = value;
    }

    void set_envelope_shape(uint8_t new_shape);

    uint8_t shape;
    uint8_t shape_counter;

private:
    uint32_t period;
    uint32_t counter;
};


// ------- RegisterChanges -----------------------------------------------------------------

struct RegisterChange
{
    uint32_t cycle;
    uint8_t register_index;
    uint8_t value;
};


class RegisterChanges
{
public:
    RegisterChanges();

    void reset();

    void exec(uint8_t cycles)
    {
        if (update_log_cycle) {
            log_cycle = new_log_cycle;
            update_log_cycle = false;
        }

        log_cycle += cycles;
    }

    boost::circular_buffer<RegisterChange> buffer;

    uint32_t new_log_cycle;
    uint32_t log_cycle;
    bool update_log_cycle;
};


// ------- AY3_8912 ------------------------------------------------------------------------

class AY3_8912
{
public:
    enum Register
    {
        CH_A_PERIOD_LOW = 0,
        CH_A_PERIOD_HIGH,
        CH_B_PERIOD_LOW,
        CH_B_PERIOD_HIGH,
        CH_C_PERIOD_LOW,
        CH_C_PERIOD_HIGH,
        NOICE_PERIOD,
        ENABLE,
        CH_A_AMPLITUDE,
        CH_B_AMPLITUDE,
        CH_C_AMPLITUDE,
        ENV_DURATION_LOW,
        ENV_DURATION_HIGH,
        ENV_SHAPE,
        IO_PORT_A,
        NUM_REGS
    };

    struct SoundState
    {
        SoundState();
        ~SoundState() = default;

        void reset();
        void print_status();

        /**
         * Write a register change to array of changes.
         * @param value register change to write
         */
        void write_register_change(uint8_t value);

        /**
         * Execute register changes.
         * @param cycle current cycle
         */
        void exec_register_changes(uint32_t cycle) {
            while (!changes.buffer.empty() && cycle >= changes.buffer[0].cycle) {
                exec_register_change(changes.buffer[0]);
                changes.buffer.pop_front();
            }
        }

        /**
         * Execute one register change
         * @param change change to execute
         */
        void exec_register_change(RegisterChange& change);

        /**
         * Trim the array of register changes.
         */
        void trim_register_changes();

        /**
         * Execute audio a number of clock cycles.
         * @param cycle number of cycles to execute.
         */
        void exec_audio(uint32_t cycle);

        bool bdir;
        bool bc1;
        bool bc2;

        uint8_t current_register;
        uint8_t registers[NUM_REGS];
        uint8_t audio_registers[NUM_REGS];
        uint32_t audio_out;

        RegisterChanges changes;
        Channel channels[3];
        Noise noise;
        Envelope envelope;

        uint32_t cycles_per_sample;
        uint32_t cycle_count;
        uint32_t last_cycle;
    };


    explicit AY3_8912(Machine& machine);

    ~AY3_8912() = default;

    /**
     * Reset the AY-3-8912.
     */
    void reset();

    /**
     * Print AY-3-8912 state to console.
     */
    void print_status();

    /**
     * Save AY-3-8912 state to snapshot.
     * @param snapshot reference to snapshot
     */
    void save_to_snapshot(Snapshot& snapshot);

    /**
     * Load AY-3-8912 state from snapshot.
     * @param snapshot reference to snapshot
     */
    void load_from_snapshot(Snapshot& snapshot);

    /**
     * Execute a number of clock cycles.
     */
    void exec(uint8_t cycles);

    /**
     * Update AY state based on BC1 and BDIR.
     */
    void update_state();

    /**
     * Set bus direction pin value.
     * @param value bus direction pin value
     */
    void set_bdir(bool value);

    /**
     * Set BC1 pin value.
     * @param value BC1 pin value
     */
    void set_bc1(bool value);

    /**
     * Set BC2 pin value.
     * @param value BC1 pin value
     */
    void set_bc2(bool value);

    /**
     * Get value of specified register
     * @param reg register to get
     * @return register value
     */
    uint8_t get_register(Register reg) { return state.registers[reg]; }

    /**
     * Set bus direction pin value - callback function.
     * @param machine Machine object for current machine
     * @param value new value
     */
    static void set_bdir_callback(Machine& machine, bool value);

    /**
     * Set BC1 pin value - callback function.
     * @param machine Machine object for current machine
     * @param value new value
     */
    static void set_bc1_callback(Machine& machine, bool value);

    /**
     * Set BC2 pin value - callback function.
     * @param machine Machine object for current machine
     * @param value new value
     */
    static void set_bc2_callback(Machine& machine, bool value);

    /**
     * Update state - callback function.
     * @param machine Machine object for current machine
     */
    static void update_state_callback(Machine& machine);

    /**
     * Called by frontend to get audio.
     * @param user_data pointer to opaque AY3_8912 pointer
     * @param raw_buffer buffer to fill with data
     * @param len buffer length
     */
    static void audio_callback(void* user_data, uint8_t* raw_buffer, int len);

//    void write_register_change(RegisterChange& register_change);

    f_read_data_handler m_read_data_handler;
    f_write_data_handler m_write_data_handler;

private:
    Machine& machine;
    SoundState state;
};

#endif // AY3_8912_H
