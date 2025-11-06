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

// The 6522 has provision for two eight-bits IO ports (A and B) plus control
// lines (CA1, CA2, CB1, CB2), interfacing with the other components.
//
// Port A is used as a secondary bus : PA0 - PA7 lines connect both to the AY (sound)
// data bus and to the printer port. AY selection is done thanks to CA2 and CB2 lines.
//
// Port B provides many connections to the keyboard, tape, and printer port.


#ifndef MOS6502_H
#define MOS6502_H

#include "mos6502_opcodes.hpp"
#include "monitor.hpp"
#include "snapshot.hpp"

#include <set>


#define STACK_BOTTOM 0x0100

#define NMI_VECTOR_L 0xFFFA
#define NMI_VECTOR_H 0xFFFB

#define RESET_VECTOR_L 0xFFFC
#define RESET_VECTOR_H 0xFFFD

#define IRQ_VECTOR_L 0xFFFE
#define IRQ_VECTOR_H 0xFFFF

class Machine;
class MOS6502;
class Memory;

typedef uint8_t (*f_memory_read_byte_handler)(Machine &oric, uint16_t address);
typedef uint8_t (*f_memory_read_byte_zp_handler)(Machine &oric, uint8_t address);

typedef uint16_t (*f_memory_read_word_handler)(Machine &oric, uint16_t address);
typedef uint16_t (*f_memory_read_word_zp_handler)(Machine &oric, uint8_t address);

typedef void (*f_memory_write_byte_handler)(Machine &oric, uint16_t address, uint8_t val);
typedef void (*f_memory_write_byte_zp_handler)(Machine &oric, uint8_t address, uint8_t val);


class MOS6502
{
public:
    explicit MOS6502(Machine& a_Machine);
    ~MOS6502() = default;

    /**
     * Get debug monitor.
     * @return reference to debug monitor
     */
    Monitor& get_monitor() { return monitor; }

    /**
     * Set program counter address.
     * @param pc program counter address
     */
    void set_pc(uint16_t pc) { PC = pc; }

    /**
     * Get program counter address.
     * @return program counter address
     */
    [[nodiscard]] uint16_t get_pc() const { return PC; }

    /**
     * Get stack pointer address.
     * @return stack pointer address
     */
    [[nodiscard]] uint8_t get_sp() const { return SP; }

    /**
     * Get the P register (processor status).
     * @return P register
     */
    [[nodiscard]] uint8_t get_p() const;

    /**
     * Set the P register (processor status).
     * @param p new P register value
     */
    void set_p(uint8_t p);

    /**
     * Set quiet status.
     * @param val new quiet value
     */
    void set_quiet(bool val) { quiet = val; }

    /**
     * Reset the processor.
     */
    void Reset();

    /**
     * Print CPU status.
     */
    void PrintStat();

    /**
     * Calculate CPU cycles used by instruction at PC.
     * @return cycles used by instruction at PC
     */
    uint8_t time_instruction();

    /**
     * Execute instruction *cycle*.
     * @param break_on_brk if true then the CPU will break on executed BRK instruction
     * @param do_break reference to varianble set to true if break is triggered
     * @return true if instruction was executed (not all cycles execute full instruction)
     */
    bool exec(bool break_on_brk, bool& do_break);

    /**
     * Save CPU state to snapshot.
     * @param snapshot reference to snapshot
     */
    void save_to_snapshot(Snapshot& snapshot) const;

    /**
     * Load CPU state from snapshot.
     * @param snapshot reference to snapshot
     */
    void load_from_snapshot(Snapshot& snapshot);

    void set_breakpoint(uint16_t address);

    // The public exposure of variables like below is uncommon for normal projects,
    // but this is an emulator where the chips must be able to quickly access each
    // other without the overhead of getter functions, etc.

    // Registers
    uint8_t A;
    uint8_t X;
    uint8_t Y;

    // Flags
    //   7                           0
    // +---+---+---+---+---+---+---+---+
    // | N | V |   | B | D | I | Z | C |  <-- flag, 0/1 = reset/set
    // +---+---+---+---+---+---+---+---+
    uint8_t N_INTERN; // negative
    uint8_t Z_INTERN; // zero
    bool V; // overflow
    bool B; // break
    bool D; // decimal
    bool I; // interrupt
    bool C; // carry

    /**
     * Trigger NMI (Non Maskable Interrupt).
     */
    void NMI() { nmi_flag = true; }
    void irq() { irq_flag = true; }
    void irq_clear() { irq_flag = false; }

    f_memory_read_byte_handler memory_read_byte_handler;
    f_memory_read_byte_zp_handler memory_read_byte_zp_handler;

    f_memory_read_word_handler memory_read_word_handler;
    f_memory_read_word_handler memory_read_word_buggy_handler;
    f_memory_read_word_zp_handler memory_read_word_zp_handler;

    f_memory_write_byte_handler memory_write_byte_handler;
    f_memory_write_byte_zp_handler memory_write_byte_zp_handler;

protected:
    /**
     * Print status and instruction at given address.
     * @param address
     */
    void PrintStat(uint16_t address);

    /**
     * Implementation of ADC instruction.
     * @param value value to add
     */
    void ADC(uint8_t value);

    /**
     * Implementation of SBC instruction.
     * @param value value to substract
     */
    void SBC(uint8_t value);

    Machine& machine;
    Memory& memory;

    uint16_t PC;
    uint8_t SP;
    bool quiet;

    bool irq_flag;
    bool nmi_flag;
    bool do_interrupt;
    bool do_nmi;

    bool instruction_load;
    uint8_t instruction_cycles;
    uint8_t current_instruction;
    uint8_t current_cycle;

    Monitor monitor;

    std::set<uint16_t> breakpoints;
    bool has_breakpoints;
};

#endif // MOS6502_H
