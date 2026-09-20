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

#include <filesystem>
#include <format>
#include <fstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "../src/config.hpp"
#include "../src/oric.hpp"
#include "../src/rom_patcher.hpp"
#include "../src/snapshot.hpp"
#include "../src/tape/tape_tap_turbo.hpp"

namespace {

constexpr RomPatch test_patch{
    .name = "Test ROM",
    .sha1 = "",
    .get_sync_pc = 0xe735,
    .get_sync_end_pc = 0xe759,
    .get_sync_loop_pc = 0xe720,
    .read_byte_pc = 0xe6c9,
    .read_byte_end_pc = 0xe6fb,
    .write_byte_pc = 0xe65e,
    .write_byte_end_pc = 0xe68a,
    .write_block_pc = 0xe607,
    .write_block_end_pc = 0xe644,
    .write_header_base_addr = 0x02a7,
    .write_name_addr = 0x027f,
    .write_data_start_addr = 0x02a9,
    .write_data_end_addr = 0x02ab,
    .basic_line_input_pc = std::nullopt,
    .read_byte_set_carry = false,
    .read_byte_store_byte_addr = 0x002f,
    .read_byte_store_zero_addr = 0x02b1,
};

std::filesystem::path test_tape_path(const std::string& name)
{
    const auto* test_info = testing::UnitTest::GetInstance()->current_test_info();
    return std::filesystem::temp_directory_path()
        / std::format("auric_{}_{}.tap", test_info->test_suite_name(), name);
}

std::vector<uint8_t> read_file(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(file), {}};
}

class TapeTapTurboTest : public testing::Test
{
protected:
    class TestTapeTapTurbo : public TapeTapTurbo
    {
    public:
        using TapeTapTurbo::TapeTapTurbo;

        void set_tape_pos(uint32_t pos)
        {
            tape_pos = pos;
        }

        uint32_t get_tape_pos() const
        {
            return tape_pos;
        }

        void set_snapshot_values()
        {
            motor_running = true;
            tape_state = TapeState::Body;
            tape_pos = 7;
            bit_index = 4;
            stopped_mid_byte = true;
            current_byte = 0x31;
            current_bit = 1;
            parity = 1;
            tape_cycle_counter = 123;
            gap_bits_remaining = 9;
            line_out = 1;
            turbo_loading = true;
            turbo_saving = true;
        }

        void clear_snapshot_values()
        {
            motor_running = false;
            tape_state = TapeState::Idle;
            tape_pos = 0;
            bit_index = 0;
            stopped_mid_byte = false;
            current_byte = 0;
            current_bit = 0;
            parity = 0;
            tape_cycle_counter = 0;
            gap_bits_remaining = 0;
            line_out = 0;
            turbo_loading = false;
            turbo_saving = false;
        }

        bool snapshot_values_match() const
        {
            return motor_running && tape_state == TapeState::Body && tape_pos == 7
                && bit_index == 4 && stopped_mid_byte && current_byte == 0x31
                && current_bit == 1 && parity == 1 && tape_cycle_counter == 123
                && gap_bits_remaining == 9 && line_out == 1
                && turbo_loading && turbo_saving;
        }
    };

    void SetUp() override
    {
        oric = std::make_unique<Oric>(config);
        oric->init_machine();
        oric->get_machine().init_cpu();
        oric->get_machine().init_mos6522();
    }

    void TearDown() override
    {
        if (!path.empty()) {
            std::filesystem::remove(path);
        }
    }

    TestTapeTapTurbo make_tape()
    {
        return TestTapeTapTurbo(*oric->get_machine().mos_6522, path, test_patch);
    }

    Config config;
    std::unique_ptr<Oric> oric;
    std::filesystem::path path;
};

} // namespace


TEST_F(TapeTapTurboTest, InitCreatesMissingTapFile)
{
    path = test_tape_path("create");
    std::filesystem::remove(path);

    auto tape = make_tape();

    EXPECT_TRUE(tape.init());
    EXPECT_TRUE(std::filesystem::exists(path));
    EXPECT_EQ(std::filesystem::file_size(path), 0);
}


