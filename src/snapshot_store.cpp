// =========================================================================
//   Copyright (C) 2009-2026 by Anders Piniesjö <pugo@pugo.org>
//
//   This program is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 2 of the License, or
//   (at your option) any later version.
// =========================================================================

#include "snapshot_store.hpp"

#include <algorithm>
#include <array>
#include <fstream>
#include <format>
#include <limits>
#include <ranges>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <utility>

namespace
{

constexpr std::array<char, 8> file_magic{'A', 'U', 'R', 'I', 'C', 'S', 'N', 'P'};
constexpr uint32_t file_version = 1;
constexpr uint64_t maximum_vector_size = 256 * 1024 * 1024;

class Writer
{
public:
    explicit Writer(std::ostream& stream) : stream(stream) {}

    void bytes(const char* data, size_t size) { stream.write(data, static_cast<std::streamsize>(size)); }

    void u8(uint8_t value) { bytes(reinterpret_cast<const char*>(&value), sizeof(value)); }
    void boolean(bool value) { u8(value ? 1 : 0); }

    void u16(uint16_t value) { integer(value); }
    void u32(uint32_t value) { integer(value); }
    void u64(uint64_t value) { integer(value); }
    void i16(int16_t value) { u16(static_cast<uint16_t>(value)); }
    void i32(int32_t value) { u32(static_cast<uint32_t>(value)); }

    void string(const std::string& value)
    {
        u64(value.size());
        if (! value.empty()) {
            bytes(value.data(), value.size());
        }
    }

    void path(const std::filesystem::path& value) { string(value.string()); }

    void vector(const std::vector<uint8_t>& value)
    {
        u64(value.size());
        if (! value.empty()) {
            bytes(reinterpret_cast<const char*>(value.data()), value.size());
        }
    }

    bool good() const { return stream.good(); }

private:
    template<typename T>
    void integer(T value)
    {
        using Unsigned = std::make_unsigned_t<T>;
        const auto unsigned_value = static_cast<Unsigned>(value);
        for (size_t i = 0; i < sizeof(T); ++i) {
            u8(static_cast<uint8_t>(unsigned_value >> (i * 8)));
        }
    }

    std::ostream& stream;
};


class Reader
{
public:
    explicit Reader(std::istream& stream) : stream(stream) {}

    void bytes(char* data, size_t size)
    {
        stream.read(data, static_cast<std::streamsize>(size));
        if (! stream) {
            throw std::runtime_error("unexpected end of snapshot file");
        }
    }

    uint8_t u8()
    {
        uint8_t value{};
        bytes(reinterpret_cast<char*>(&value), sizeof(value));
        return value;
    }

    bool boolean()
    {
        const auto value = u8();
        if (value > 1) {
            throw std::runtime_error("invalid boolean in snapshot file");
        }
        return value != 0;
    }

    uint16_t u16() { return integer<uint16_t>(); }
    uint32_t u32() { return integer<uint32_t>(); }
    uint64_t u64() { return integer<uint64_t>(); }
    int16_t i16() { return static_cast<int16_t>(u16()); }
    int32_t i32() { return static_cast<int32_t>(u32()); }

    std::string string()
    {
        const auto size = u64();
        check_size(size);
        std::string value(size, '\0');
        if (size != 0) {
            bytes(value.data(), value.size());
        }
        return value;
    }

    std::filesystem::path path() { return std::filesystem::path(string()); }

    std::vector<uint8_t> vector()
    {
        const auto size = u64();
        check_size(size);
        std::vector<uint8_t> value(size);
        if (size != 0) {
            bytes(reinterpret_cast<char*>(value.data()), value.size());
        }
        return value;
    }

private:
    void check_size(uint64_t size)
    {
        if (size > maximum_vector_size || size > static_cast<uint64_t>(std::numeric_limits<size_t>::max())) {
            throw std::runtime_error("snapshot value is too large");
        }
    }

    template<typename T>
    T integer()
    {
        using Unsigned = std::make_unsigned_t<T>;
        Unsigned value = 0;
        for (size_t i = 0; i < sizeof(T); ++i) {
            value |= static_cast<Unsigned>(u8()) << (i * 8);
        }
        return static_cast<T>(value);
    }

