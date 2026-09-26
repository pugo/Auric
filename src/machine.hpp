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

#ifndef MACHINE_H
#define MACHINE_H

#include <chrono>
#include <deque>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "chip/mos6502.hpp"
#include "chip/mos6522.hpp"
#include "chip/ay3_8912.hpp"
#include "chip/ula.hpp"
#include "memory.hpp"
#include "monitor.hpp"
#include "rom_patcher.hpp"
#include "snapshot.hpp"
#include "snapshot_store.hpp"
#include "tape/tape.hpp"
#include "tape/tape_autostarter.hpp"
#include "disk/drive.hpp"

class Oric;
class Frontend;
class AY3_8912;
class TapeTapTurbo;


class Machine
{
public:
    struct Watchpoint
    {
        uint16_t start{0};
        uint16_t end{0};
        bool on_read{false};
        bool on_write{true};
        bool enabled{true};
    };

    struct WatchpointHit
    {
        uint16_t address{0};
        uint8_t value{0};
        uint16_t pc{0};
        bool write{false};
    };

    struct TraceEntry
    {
        uint64_t cycle{0};
        uint16_t pc{0};
        uint8_t cycles{0};
        std::string instruction;
    };

    explicit Machine(Oric& oric);
    ~Machine() = default;

    /**
     * Reset the machine.
     */
    void reset();

    /**
     * Reset the CPU.
     */
    void reset_cpu();

    /**
     * Init the machine.
     * @param frontend pointer to Frontend object
     */
    void init(Frontend* frontend);

    /**
     * Init the storage components.
     */
    void init_storage();

    /**
     * Init the RAM.
     */
    void init_ram();

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
     * Get debug monitor.
     * @return reference to debug monitor
     */
    Monitor& get_monitor()
    {
        return monitor;
    }

    /**
     * Set whether the Oric ROM is enabled.
     * @param enabled true to enable Oric ROM
     */
    void set_oric_rom_enabled(bool enabled)
    {
        oric_rom_enabled = enabled;
    }

    /**
     * Set whether the diskdrive ROM is enabled.
     * @param enabled true to enable diskdrive ROM
     */
    void set_diskdrive_rom_enabled(bool enabled)
    {
        disk_rom_enabled = enabled;
    }

    /**
     * Run the machine.
     * @param oric Pointer to Oric object
     */
    void run(Oric* oric);
    void run_until_frame_or_break(Oric* oric);
    void render_current_frame();

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
    void clear_stop() { break_exec = false; }

    /**
     * Trigger CPU IRQ.
     */
    void set_irq_source(uint8_t source) { cpu->set_irq_source(source); }

    /**
     * Clear CPU IRQ.
     */
    void clear_irq_source(uint8_t source) { cpu->clear_irq_source(source); }

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

    void insert_tape(std::filesystem::path path);
    void eject_tape();
    void rewind_tape();

    void insert_disk(std::filesystem::path path, uint8_t drive_number = 0);
    void eject_disk(uint8_t drive_number);

    Drive* get_disk_drive() { return disk.get(); }
    Tape* get_tape() { return tape.get(); }

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
    std::string format_stat();

    /**
     * Get the total number of CPU cycles.
     * @return total cycles
     */
    uint64_t get_total_cycles() const { return total_cycles; }

    /**
     * Set whether to enable instruction trace logging.
     * @param enabled true to enable instruction trace logging
     */
    void set_trace_enabled(bool enabled) { trace_enabled = enabled; }

    /**
     * Check if instruction trace logging is enabled.
     * @return true if instruction trace logging is enabled
     */
    bool trace_is_enabled() const { return trace_enabled; }

    /**
     * Clear the instruction trace log.
     */
    void clear_trace() { trace_log.clear(); }

    /**
     * Get the instruction trace log.
     * @return reference to instruction trace log
     */
    const std::deque<TraceEntry>& get_trace() const { return trace_log; }

    /**
     * Add a watchpoint.
     * @param start start address of watchpoint
     * @param end end address of watchpoint
     * @param on_read true to trigger on read access
     * @param on_write true to trigger on write access
     * @return index of the added watchpoint
     */
    size_t add_watchpoint(uint16_t start, uint16_t end, bool on_read, bool on_write);

    /**
     * Remove a watchpoint by index.
     * @param index index of the watchpoint to remove
     * @return true if the watchpoint was removed, false if index is invalid
     */
    bool remove_watchpoint(size_t index);