TEST_F(TapeTapTurboTest, SnapshotRestoresPlaybackAndTurboState)
{
    path = test_tape_path("snapshot_state");
    std::ofstream(path, std::ios::binary).put(0x16);

    auto tape = make_tape();
    ASSERT_TRUE(tape.init());
    tape.set_snapshot_values();

    Snapshot snapshot;
    tape.save_to_snapshot(snapshot);
    tape.clear_snapshot_values();
    tape.load_from_snapshot(snapshot);

    EXPECT_EQ(snapshot.tape.kind, TapeSnapshotKind::TapTurbo);
    EXPECT_TRUE(tape.snapshot_values_match());
}


TEST_F(TapeTapTurboTest, SaveInterceptAppendsByteToTapFile)
{
    path = test_tape_path("append_one");
    std::filesystem::remove(path);

    auto tape = make_tape();
    ASSERT_TRUE(tape.init());
    tape.motor_on(true);

    auto& machine = oric->get_machine();
    machine.cpu->set_pc(test_patch.write_byte_pc);
    machine.cpu->A = 0x42;
    machine.cpu->X = 0x91;
    machine.cpu->Y = 0x37;

    EXPECT_TRUE(tape.intercept(*machine.cpu, machine.memory, true));
    tape.motor_on(false);
    EXPECT_EQ(read_file(path), std::vector<uint8_t>({0x42}));
    EXPECT_EQ(machine.memory.mem[0x002f], 0x42);
    EXPECT_EQ(machine.cpu->get_pc(), test_patch.write_byte_end_pc);
    EXPECT_EQ(machine.cpu->A, 0x91);
    EXPECT_EQ(machine.cpu->X, 0x91);
    EXPECT_EQ(machine.cpu->Y, 0x37);
    EXPECT_TRUE(machine.cpu->C);
}


TEST_F(TapeTapTurboTest, BlankTapDoesNotStopMotorBeforeSaveIntercept)
{
    path = test_tape_path("blank_save_leadin");
    std::filesystem::remove(path);

    auto tape = make_tape();
    ASSERT_TRUE(tape.init());
    tape.motor_on(true);

    tape.exec(6);

    auto& machine = oric->get_machine();
    machine.cpu->set_pc(test_patch.write_byte_pc);
    machine.cpu->A = 0x42;
    machine.cpu->X = 0x91;

    EXPECT_TRUE(tape.is_motor_running());
    EXPECT_TRUE(tape.intercept(*machine.cpu, machine.memory, true));
    tape.motor_on(false);
    EXPECT_EQ(read_file(path), std::vector<uint8_t>({0x42}));
}


TEST_F(TapeTapTurboTest, SaveInterceptOverwritesAtCurrentTapePosition)
{
    path = test_tape_path("overwrite_existing");
    {
        std::ofstream file(path, std::ios::binary);
        file.put(static_cast<char>(0x16));
        file.put(static_cast<char>(0x24));
    }

    auto tape = make_tape();
    ASSERT_TRUE(tape.init());
    tape.motor_on(true);

    auto& machine = oric->get_machine();
    machine.cpu->set_pc(test_patch.write_byte_pc);
    machine.cpu->A = 0xaa;

    EXPECT_TRUE(tape.intercept(*machine.cpu, machine.memory, true));
    tape.motor_on(false);
    EXPECT_EQ(read_file(path), std::vector<uint8_t>({0xaa, 0x24}));
}


TEST_F(TapeTapTurboTest, SaveInterceptWritesAtEndOfExistingTapFile)
{
    path = test_tape_path("write_at_end_existing");
    {
        std::ofstream file(path, std::ios::binary);
        file.put(static_cast<char>(0x16));
        file.put(static_cast<char>(0x24));
    }

    auto tape = make_tape();
    ASSERT_TRUE(tape.init());
    tape.set_tape_pos(2);
    tape.motor_on(true);
    tape.exec(6);

    auto& machine = oric->get_machine();
    machine.cpu->set_pc(test_patch.write_byte_pc);
    machine.cpu->A = 0xaa;

    EXPECT_TRUE(tape.is_motor_running());
    EXPECT_TRUE(tape.intercept(*machine.cpu, machine.memory, true));
    tape.motor_on(false);
    EXPECT_EQ(read_file(path), std::vector<uint8_t>({0x16, 0x24, 0xaa}));
}


