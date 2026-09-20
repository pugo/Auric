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

#include <spdlog/spdlog.h>
#include <fstream>
#include <print>
#include <sstream>
#include <vector>

#include "disk_image.hpp"

constexpr uint32_t track_size = 6400;   // bytes per track
constexpr uint32_t header_size = 256;   // bytes of header



// ==== DiskSector ============================================

DiskSector::DiskSector(uint16_t sector_number, std::span<uint8_t> sector_data) :
    sector_number(sector_number),
    valid(false)
{
    // spdlog::debug("   -- Sector data start byte: {:x}", static_cast<int>(sector_data[0]));
    // if (sector_data[0] == 0xfb) {
    //     spdlog::debug("   -- Sector data ID byte indicates normal data sector");
    // } else if (sector_data[0] == 0xf8) {
    //     spdlog::debug("   -- Sector data ID byte indicates deleted data sector");
    // } else {
    //     spdlog::debug("   -- Sector data ID byte indicates unknown sector type");
    // }

    if (sector_data[0] == 0xfb || sector_data[0] == 0xf8) {
        this->sector_mark = sector_data[0];
        this->data = sector_data.subspan(1, sector_data.size() - 3); // Exclude ID byte and CRC
        valid = true;
    }
}


// ==== DiskTrack ============================================

DiskTrack::DiskTrack(std::span<uint8_t> track_data)
{
    spdlog::debug(" DiskTrack - Track data size: {}", track_data.size());

    this->data = track_data;
    auto data_ptr = track_data.begin();
    auto data_end = track_data.end();

    uint16_t sector{0};

    while (data_ptr < data_end) {
        // spdlog::debug(" -- ptr: {:x} -- Searching for sector ID record for sector {}",
        //                data_ptr - track_data.begin(), sector);
        while (data_ptr < (data_end - 10) &&
               !(data_ptr[0] == 0xa1 && data_ptr[1] == 0xa1 && data_ptr[2] == 0xa1 && data_ptr[3] == 0xfe)) {
            ++data_ptr;
        }
        data_ptr += 3;

        if (data_ptr >= data_end - 7) {
            break;
        }

        auto track_nr = static_cast<uint16_t>(data_ptr[1]);
        auto side_nr = static_cast<uint16_t>(data_ptr[2]);
        auto sector_nr = static_cast<uint16_t>(data_ptr[3]);
        auto bps = static_cast<uint16_t>(data_ptr[4]);
        auto sector_size = 128 << bps;

        // spdlog::debug(" -- ptr: {:x} -- Track header: track {}, side {}, sector {}, sector_size {}",
        //                data_ptr - track_data.begin(), track_nr, side_nr, sector_nr, sector_size);
        data_ptr += 7; // Skip ID record and CRC

        if (data_ptr >= data_end - sector_size - 3) {
            break;
        }

        while (data_ptr < data_end && (data_ptr[0] != 0xfb && data_ptr[0] != 0xf8)) {
            ++data_ptr;
        }

        auto sector_data = std::span<uint8_t>(data_ptr, sector_size + 3); // Include ID byte and CRC
        sectors.push_back(DiskSector(sector_nr, sector_data));

        // spdlog::debug(" -- data position: {:x}", data_ptr - track_data.begin());

        data_ptr += 256;
    }
}


DiskSector* DiskTrack::get_sector(uint16_t sector_number)
{
    for (auto& sector : sectors) {
        if (sector.sector_number == sector_number) {
            return &sector;
        }
    }

    return nullptr;
}

DiskSector* DiskTrack::get_first_sector()
{
    if (sectors.empty()) {
        return nullptr;
    }

    return &sectors.front();
}



// ==== DiskSide ============================================

DiskSide::DiskSide(uint8_t side) :
    side(side)
{
    spdlog::debug("Added DiskSide: side {}", static_cast<int>(side));
}

void DiskSide::add_track(DiskTrack track)
{
    tracks.push_back(std::move(track));
}


DiskTrack* DiskSide::get_track(uint8_t track)
{
    if (track >= tracks.size()) {
        return nullptr;
    }

    return &tracks[track];
}


// ==== DiskImage ============================================

DiskImage::DiskImage(const std::filesystem::path& path) :
    image_path(path),
    image_size(0),
    side_count_(0),
    tracks_count_(0),
    geometry_(0),
    dirty(false),
    data(nullptr)
{
}


DiskImage::~DiskImage()
{
}


