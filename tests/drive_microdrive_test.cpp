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

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <vector>

#include <gtest/gtest.h>

#include "../src/config.hpp"
#include "../src/disk/drive_microdrive.hpp"
#include "../src/oric.hpp"
#include "../src/snapshot.hpp"

namespace {

constexpr uint32_t track_size = 6400;
constexpr uint32_t header_size = 256;

void write32(std::vector<uint8_t>& data, size_t offset, uint32_t value)
{
    data[offset] = static_cast<uint8_t>(value & 0xff);
    data[offset + 1] = static_cast<uint8_t>((value >> 8) & 0xff);
    data[offset + 2] = static_cast<uint8_t>((value >> 16) & 0xff);
    data[offset + 3] = static_cast<uint8_t>((value >> 24) & 0xff);
}

std::filesystem::path create_test_disk_image(uint8_t fill)
{
    auto path = std::filesystem::temp_directory_path()
              / ("oric_microdrive_test_"
                 + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())
                 + ".mfm");

    std::vector<uint8_t> data(header_size + track_size, 0x4e);
    std::copy_n("MFM_DISK", 8, data.begin());
    write32(data, 8, 1);   // sides
    write32(data, 12, 1);  // tracks
    write32(data, 16, 0);  // geometry

    auto pos = header_size;
    data[pos + 0] = 0xa1;
    data[pos + 1] = 0xa1;
    data[pos + 2] = 0xa1;
    data[pos + 3] = 0xfe;
    data[pos + 4] = 0x00;  // track
    data[pos + 5] = 0x00;  // side
    data[pos + 6] = 0x01;  // sector
    data[pos + 7] = 0x01;  // 256 bytes
    data[pos + 8] = 0x00;  // CRC placeholder
    data[pos + 9] = 0x00;

    pos += 10;
    data[pos] = 0xfb;
    std::fill_n(data.begin() + pos + 1, 256, fill);
    data[pos + 257] = 0x00;
    data[pos + 258] = 0x00;

    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));

    return path;
}

class DriveMicrodriveTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        oric = std::make_unique<Oric>(config);
        oric->init_machine();
        oric->get_machine().init_cpu();
    }

    void TearDown() override
    {
        oric.reset();
        for (const auto& path : paths) {
            std::filesystem::remove(path);
        }
    }

    std::filesystem::path make_disk(uint8_t fill)
    {
        auto path = create_test_disk_image(fill);
        paths.push_back(path);
        return path;
    }

    Config config;
    std::unique_ptr<Oric> oric;
    std::vector<std::filesystem::path> paths;
};

} // namespace

TEST_F(DriveMicrodriveTest, KeepsSeparateImagesPerDrive)
{
    DriveMicrodrive drive(oric->get_machine());

    ASSERT_TRUE(drive.insert_disk(make_disk(0x11), 0));
    auto* drive0_image = drive.get_disk_image();
    ASSERT_NE(drive0_image, nullptr);

    ASSERT_TRUE(drive.insert_disk(make_disk(0x22), 1));
    EXPECT_EQ(drive.get_disk_image(), drive0_image);

    drive.write_byte(0x4, 0x20);
    auto* drive1_image = drive.get_disk_image();
    ASSERT_NE(drive1_image, nullptr);
    EXPECT_NE(drive1_image, drive0_image);

    drive.write_byte(0x4, 0x00);
    EXPECT_EQ(drive.get_disk_image(), drive0_image);
}


TEST_F(DriveMicrodriveTest, SnapshotRestoresDiskMediaBytes)
{
    DriveMicrodrive drive(oric->get_machine());
    ASSERT_TRUE(drive.insert_disk(make_disk(0x11), 0));

    Snapshot snapshot;
    drive.save_to_snapshot(snapshot);

    auto* sector = drive.get_disk_image()->get_track(0, 0)->get_sector(1);
    ASSERT_NE(sector, nullptr);
    ASSERT_FALSE(sector->data.empty());
    sector->data[0] = 0x99;

    drive.load_from_snapshot(snapshot);

    auto* restored_sector = drive.get_disk_image()->get_track(0, 0)->get_sector(1);
    ASSERT_NE(restored_sector, nullptr);
    EXPECT_EQ(restored_sector->data[0], 0x11);
}

