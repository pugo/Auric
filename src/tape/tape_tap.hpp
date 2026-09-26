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

#ifndef TAPE_TAP_H
#define TAPE_TAP_H

#include <filesystem>
#include <fstream>
#include <optional>
#include <span>

#include "chip/mos6522.hpp"
#include "tape.hpp"


class TapeTap : public Tape
{
public:
    enum class TapeState {
        Idle,
        ParseHeader,
        Leader,
        Header,
        Gap,
        Body,
        EndOfBlock,
        Fail
    };

    struct TapeHeader {
        uint16_t sync_len;
        uint8_t file_type;
        uint8_t auto_flag;
        uint16_t start_addr, end_addr;
        std::string name;
    };

    TapeTap(MOS6522& via, const std::filesystem::path& path);

    virtual ~TapeTap();

    /**
     * Initialize tape.
     * @return true on success
     */
    bool init() override;

    void save_to_snapshot(Snapshot& snapshot) const override;
    void load_from_snapshot(const Snapshot& snapshot) override;
    /**
     * Reset tape postion.
     */
    void reset() override;

    /**
     * Print tape status to console.
     */
    void print_stat() override;

    /**
     * Set motor state.
     * @param motor_on true if motor is on
     */
    void motor_on(bool motor_on) override;

    /**
     * Intercept a ROM tape routine before CPU opcode fetch. Allows turbo loading and saving.
     * @param cpu reference to CPU
     * @param ram reference to RAM
     * @param oric_rom_enabled true if Oric ROM is enabled
     * @return true if the tape handled this CPU step.
     */
    bool intercept(MOS6502& cpu, Memory& ram, bool oric_rom_enabled) override { return false; };

protected:
    /**
     * Check if TAP data is available.
     * @return true if a TAP file has been loaded.
     */
    bool has_tap_data() const;

    /**
     * Check if a TAP header starts at the current tape position.
     * @return true if a loadable header is available at the current position.
     */
    bool has_tap_header_at_current_pos() const;

    /**
     * Seek from the current tape position to the next TAP sync marker.
     * @return true if a valid sync marker was found.
     */
    bool seek_next_sync();

    /**
     * Read the next logical TAP byte and advance the tape position.
     * @return next byte if available.
     */
    std::optional<uint8_t> read_next_tap_byte();

    /**
     * Write one logical TAP byte to the tape image and in-memory data.
     * @param byte byte to write
     * @return true if the byte was written.
     */
    bool write_tap_byte(uint8_t byte);

    /**
     * Write logical TAP bytes to the tape image and in-memory data.
     * @param bytes bytes to write
     * @return true if the bytes were written.
     */
    bool write_tap_bytes(std::span<const uint8_t> bytes);

    /**
     * Close the TAP write stream if saving has opened it.
     */
    void close_tap_write_file();

    /**
     * Read tape header.
     * @return true if header is valid
     */
    bool parse_header();

    /**
     * Count sync bytes.
     * @return number of sync bytes
     */
    size_t count_sync_bytes();

    /**
     * Validate header start.
     * @param pos header start position
     * @return true if header start is valid
     */
    bool validate_header_start(size_t pos);

    /**
     * Log file type info.
     * @param file_type file type byte
     * @param auto_flag auto flag byte
     */
    void log_file_type(uint8_t file_type, uint8_t auto_flag);

    /**
     * Read a null-terminated string from tape.
     * @param offset offset to read from. Updated to end of string.
     * @return read string
     */
    std::string read_null_terminated_string(size_t& offset) const;

    std::filesystem::path path;
    MOS6522& via;
    size_t tape_size;

    TapeState tape_state;

    uint32_t sync_end;
    uint32_t body_start;
    uint32_t body_remaining;
    uint16_t leader_count;

    uint32_t tape_pos;
    uint8_t bit_index;

    bool stopped_mid_byte;

    std::vector<uint8_t> memory_vector;
    uint8_t* data;
    std::fstream write_file;

    static const int Pulse_1 = 208;
    static const int Pulse_0 = 416;
};

#endif // TAPE_TAP_H