    std::istream& stream;
};

void write_cpu(Writer& w, const MOS6502_state& s)
{
    w.u8(s.A);
    w.u8(s.X);
    w.u8(s.Y);
    w.u8(s.N_INTERN);
    w.u8(s.Z_INTERN);
    w.boolean(s.V);
    w.boolean(s.B);
    w.boolean(s.D);
    w.boolean(s.I);
    w.boolean(s.C);
    w.u16(s.PC);
    w.u8(s.SP);
    w.u8(s.irq_flags);
    w.boolean(s.nmi_flag);
    w.boolean(s.do_interrupt);
    w.boolean(s.do_nmi);
    w.boolean(s.instruction_load);
    w.u8(s.instruction_cycles);
    w.u8(s.current_instruction);
    w.u8(s.current_cycle);
}

void read_cpu(Reader& r, MOS6502_state& s)
{
    s.A = r.u8();
    s.X = r.u8();
    s.Y = r.u8();
    s.N_INTERN = r.u8();
    s.Z_INTERN = r.u8();
    s.V = r.boolean();
    s.B = r.boolean();
    s.D = r.boolean();
    s.I = r.boolean();
    s.C = r.boolean();
    s.PC = r.u16();
    s.SP = r.u8();
    s.irq_flags = r.u8();
    s.nmi_flag = r.boolean();
    s.do_interrupt = r.boolean();
    s.do_nmi = r.boolean();
    s.instruction_load = r.boolean();
    s.instruction_cycles = r.u8();
    s.current_instruction = r.u8();
    s.current_cycle = r.u8();
}

void write_via(Writer& w, const MOS6522::State& s)
{
    w.boolean(s.ca1);
    w.boolean(s.ca2);
    w.boolean(s.ca2_do_pulse);
    w.boolean(s.cb1);
    w.boolean(s.cb2);
    w.boolean(s.cb2_do_pulse);
    w.u8(s.ira);
    w.u8(s.ira_latch);
    w.u8(s.ora);
    w.u8(s.ddra);
    w.u8(s.irb);
    w.u8(s.irb_latch);
    w.u8(s.orb);
    w.u8(s.ddrb);
    w.u8(s.t1_latch_low);
    w.u8(s.t1_latch_high);
    w.u16(s.t1_counter);
    w.boolean(s.t1_run);
    w.u8(s.t1_reload);
    w.u8(s.t2_latch_low);
    w.u8(s.t2_latch_high);
    w.u16(s.t2_counter);
    w.boolean(s.t2_run);
    w.boolean(s.t2_reload);
    w.u8(s.sr);
    w.u8(s.sr_counter);
    w.u16(s.sr_timer);
    w.boolean(s.sr_run);
    w.boolean(s.sr_first);
    w.boolean(s.sr_out_started);
    w.boolean(s.sr_out_gap_pending);
    w.u8(s.acr);
    w.u8(s.pcr);
    w.u8(s.ifr);
    w.u8(s.ier);
}

void read_via(Reader& r, MOS6522::State& s)
{
    s.ca1 = r.boolean();
    s.ca2 = r.boolean();
    s.ca2_do_pulse = r.boolean();
    s.cb1 = r.boolean();
    s.cb2 = r.boolean();
    s.cb2_do_pulse = r.boolean();
    s.ira = r.u8();
    s.ira_latch = r.u8();
    s.ora = r.u8();
    s.ddra = r.u8();
    s.irb = r.u8();
    s.irb_latch = r.u8();
    s.orb = r.u8();
    s.ddrb = r.u8();
    s.t1_latch_low = r.u8();
    s.t1_latch_high = r.u8();
    s.t1_counter = r.u16();
    s.t1_run = r.boolean();
    s.t1_reload = r.u8();
    s.t2_latch_low = r.u8();
    s.t2_latch_high = r.u8();
    s.t2_counter = r.u16();
    s.t2_run = r.boolean();
    s.t2_reload = r.boolean();
    s.sr = r.u8();
    s.sr_counter = r.u8();
    s.sr_timer = r.u16();
    s.sr_run = r.boolean();
    s.sr_first = r.boolean();
    s.sr_out_started = r.boolean();
    s.sr_out_gap_pending = r.boolean();
    s.acr = r.u8();
    s.pcr = r.u8();
    s.ifr = r.u8();
    s.ier = r.u8();
}

void write_ay(Writer& w, const AY3_8912::SoundState& s)
{
    w.boolean(s.bdir); w.boolean(s.bc1); w.boolean(s.bc2); w.u8(s.current_register);
    for (auto value : s.registers) { w.u8(value); }
    for (auto value : s.audio_registers) { w.u8(value); }
    w.u32(s.audio_out);
    w.u32(s.changes.new_log_cycle); w.u32(s.changes.log_cycle); w.boolean(s.changes.update_log_cycle);
    w.u64(s.changes.buffer.size());
    for (size_t i = 0; i < s.changes.buffer.size(); ++i) {
        const auto& change = s.changes.buffer[i];
        w.u32(change.cycle); w.u8(change.register_index); w.u8(change.value);
    }
    for (const auto& channel : s.channels) {
        w.u16(channel.volume); w.u32(channel.tone_period); w.u32(channel.counter);
        w.u16(channel.value); w.u16(channel.output_bit); w.u8(channel.disabled);
        w.u16(channel.noise_diabled); w.boolean(channel.use_envelope);
    }
    w.u16(s.noise.output_bit); w.u16(s.noise.period); w.u32(s.noise.get_counter()); w.u32(s.noise.get_rng());
    w.u8(s.envelope.shape); w.u8(s.envelope.shape_counter);
    w.u32(s.envelope.get_period()); w.u32(s.envelope.get_counter());
    w.u32(s.cycles_per_sample); w.u32(s.cycle_count); w.u32(s.last_cycle);
}

void read_ay(Reader& r, AY3_8912::SoundState& s)
{
    s.bdir = r.boolean(); s.bc1 = r.boolean(); s.bc2 = r.boolean(); s.current_register = r.u8();
    for (auto& value : s.registers) { value = r.u8(); }
    for (auto& value : s.audio_registers) { value = r.u8(); }
    s.audio_out = r.u32();
    s.changes.new_log_cycle = r.u32(); s.changes.log_cycle = r.u32(); s.changes.update_log_cycle = r.boolean();
    const auto change_count = r.u64();
    if (change_count > register_changes_size) {
        throw std::runtime_error("too many AY register changes in snapshot");
    }
    s.changes.buffer.set_capacity(register_changes_size);
    for (uint64_t i = 0; i < change_count; ++i) {
        s.changes.buffer.push_back({r.u32(), r.u8(), r.u8()});
    }
    for (auto& channel : s.channels) {
        channel.volume = r.u16(); channel.tone_period = r.u32(); channel.counter = r.u32();
        channel.value = r.u16(); channel.output_bit = r.u16(); channel.disabled = r.u8();
        channel.noise_diabled = r.u16(); channel.use_envelope = r.boolean();
    }
    s.noise.output_bit = r.u16(); s.noise.period = r.u16();
    const auto noise_counter = r.u32(); const auto noise_rng = r.u32();
    s.noise.set_timing(noise_counter, noise_rng);
    s.envelope.shape = r.u8(); s.envelope.shape_counter = r.u8();
    s.envelope.set_timing(r.u32(), r.u32());
    s.cycles_per_sample = r.u32(); s.cycle_count = r.u32(); s.last_cycle = r.u32();
}

void write_wd(Writer& w, const WD1793::SnapshotState& s)
{
    w.u8(s.data); w.u8(s.side); w.u8(s.track); w.u8(s.sector); w.u8(s.command); w.u8(s.status);
    w.u8(s.current_track_number); w.u8(s.current_sector_number); w.u8(s.sector_type);
    w.i16(s.interrupt_counter); w.u8(s.status_at_interrupt); w.boolean(s.update_status_at_interrupt);
    w.i16(s.data_request_counter); w.u16(s.offset);
    for (auto value : s.address_data) { w.u8(value); }
    w.u8(static_cast<uint8_t>(s.operation)); w.boolean(s.multiple_sectors);
}

void read_wd(Reader& r, WD1793::SnapshotState& s)
{
    s.data = r.u8(); s.side = r.u8(); s.track = r.u8(); s.sector = r.u8(); s.command = r.u8(); s.status = r.u8();
    s.current_track_number = r.u8(); s.current_sector_number = r.u8(); s.sector_type = r.u8();
    s.interrupt_counter = r.i16(); s.status_at_interrupt = r.u8(); s.update_status_at_interrupt = r.boolean();
    s.data_request_counter = r.i16(); s.offset = r.u16();
    for (auto& value : s.address_data) { value = r.u8(); }
    const auto operation = r.u8();
    if (operation > static_cast<uint8_t>(WD1793::OperationType::WriteTrack)) {
        throw std::runtime_error("invalid WD1793 operation in snapshot");
    }
    s.operation = static_cast<WD1793::OperationType>(operation);
    s.multiple_sectors = r.boolean();
}

void write_tape(Writer& w, const TapeSnapshotState& s)
{
    w.u8(static_cast<uint8_t>(s.kind)); w.string(s.path); w.vector(s.data); w.boolean(s.motor_running);
    w.u8(s.tape_state); w.u32(s.sync_end); w.u32(s.body_start); w.u32(s.body_remaining); w.u16(s.leader_count);
    w.u32(s.tape_pos); w.u8(s.bit_index); w.boolean(s.stopped_mid_byte); w.u8(s.current_byte); w.u8(s.current_bit);
    w.u8(s.parity); w.i16(s.tape_cycle_counter); w.u8(s.gap_bits_remaining); w.u8(s.line_out);
    w.boolean(s.turbo_loading); w.boolean(s.turbo_saving);
}

void read_tape(Reader& r, TapeSnapshotState& s)
{
    const auto kind = r.u8();
    if (kind > static_cast<uint8_t>(TapeSnapshotKind::TapTurbo)) {
        throw std::runtime_error("invalid tape kind in snapshot");
    }
    s.kind = static_cast<TapeSnapshotKind>(kind); s.path = r.string(); s.data = r.vector(); s.motor_running = r.boolean();
    s.tape_state = r.u8(); s.sync_end = r.u32(); s.body_start = r.u32(); s.body_remaining = r.u32(); s.leader_count = r.u16();
    s.tape_pos = r.u32(); s.bit_index = r.u8(); s.stopped_mid_byte = r.boolean(); s.current_byte = r.u8(); s.current_bit = r.u8();
    s.parity = r.u8(); s.tape_cycle_counter = r.i16(); s.gap_bits_remaining = r.u8(); s.line_out = r.u8();
    s.turbo_loading = r.boolean(); s.turbo_saving = r.boolean();
}

void write_snapshot(Writer& w, const Snapshot& s)
{
    write_cpu(w, s.mos6502); write_via(w, s.mos6522); write_ay(w, s.ay3_8919); write_wd(w, s.wd1793);
    w.u8(s.drive_microdrive.drive_number); w.u8(s.drive_microdrive.status);
    w.u8(s.drive_microdrive.interrupt_request); w.u8(s.drive_microdrive.data_request);
    for (const auto& image : s.disk_images) {
        w.boolean(image.has_value());
        if (image) {
            w.path(image->path); w.vector(image->data); w.boolean(image->dirty);
        }
    }
    w.boolean(s.microdrive_present); write_tape(w, s.tape);
    w.boolean(s.oric_rom_enabled); w.boolean(s.disk_rom_enabled); w.i32(s.cycle_count); w.u8(s.current_key_row);
    for (auto value : s.key_rows) { w.u8(value); }
    w.u8(s.ula.video_attrib); w.u8(s.ula.text_attrib); w.u16(s.ula.raster_current);
    w.u8(s.ula.warpmode_counter); w.u8(s.ula.blink); w.u32(s.ula.frame_count); w.vector(s.ula.pixels);
    w.vector(s.memory);
}

void read_snapshot(Reader& r, Snapshot& s)
{
    read_cpu(r, s.mos6502); read_via(r, s.mos6522); read_ay(r, s.ay3_8919); read_wd(r, s.wd1793);
    s.drive_microdrive.drive_number = r.u8(); s.drive_microdrive.status = r.u8();
    s.drive_microdrive.interrupt_request = r.u8(); s.drive_microdrive.data_request = r.u8();
    for (auto& image : s.disk_images) {
        if (r.boolean()) {
            image = DiskImage::SnapshotState{r.path(), r.vector(), r.boolean()};
        }
        else {
            image.reset();
        }
    }
    s.microdrive_present = r.boolean(); read_tape(r, s.tape);
    s.oric_rom_enabled = r.boolean(); s.disk_rom_enabled = r.boolean(); s.cycle_count = r.i32(); s.current_key_row = r.u8();
    for (auto& value : s.key_rows) { value = r.u8(); }
    s.ula.video_attrib = r.u8(); s.ula.text_attrib = r.u8(); s.ula.raster_current = r.u16();
    s.ula.warpmode_counter = r.u8(); s.ula.blink = r.u8(); s.ula.frame_count = r.u32(); s.ula.pixels = r.vector();
    s.memory = r.vector();
}

std::string normalize_path(const std::string& value)
{
    if (value.empty()) {
        return {};
    }

    std::error_code error;
    auto path = std::filesystem::absolute(std::filesystem::path(value), error);
    if (error) {
        path = std::filesystem::path(value);
    }
    path = path.lexically_normal();
    return path.generic_string();
}

uint64_t context_hash(const SnapshotContext& context)
{
    uint64_t hash = 14695981039346656037ull;
    const auto add = [&hash](std::string_view value) {
        for (const auto character : value) {
            hash ^= static_cast<uint8_t>(character);
            hash *= 1099511628211ull;
        }
        hash ^= 0xff;
        hash *= 1099511628211ull;
    };

    add(context.tape);
    for (const auto& disk : context.disks) { add(disk); }
    return hash;
}

void write_context(Writer& w, const SnapshotContext& context)
{
    w.string(context.tape);
    for (const auto& disk : context.disks) { w.string(disk); }
}

SnapshotContext read_context(Reader& r)
{
    SnapshotContext context;
    context.tape = r.string();
    for (auto& disk : context.disks) { disk = r.string(); }
    return context;
}

} // namespace