TEST_F(DriveMicrodriveTest, MachineDiskInsertKeepsExistingMicrodriveState)
{
    auto& machine = oric->get_machine();

    ASSERT_TRUE(machine.cpu != nullptr);

    machine.insert_disk(make_disk(0x66));
    auto* drive = dynamic_cast<DriveMicrodrive*>(machine.get_disk_drive());
    ASSERT_NE(drive, nullptr);
    ASSERT_NE(drive->get_disk_image(), nullptr);

    drive->write_byte(0x4, 0x20);
    ASSERT_EQ(drive->get_disk_image(), nullptr);

    machine.insert_disk(make_disk(0x77));
    auto* drive_after_insert = dynamic_cast<DriveMicrodrive*>(machine.get_disk_drive());

    EXPECT_EQ(drive_after_insert, drive);
    ASSERT_NE(drive_after_insert, nullptr);
    EXPECT_EQ(drive_after_insert->get_disk_image(), nullptr);
}

TEST_F(DriveMicrodriveTest, MachineDiskInsertReplacesDriveNoneAfterBootWithoutDisk)
{
    auto& machine = oric->get_machine();

    ASSERT_TRUE(machine.cpu != nullptr);

    machine.init_disk();
    ASSERT_EQ(dynamic_cast<DriveMicrodrive*>(machine.get_disk_drive()), nullptr);

    machine.insert_disk(make_disk(0x88));
    auto* drive = dynamic_cast<DriveMicrodrive*>(machine.get_disk_drive());

    ASSERT_NE(drive, nullptr);
    EXPECT_NE(drive->get_disk_image(), nullptr);
    EXPECT_FALSE(machine.oric_rom_enabled);
    EXPECT_TRUE(machine.disk_rom_enabled);
}

TEST_F(DriveMicrodriveTest, MediaChangeDuringCommandClearsWd1793BusyState)
{
    DriveMicrodrive drive(oric->get_machine());

    ASSERT_TRUE(drive.insert_disk(make_disk(0x99), 0));

    drive.write_byte(0x2, 0x01);
    drive.write_byte(0x0, 0x80);
    ASSERT_NE(drive.get_wd1793_state().status & WD1793::StatusBusy, 0);
    ASSERT_GT(drive.get_wd1793_state().data_request_counter, 0);

    ASSERT_TRUE(drive.insert_disk(make_disk(0xaa), 0));

    EXPECT_EQ(drive.get_wd1793_state().status & WD1793::StatusBusy, 0);
    EXPECT_EQ(drive.get_wd1793_state().status & WD1793::StatusDataRequest, 0);
    EXPECT_EQ(drive.get_wd1793_state().data_request_counter, 0);
    EXPECT_EQ(drive.get_wd1793_state().interrupt_counter, 0);
    EXPECT_NE(drive.get_wd1793_state().current_track, nullptr);
    EXPECT_EQ(drive.get_wd1793_state().current_sector, nullptr);
    EXPECT_EQ(drive.read_byte(0x8), 0xff);
}

TEST_F(DriveMicrodriveTest, EjectsOneDriveWithoutAffectingAnother)
{
    DriveMicrodrive drive(oric->get_machine());

    ASSERT_TRUE(drive.insert_disk(make_disk(0x11), 0));
    auto* drive0_image = drive.get_disk_image();
    ASSERT_NE(drive0_image, nullptr);

    ASSERT_TRUE(drive.insert_disk(make_disk(0x22), 1));
    drive.write_byte(0x4, 0x20);
    auto* drive1_image = drive.get_disk_image();
    ASSERT_NE(drive1_image, nullptr);
    EXPECT_NE(drive1_image, drive0_image);

    EXPECT_TRUE(drive.eject_disk(1));
    EXPECT_EQ(drive.get_disk_image(), nullptr);

    drive.write_byte(0x4, 0x00);
    EXPECT_EQ(drive.get_disk_image(), drive0_image);
}

