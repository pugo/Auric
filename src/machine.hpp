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

#ifndef MACHINE_H
#define MACHINE_H

#include <memory>
#include <chrono>
#include <optional>

#include "chip/mos6502.hpp"
#include "chip/mos6522.hpp"
#include "chip/ay3_8912.hpp"
#include "chip/ula.hpp"
#include "memory.hpp"
#include "monitor.hpp"
#include "snapshot.hpp"
#include "tape/tape.hpp"

class Oric;
class Frontend;
class AY3_8912;


class Machine
{
public:
    explicit Machine(Oric& oric);
    ~Machine() = default;

    /**
     * Init the machine.
     * @param frontend pointer to Frontend object
     */
    void init(Frontend* frontend);

    /**
     * Init the CPU.
     */
    void init_cpu();

    /**
     * Init the MOS 6522 (VIA).
     */
    void init_mos6522();

    /**
     * Init the AY3 sound chip.
     */
    void init_ay3();

    /**
     * Import the disk support.
     */
    void init_disk();

    /**
     * Import the tape support.
     */
    void init_tape();

    /**
     * Reset the machine.
     */
    void reset() const;

    /**
     * Get debug monitor.
     * @return reference to debug monitor
     */
    Monitor& get_monitor()
    {
        return monitor;
    }

    /**
     * Run the machine.
     * @param oric Pointer to Oric object
     */
    void run(Oric* oric);

    /**
     * Run the machine from given address.
     * @param address address to run from
     * @param oric Pointer to Oric object
     */
    void run(uint16_t address, Oric* oric) { cpu->set_pc(address); run(oric); }

    /**
     * Stop the machine.
     */
    void stop() { break_exec = true; }

    /**
     * Trigger CPU IRQ.
     */
    void irq() const { cpu->irq(); }

    /**
     * Clear CPU IRQ.
     */
    void irq_clear() const { cpu->irq_clear(); }

    /**
     * Handle key press.
     * @param key_bits key code
     * @param down true if key down, false if key up
     */
    void key_press(uint8_t key_bits, bool down);

    /**
     * Update key output to other circuits.
     */
    void update_key_output();

    /**
     * Called on VIA ORB changed.
     * @param orb new ORB value
     */
    void via_orb_changed(uint8_t orb);

    /**
     * Save snapshot of all to RAM.
     */
    void save_snapshot();

    /**
     * Load snapshot of all from RAM.
     */
    void load_snapshot();

    /**
     * Toggle warp mode on and off.
     * @return true if warp mode is on
     */
    bool toggle_warp_mode();

    /**
     * Set whether to disassemble executed instructions.
     * @param disassemble true to disassemble executed instructions
     */
    void set_disassemble_execution(bool disassemble)
    {
        disassemble_execution = disassemble;
    }

    /**
     * Print CPU status.
     */
    void PrintStat();

    // --- Memory functions -------------------

    static uint8_t read_byte(Machine& machine, uint16_t address)
    {
        if (!machine.oric_rom_enabled) {
            if (machine.disk_rom_enabled && address >= 0xe000) {
                return machine.disk_rom.mem[address - 0xe000];
            }
        }
        else {
            if (address >= 0xc000) {
                return machine.oric_rom.mem[address - 0xc000];
            }
        }

        if (address >= 0x300 && address < 0x400) {
            if (address >= 0x310 && address < 0x31c) {
                std::println("WD1793 read");
                return 0x0;
            }

            return machine.mos_6522->read_byte(address);
        }

        return machine.memory.mem[address];
    }

    static uint8_t read_byte_zp(Machine &machine, uint8_t address)
    {
        return machine.memory.mem[address];
    }

    static uint16_t read_word(Machine& m, uint16_t addr)
    {
        return read_byte(m, addr) | (read_byte(m, addr + 1) << 8);
    }

    static uint16_t read_word_zp(Machine &machine, uint8_t address)
    {
        return machine.memory.mem[address] | (machine.memory.mem[address + 1 & 0xff] << 8);
    }

    static void write_byte(Machine &machine, uint16_t address, uint8_t val)
    {
        if (! machine.oric_rom_enabled) {
            if (machine.disk_rom_enabled && address >= 0xe000) {
                return;
            }
        }
        else {
            if (address >= 0xc000) {
                return;
            }
        }

        if (address >= 0x300 && address < 0x400) {
            if (address >= 0x310 && address < 0x31c) {
                std::println
                ("WD1793 write: {:04x} <- {:02x}", address, val);
                if (address == 0x314) {
                    machine.oric_rom_enabled = val & 0x02;
                }
                return;
            }

            machine.mos_6522->write_byte(address, val);
            return;
        }

        machine.memory.mem[address] = val;
    }

    static void write_byte_zp(Machine &machine, uint8_t address, uint8_t val)
    {
        if (address > 0x00ff) {
            return;
        }
        machine.memory.mem[address] = val;
    }

    static uint8_t read_via_ora(Machine& machine)
    {
        return machine.mos_6522->read_ora();
    }

    static uint8_t read_via_orb(Machine& machine)
    {
        return machine.mos_6522->read_orb();
    }

    static void via_orb_changed_callback(Machine& machine, uint8_t orb)
    {
        machine.via_orb_changed(orb);
    }

    static void irq_callback(Machine& machine)
    {
        machine.irq();
    }

    static void irq_clear_callback(Machine& machine)
    {
        machine.irq_clear();
    }

    std::unique_ptr<MOS6502> cpu;
    std::unique_ptr<MOS6522> mos_6522;
    std::unique_ptr<AY3_8912> ay3;

    bool break_exec;
    Memory memory;
    Memory oric_rom;
    Memory disk_rom;
    bool oric_rom_enabled;
    bool disk_rom_enabled;

    Frontend* frontend;
    bool warpmode_on;

protected:
    /**
     * Print status and instruction at given address.
     * @param address
     */
    void PrintStat(uint16_t address);

    ULA ula;
    Oric& oric;
    Monitor monitor;

    std::unique_ptr<Tape> tape;

    bool disassemble_execution;
    int32_t cycle_count;
    std::chrono::high_resolution_clock::time_point next_frame_tp;

    bool sound_paused;
    uint32_t sound_pause_counter;

    uint8_t current_key_row;
    uint8_t key_rows[8];

    std::optional<Snapshot> snapshot;
};

#endif // MACHINE_H