uint32_t DiskImage::read32(uint32_t offset) const
{
    uint32_t value{0};
    value |= static_cast<uint32_t>(data[offset]);
    value |= static_cast<uint32_t>(data[offset + 1]) << 8;
    value |= static_cast<uint32_t>(data[offset + 2]) << 16;
    value |= static_cast<uint32_t>(data[offset + 3]) << 24;
    return value;
}


bool DiskImage::init()
{
    spdlog::info("DiskImage: Reading disk image file '{}'", image_path.string());

    std::ifstream file (image_path, std::ios::in | std::ios::binary | std::ios::ate);
    if (file.is_open())
    {
        image_size = file.tellg();
        memory_vector = std::vector<uint8_t>(image_size);
        data = memory_vector.data();

        file.seekg(0, std::ios::beg);
        file.read(reinterpret_cast<char*>(data), image_size);
        file.close();
    }
    else {
        spdlog::warn("DiskImage: unable to open image file");
        return false;
    }

    if (memory_vector.size() >= 8 && std::equal(memory_vector.begin(), memory_vector.begin() + 8, "MFM_DISK")) {
        spdlog::info("DiskImage: MFM disk image detected");
    } else {
        spdlog::warn("DiskImage: unknown disk image format");
        return false;
    }

    side_count_ = static_cast<uint8_t>(read32(8));
    tracks_count_ = static_cast<uint16_t>(read32(12));
    geometry_ = static_cast<uint8_t>(read32(16));

    spdlog::debug("DiskImage: sides: {}, tracks: {}, geometry: {}",
                  static_cast<int>(side_count_),
                  static_cast<int>(tracks_count_),
                  static_cast<int>(geometry_));

    spdlog::debug("Total size: {}", image_size);
    spdlog::debug("data start: {}", static_cast<void*>(data));

    if (!rebuild_disk_sides()) {
        return false;
    }

    return true;
}


bool DiskImage::rebuild_disk_sides()
{
    disk_sides.clear();

    for (uint8_t i = 0; i < side_count_; ++i) {
        disk_sides.emplace_back(DiskSide(i));
    }

    size_t size_per_side = tracks_count_ * track_size;

    for (uint8_t side = 0; side < side_count_; ++side) {
        spdlog::debug("======= DiskImage: sides: {} =======", static_cast<int>(side));

        for (uint16_t track = 0; track < tracks_count_; ++track) {
            const size_t offset = header_size + (side * size_per_side) + (track * track_size);
            if (offset + track_size > image_size) {
                spdlog::error("DiskImage: track data out of bounds");
                return false;
            }

            auto track_data = std::span<uint8_t>(data + offset, track_size);
            spdlog::debug("======= DiskImage: track: {} =======", static_cast<int>(track));
            disk_sides[side].add_track(DiskTrack(track_data));
        }
    }

    return true;
}


DiskImage::SnapshotState DiskImage::save_to_snapshot() const
{
    return {image_path, memory_vector, dirty};
}


void DiskImage::load_from_snapshot(const SnapshotState& snapshot)
{
    image_path = snapshot.path;
    memory_vector = snapshot.data;
    image_size = memory_vector.size();
    data = memory_vector.data();
    dirty = snapshot.dirty;

    if (memory_vector.size() < header_size + 20) {
        side_count_ = 0;
        tracks_count_ = 0;
        geometry_ = 0;
        disk_sides.clear();
        return;
    }

    if (std::equal(memory_vector.begin(), memory_vector.begin() + 8, "MFM_DISK")) {
        side_count_ = static_cast<uint8_t>(read32(8));
        tracks_count_ = static_cast<uint16_t>(read32(12));
        geometry_ = static_cast<uint8_t>(read32(16));
        rebuild_disk_sides();
    }
}

void DiskImage::mark_dirty()
{
    dirty = true;
    last_write = std::chrono::steady_clock::now();
}

void DiskImage::flush_if_dirty(bool force)
{
    if (!dirty) {
        return;
    }

    const auto now = std::chrono::steady_clock::now();
    if (!force && now - last_write < std::chrono::milliseconds(1000)) {
        return;
    }

    last_write = now;

    std::ofstream file (image_path, std::ios::out | std::ios::binary);
    if (!file.is_open()) {
        spdlog::error("DiskImage: failed to open file for writing");
    }

    file.write((char*)data, image_size);
    file.close();

    dirty = false;

    spdlog::debug("DiskImage: disk image file '{}' written", image_path.string());
}


DiskTrack* DiskImage::get_track(uint8_t side, uint8_t track)
{
    if (side >= side_count_) {
        return nullptr;
    }

    return disk_sides[side].get_track(track);
}
