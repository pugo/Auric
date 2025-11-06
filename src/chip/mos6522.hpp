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

#ifndef CHIP_MOS6522_H
#define CHIP_MOS6522_H

#include <map>
#include <string>

class Machine;
class Memory;
class Snapshot;


class MOS6522
{
public:
    enum Register {
        ORB   = 0x00,	// Output register B
        ORA   = 0x01,	// Output register A
        DDRB  = 0x02,	// Data direction port B
        DDRA  = 0x03,	// Data direction port A
        T1C_L = 0x04,	// Write: into T1 low latch.   Read: T1 low counter, reset interrupt flag.
        T1C_H = 0x05,	// Write: into T1 high latch+counter, transfer low.  Read: T1 high counter.
        T1L_L = 0x06,	// Write: into T1 low latch.   Read: T1 low latch.
        T1L_H = 0x07,	// Write: into T1 high latch.  Read: T2 high latch.
        T2C_L = 0x08,	// Write: into T2 low latch.   Read: T2 low counter, reset interrupt flag.
        T2C_H = 0x09,	// Write: into T2 high latch+counter, transfer low.  Read: T2 high counter.
        SR    = 0x0A,	// Shift Register
        ACR   = 0x0B,	// Auxilliary Control Register
        PCR   = 0x0C,	// Peripheral Control Register
        IFR   = 0x0D,	// Interrupt Flag Register
        IER   = 0x0E,	// Interrupt Enable Register
        IORA2 = 0x0F,
    };

    enum IRQ {
        IRQ_CA2  = 0x01,
        IRQ_CA1  = 0x02,
        IRQ_SR   = 0x04,
        IRQ_CB2  = 0x08,
        IRQ_CB1  = 0x10,
        IRQ_T2   = 0x20,
        IRQ_T1   = 0x40,
        IRQ_CTRL = 0x80
    };

    // PCR controlling bits.
    enum PCR {
        PCR_MASK_CA1 = 0x01,
        PCR_MASK_CA2 = 0x0E,
        PCR_MASK_CB1 = 0x10,
        PCR_MASK_CB2 = 0xE0
    };

    enum ACR {
        ACR_PA_LATCH_ENABLE = 0x01,
        ACR_PB_LATCH_ENABLE = 0x02
    };

    /**
     * State of a MOS6522.
     */
    struct State
    {
        bool ca1;
        bool ca2;
        bool ca2_do_pulse;

        bool cb1;
        bool cb2;
        bool cb2_do_pulse;

        uint8_t ira;		// Input Register A
        uint8_t ira_latch;  // Input Register A - input latch
        uint8_t ora;		// Output Register A
        uint8_t ddra;		// Data Direction Register A (input = 0, output = 1)

        uint8_t irb;		// Input Register B
        uint8_t irb_latch;  // Input Register B - input latch
        uint8_t orb;		// Output Register B
        uint8_t ddrb;		// Data Direction Register B (input = 0, output = 1)

        uint8_t t1_latch_low;
        uint8_t t1_latch_high;
        uint16_t t1_counter;
        bool t1_run;
        uint8_t t1_reload;

        uint8_t t2_latch_low;
        uint8_t t2_latch_high;
        uint16_t t2_counter;
        bool t2_run;
        bool t2_reload;

        uint8_t sr;              // Shift register
        uint8_t sr_counter;      // Modulo 8 counter for current bit
        uint16_t sr_timer;       // Time countdown
        bool sr_run;             // Set true by read/write to SR
        bool sr_first;           // Used to handle differences in shift cycles.
        bool sr_out_started;     // Used to handle initial shift out delay.
        bool sr_out_gap_pending; // Used to handle adding another counter period after each shift out byte.

        uint8_t acr;		    // Auxilliary control register (shift mode, etc)
        // |  7  |  6  |    5    |  4  |  3  |  2  |      1       |      0       |
        // |  T1 ctrl  | T2 ctrl |     SR  ctrl    | PB latch en. | PA latch en. |

        uint8_t pcr;		// Peripheral control register
        // |  7  |  6  |  5  |     4    |  3  |  2  |  1  |     0    |
        // |    CB2 ctrl     | CB1 ctrl |     CA2 ctrl    | CA1 ctrl |