    /**
     * Enable or disable a watchpoint by index.
     * @param index index of the watchpoint to enable/disable
     * @param enabled true to enable, false to disable
     * @return true if the watchpoint was updated, false if index is invalid
     */
    bool set_watchpoint_enabled(size_t index, bool enabled);

    /**
     * Get the list of watchpoints.
     * @return reference to the vector of watchpoints
     */
    const std::vector<Watchpoint>& get_watchpoints() const { return watchpoints; }

    /**
     * Take the last watchpoint hit, if any.
     * @return optional WatchpointHit if a watchpoint was hit, std::nullopt otherwise
     */
    std::optional<WatchpointHit> take_watchpoint_hit();


    /**
     * Read a byte from memory.
     * @param address address to read from
     * @return value read from memory
     */
    uint8_t peek_byte(uint16_t address) const { return memory.mem[address]; }

    /**
     * Write a byte to memory.
     * @param address address to write to
     * @param value value to write
     */
    void poke_byte(uint16_t address, uint8_t value) { memory.mem[address] = value; }

    // --- Memory functions -------------------

    static uint8_t read_byte(Machine& machine, uint16_t address)
    {
        machine.record_bus_access(address, false, 0);
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
                return machine.disk->read_byte(address - 0x310);
            }

            return machine.mos_6522->read_byte(address);
        }

        return machine.memory.mem[address];
    }

    static uint8_t read_byte_no_watchpoint(Machine& machine, uint16_t address)
    {
        if (!machine.oric_rom_enabled) {
            if (machine.disk_rom_enabled && address >= 0xe000) {
                return machine.disk_rom.mem[address - 0xe000];
            }
        }
        else if (address >= 0xc000) {
            return machine.oric_rom.mem[address - 0xc000];
        }

        if (address >= 0x300 && address < 0x400) {
            if (address >= 0x310 && address < 0x31c) {
                return machine.disk->read_byte(address - 0x310);
            }
            return machine.mos_6522->read_byte(address);
        }

        return machine.memory.mem[address];
    }

    static uint8_t read_byte_zp(Machine &machine, uint8_t address)
    {
        machine.record_bus_access(address, false, 0);
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
        machine.record_bus_access(address, true, val);
        if (! machine.oric_rom_enabled) {
            if (machine.disk_rom_enabled && address >= 0xe000) {
                return;  // Can't write into disk ROM.
            }
        }
        else {
            if (address >= 0xc000) {
                return;  // Can't write into BASIC ROM.
            }
        }

        if (address >= 0x300 && address < 0x400) {
            if (address >= 0x310 && address < 0x31c) {
                machine.disk->write_byte(address - 0x310, val);
                return;
            }

            machine.mos_6522->write_byte(address, val);
            return;
        }

        machine.memory.mem[address] = val;
    }

    static void write_byte_zp(Machine &machine, uint8_t address, uint8_t val)
    {
        machine.record_bus_access(address, true, val);
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
        machine.set_irq_source(IRQ_SOURCE_VIA);
    }

    static void irq_clear_callback(Machine& machine)
    {
        machine.clear_irq_source(IRQ_SOURCE_VIA);
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
    const RomPatch* rom_patch;

    Frontend* frontend;
    bool warpmode_on;

protected:
    /**
     * Print status and instruction at given address.
     * @param address
     */
    void PrintStat(uint16_t address);
    std::string format_stat(uint16_t address);
    bool try_tape_turbo_intercept();
    bool load_tape(std::filesystem::path path);
    Snapshot capture_snapshot();
    SnapshotContext current_snapshot_context();
    void restore_snapshot(Snapshot& saved_snapshot);

    ULA ula;
    Oric& oric;
    Monitor monitor;

    std::unique_ptr<Drive> disk;
    std::unique_ptr<Tape> tape;

    bool has_tape_turbo;
    std::unique_ptr<TapeAutostarter> tape_autostarter;

    bool disassemble_execution;
    int32_t cycle_count;
    uint64_t total_cycles;
    bool trace_enabled{false};
    std::deque<TraceEntry> trace_log;
    std::chrono::high_resolution_clock::time_point next_frame_tp;
    bool frame_timer_initialized;

    bool sound_paused;
    uint32_t sound_pause_counter;

    uint8_t current_key_row;
    uint8_t key_rows[8];

    std::optional<Snapshot> snapshot;

    std::vector<Watchpoint> watchpoints;
    std::optional<WatchpointHit> watchpoint_hit;

    void record_bus_access(uint16_t address, bool write, uint8_t value);
    void append_trace(uint16_t pc, uint8_t cycles);
};

#endif // MACHINE_H