TEST_F(TapeTapTurboTest, TurboExecContinuesNormalPlaybackAfterHeaderParse)
{
    path = test_tape_path("normal_playback_after_header");
    {
        std::ofstream file(path, std::ios::binary);
        file.put(static_cast<char>(0x16));
        file.put(static_cast<char>(0x16));
        file.put(static_cast<char>(0x16));
        file.put(static_cast<char>(0x24));
        file.put(static_cast<char>(0x00));
        file.put(static_cast<char>(0x00));
        file.put(static_cast<char>(0x00));
        file.put(static_cast<char>(0x00));
        file.put(static_cast<char>(0x12));
        file.put(static_cast<char>(0x34));
        file.put(static_cast<char>(0x12));
        file.put(static_cast<char>(0x34));
        file.put(static_cast<char>(0x00));
        file.put(static_cast<char>('A'));
        file.put(static_cast<char>(0x00));
        file.put(static_cast<char>(0x55));
    }

    auto tape = make_tape();
    ASSERT_TRUE(tape.init());
    tape.motor_on(true);

    for (int i = 0; i < 10000; ++i) {
        tape.exec(6);
    }

    EXPECT_TRUE(tape.is_motor_running());
    EXPECT_GT(tape.get_tape_pos(), 1);
}


TEST_F(TapeTapTurboTest, SaveAfterLoadedFileDoesNotStopMotorOnTrailingNonHeaderData)
{
    path = test_tape_path("save_after_load_trailing_data");
    {
        std::ofstream file(path, std::ios::binary);
        file.put(static_cast<char>(0x16));
        file.put(static_cast<char>(0x24));
        file.put(static_cast<char>(0x7f));
    }

    auto tape = make_tape();
    ASSERT_TRUE(tape.init());
    tape.set_tape_pos(2);
    tape.motor_on(true);
    tape.exec(6);

    auto& machine = oric->get_machine();
    machine.cpu->set_pc(test_patch.write_byte_pc);
    machine.cpu->A = 0xaa;

    EXPECT_TRUE(tape.is_motor_running());
    EXPECT_TRUE(tape.intercept(*machine.cpu, machine.memory, true));
    tape.motor_on(false);
    EXPECT_EQ(read_file(path), std::vector<uint8_t>({0x16, 0x24, 0xaa}));
}


TEST_F(TapeTapTurboTest, RewindThenSaveOverwritesWithoutTruncatingTrailingData)
{
    path = test_tape_path("rewind_overwrite");
    {
        std::ofstream file(path, std::ios::binary);
        file.put(static_cast<char>(0x11));
        file.put(static_cast<char>(0x22));
        file.put(static_cast<char>(0x33));
    }

    auto tape = make_tape();
    ASSERT_TRUE(tape.init());
    tape.set_tape_pos(3);
    tape.reset();
    tape.motor_on(true);

    auto& machine = oric->get_machine();
    machine.cpu->set_pc(test_patch.write_byte_pc);
    machine.cpu->A = 0xaa;

    EXPECT_TRUE(tape.intercept(*machine.cpu, machine.memory, true));
    tape.motor_on(false);
    EXPECT_EQ(read_file(path), std::vector<uint8_t>({0xaa, 0x22, 0x33}));
}