        uint8_t ifr;		// Interrupt Flag Register:   | IRQ  | T1 | T2 | CB1 | CB2 | SR | CA1 | CA2 |
        uint8_t ier;		// Interrupt Enable Register: | ctrl | T1 | T2 | CB1 | CB2 | SR | CA1 | CA2 |

        void reset();
        void print() const;

        inline void sr_shift_in();
        inline void sr_shift_out();
        void sr_stop();
    };

    typedef void (*f_orb_changed_handler)(Machine &machine, uint8_t orb);
    typedef void (*f_ca2_changed_handler)(Machine &machine, bool value);
    typedef void (*f_cb2_changed_handler)(Machine &machine, bool aValue);
    typedef void (*f_psg_changed_handler)(Machine &machine);

    typedef void (*f_irq_handler)(Machine &machine);
    typedef void (*f_irq_clear_handler)(Machine &machine);

    explicit MOS6522(Machine& a_Machine);
    ~MOS6522() = default;

    /**
     * Execute one clock cycle.
     */
    void exec();

    /**
     * Save MOS 6522 state to snapshot.
     * @param snapshot reference to snapshot
     */
    void save_to_snapshot(Snapshot& snapshot);

    /**
     * Load MOS 6522 state from snapshot.
     * @param snapshot reference to snapshot
     */
    void load_from_snapshot(Snapshot& snapshot);

    /**
     * Read register value.
     * @param offset register to read
     * @return value of register
     */
    uint8_t read_byte(uint16_t offset);

    /**
     * Write register value.
     * @param offset register to write
     * @param value new value
     */
    void write_byte(uint16_t offset, uint8_t value);

    /**
     * Read output register A.
     * @return value of output register A, with correct data direction
     */
    [[nodiscard]] uint8_t read_ora() const { return (state.ora & state.ddra); }

    /**
     * Read output register B.
     * @return value of output register B, with correct data direction
     */
    [[nodiscard]] uint8_t read_orb() const { return (state.orb & state.ddrb); }

    /**
     * Set bit in input register A.
     * @param bit bit to set (0-7)
     * @param value value to set (boolean)
     */
    void set_ira_bit(uint8_t bit, bool value);

    /**
     * Set bit in input register B.
     * @param bit bit to set (0-7)
     * @param value value to set (boolean)
     */
    void set_irb_bit(uint8_t bit, bool value);

    /**
     * Set CA1 value
     * @param value CA1 value
     */
    void write_ca1(bool value);

    /**
     * Set CA2 value
     * @param value CA1 value
     */
    void write_ca2(bool value);

    /**
     * Set CB1 value
     * @param value CA1 value
     */
    void write_cb1(bool value);

    /**
     * Set CB2 value
     * @param value CA1 value
     */
    void write_cb2(bool value);

    /**
     * Return reference ot current MOS 6522 state.
     * @return reference to current MOS 6522 state
     */
    MOS6522::State& get_state() { return state; }

    /**
     * Get value of T1 counter.
     * @return value of T1 counter
     * Mainly used by unit tests to be able to get and set values without affecting interrupt flags.
     */
    [[nodiscard]] uint16_t get_t1_counter() const { return state.t1_counter; }

    /**
     * Get value of T2 counter.
     * @return value of T2 counter
     * Mainly used by unit tests to be able to get and set values without affecting interrupt flags.
     */
    [[nodiscard]] uint16_t get_t2_counter() const { return state.t2_counter; }

    /**
     * Set IFR (interrupt flag register) value
     * @param value new IFR value
     */
    void set_ifr(uint8_t value) { state.ifr = value; }

    f_orb_changed_handler orb_changed_handler;
    f_ca2_changed_handler ca2_changed_handler;
    f_cb2_changed_handler cb2_changed_handler;

    f_psg_changed_handler psg_changed_handler;

    f_irq_handler irq_handler;
    f_irq_clear_handler irq_clear_handler;

private:
    void irq_check();
    void irq_set(uint8_t bits);
    void irq_clear(uint8_t bits);
    bool sr_handle_counter();

    Machine& machine;
    MOS6522::State state;

    std::map<Register, std::string> register_names;
};


#endif // CHIP_MOS6522_H