TEST_F(DriveMicrodriveTest, EjectSelectedDriveClearsWd1793MediaPointers)
{
    DriveMicrodrive drive(oric->get_machine());

    ASSERT_TRUE(drive.insert_disk(make_disk(0x33), 0));
    EXPECT_NE(drive.get_wd1793_state().current_track, nullptr);

    EXPECT_TRUE(drive.eject_disk(0));
    EXPECT_EQ(drive.get_disk_image(), nullptr);
    EXPECT_EQ(drive.get_wd1793_state().current_track, nullptr);
    EXPECT_EQ(drive.get_wd1793_state().current_sector, nullptr);
}

TEST_F(DriveMicrodriveTest, DriveSwitchClearsAndReloadsWd1793MediaPointers)
{
    DriveMicrodrive drive(oric->get_machine());

    ASSERT_TRUE(drive.insert_disk(make_disk(0x33), 0));
    EXPECT_NE(drive.get_wd1793_state().current_track, nullptr);

    drive.write_byte(0x4, 0x20);
    EXPECT_EQ(drive.get_disk_image(), nullptr);
    EXPECT_EQ(drive.get_wd1793_state().current_track, nullptr);
    EXPECT_EQ(drive.get_wd1793_state().current_sector, nullptr);
    EXPECT_EQ(drive.get_wd1793_state().status, 0);

    drive.write_byte(0x4, 0x00);
    EXPECT_NE(drive.get_disk_image(), nullptr);
    EXPECT_NE(drive.get_wd1793_state().current_track, nullptr);
}

TEST_F(DriveMicrodriveTest, ReadSectorOnEmptySelectedDriveCompletesWithoutDrq)
{
    DriveMicrodrive drive(oric->get_machine());

    ASSERT_TRUE(drive.insert_disk(make_disk(0x44), 0));
    drive.write_byte(0x4, 0x20);
    ASSERT_EQ(drive.get_disk_image(), nullptr);

    drive.write_byte(0x2, 0x01);
    drive.write_byte(0x0, 0x80);

    EXPECT_EQ(drive.get_wd1793_state().status & WD1793::StatusBusy, WD1793::StatusBusy);

    drive.exec(100);
    EXPECT_EQ(drive.get_wd1793_state().status & WD1793::StatusBusy, 0);
    EXPECT_EQ(drive.get_wd1793_state().status & WD1793::StatusRecordNotFound, WD1793::StatusRecordNotFound);
    EXPECT_EQ(drive.read_byte(0x8), 0xff);
    EXPECT_EQ(drive.get_wd1793_state().current_track, nullptr);
    EXPECT_EQ(drive.get_wd1793_state().current_sector, nullptr);
}

TEST_F(DriveMicrodriveTest, ReadAddressOnEmptySelectedDriveCompletesWithRecordNotFound)
{
    DriveMicrodrive drive(oric->get_machine());

    ASSERT_TRUE(drive.insert_disk(make_disk(0x55), 0));
    drive.write_byte(0x4, 0x20);
    ASSERT_EQ(drive.get_disk_image(), nullptr);

    drive.write_byte(0x0, 0xc3);
    EXPECT_EQ(drive.get_wd1793_state().status & WD1793::StatusBusy, WD1793::StatusBusy);

    drive.exec(100);
    EXPECT_EQ(drive.get_wd1793_state().status & WD1793::StatusBusy, 0);
    EXPECT_EQ(drive.get_wd1793_state().status & WD1793::StatusRecordNotFound, WD1793::StatusRecordNotFound);
    EXPECT_EQ(drive.read_byte(0x8), 0xff);
}