SnapshotStore::SnapshotStore(std::filesystem::path directory) :
    directory(std::move(directory))
{
}


SnapshotContext SnapshotStore::context_from_snapshot(const Snapshot& snapshot)
{
    SnapshotContext context;
    if (snapshot.tape.kind != TapeSnapshotKind::Blank) {
        context.tape = normalize_path(snapshot.tape.path);
    }

    for (size_t i = 0; i < snapshot.disk_images.size(); ++i) {
        if (snapshot.disk_images[i]) {
            context.disks[i] = normalize_path(snapshot.disk_images[i]->path.string());
        }
    }
    return context;
}


std::filesystem::path SnapshotStore::slot_path(const SnapshotContext& context) const
{
    const auto filename = context.tape.empty() && std::ranges::all_of(context.disks, [](const auto& path) { return path.empty(); })
        ? "general.snap"
        : std::format("slot-{:016x}.snap", context_hash(context));
    return directory / filename;
}


bool SnapshotStore::save(const Snapshot& snapshot, const SnapshotContext& context, std::string& error) const
{
    error.clear();
    try {
        std::filesystem::create_directories(directory);
        const auto target = slot_path(context);
        const auto temporary = target.string() + ".tmp";

        std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
        if (! output) {
            error = std::format("unable to open '{}' for writing", target.string());
            return false;
        }

        Writer writer(output);
        writer.bytes(file_magic.data(), file_magic.size());
        writer.u32(file_version);
        write_context(writer, context);
        write_snapshot(writer, snapshot);
        output.flush();

        if (! writer.good()) {
            error = std::format("unable to write '{}'", target.string());
            output.close();
            std::filesystem::remove(temporary);
            return false;
        }

        output.close();
        std::filesystem::rename(temporary, target);
        return true;
    }
    catch (const std::exception& exception) {
        error = exception.what();
        return false;
    }
}


std::optional<Snapshot> SnapshotStore::load(const SnapshotContext& context, std::string& error) const
{
    error.clear();
    const auto target = slot_path(context);
    std::ifstream input(target, std::ios::binary);
    if (! input) {
        return std::nullopt;
    }

    try {
        Reader reader(input);
        std::array<char, file_magic.size()> magic{};
        reader.bytes(magic.data(), magic.size());

        if (magic != file_magic) {
            throw std::runtime_error("invalid snapshot file signature");
        }

        if (reader.u32() != file_version) {
            throw std::runtime_error("unsupported snapshot file version");
        }

        const auto stored_context = read_context(reader);
        if (stored_context != context) {
            throw std::runtime_error("snapshot media context does not match the current media");
        }

        Snapshot snapshot;
        read_snapshot(reader, snapshot);
        return snapshot;
    }
    catch (const std::exception& exception) {
        error = exception.what();
        return std::nullopt;
    }
}