TEST_F(TapeTapTurboTest, SaveBlockInterceptWritesCompleteTapBlock)
{
    path = test_tape_path("block");
    std::filesystem::remove(path);

    auto tape = make_tape();
    ASSERT_TRUE(tape.init());
    tape.motor_on(true);

    auto& machine = oric->get_machine();
    machine.cpu->set_pc(test_patch.write_block_pc);

    machine.memory.mem[0x02b0] = 0x00;
    machine.memory.mem[0x02af] = 0x00;
    machine.memory.mem[0x02ae] = 0x00;
    machine.memory.mem[0x02ad] = 0x00;
    machine.memory.mem[0x02ac] = 0x12;
    machine.memory.mem[0x02ab] = 0x36;
    machine.memory.mem[0x02aa] = 0x12;
    machine.memory.mem[0x02a9] = 0x34;
    machine.memory.mem[0x02a8] = 0x00;

    machine.memory.mem[0x027f] = 'T';
    machine.memory.mem[0x0280] = 'E';
    machine.memory.mem[0x0281] = 'S';
    machine.memory.mem[0x0282] = 'T';
    machine.memory.mem[0x0283] = 0x00;

    machine.memory.mem[0x1234] = 0xde;
    machine.memory.mem[0x1235] = 0xad;
    machine.memory.mem[0x1236] = 0xbe;

    EXPECT_TRUE(tape.intercept(*machine.cpu, machine.memory, true));
    tape.motor_on(false);

    const auto bytes = read_file(path);
    ASSERT_EQ(bytes.size(), 192 + 1 + 9 + 5 + 3);
    EXPECT_EQ(bytes[0], 0x16);
    EXPECT_EQ(bytes[191], 0x16);
    EXPECT_EQ(bytes[192], 0x24);
    EXPECT_EQ(bytes[193 + 2], 0x00);
    EXPECT_EQ(bytes[193 + 5], 0x36);
    EXPECT_EQ(bytes[193 + 7], 0x34);
    EXPECT_EQ(bytes[202], 'T');
    EXPECT_EQ(bytes[206], 0x00);
    EXPECT_EQ(bytes[207], 0xde);
    EXPECT_EQ(bytes[208], 0xad);
    EXPECT_EQ(bytes[209], 0xbe);
    EXPECT_EQ(machine.cpu->get_pc(), test_patch.write_block_end_pc);
    EXPECT_TRUE(machine.cpu->C);
}


TEST_F(TapeTapTurboTest, SaveInterceptRequiresMotorAndOricRom)
{
    path = test_tape_path("guards");
    std::filesystem::remove(path);

    auto tape = make_tape();
    ASSERT_TRUE(tape.init());

    auto& machine = oric->get_machine();
    machine.cpu->set_pc(test_patch.write_byte_pc);
    machine.cpu->A = 0x55;

    EXPECT_FALSE(tape.intercept(*machine.cpu, machine.memory, true));
    tape.motor_on(true);
    EXPECT_FALSE(tape.intercept(*machine.cpu, machine.memory, false));
    EXPECT_EQ(read_file(path), std::vector<uint8_t>({}));
}


TEST(TapeRomPatchTest, SupportedRomsHaveWriteByteHooks)
{
    EXPECT_EQ(test_patch.write_byte_pc, 0xe65e);
    EXPECT_EQ(test_patch.write_byte_end_pc, 0xe68a);

    constexpr RomPatch basic10_patch{
        .name = "Oric-1 BASIC 1.0",
        .sha1 = "",
        .get_sync_pc = 0xe696,
        .get_sync_end_pc = 0xe6b9,
        .get_sync_loop_pc = 0xe681,
        .read_byte_pc = 0xe630,
        .read_byte_end_pc = 0xe65b,
        .write_byte_pc = 0xe5c6,
        .write_byte_end_pc = 0xe5f2,
        .write_block_pc = 0xe57b,
        .write_block_end_pc = 0xe5ba,
        .write_header_base_addr = 0x005d,
        .write_name_addr = 0x0035,
        .write_data_start_addr = 0x005f,
        .write_data_end_addr = 0x0061,
        .basic_line_input_pc = std::nullopt,
        .read_byte_set_carry = false,
        .read_byte_store_byte_addr = 0x002f,
        .read_byte_store_zero_addr = std::nullopt,
    };

    EXPECT_EQ(basic10_patch.write_byte_pc, 0xe5c6);
    EXPECT_EQ(basic10_patch.write_byte_end_pc, 0xe5f2);
    EXPECT_EQ(basic10_patch.write_block_pc, 0xe57b);
    EXPECT_EQ(basic10_patch.write_block_end_pc, 0xe5ba);
}
