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

#include <boost/log/trivial.hpp>
#include <fstream>
#include <sstream>
#include <vector>

#include "disk_image.hpp"

constexpr uint32_t track_size = 6400;   // bytes per track
constexpr uint32_t header_size = 256;   // bytes of header



// ==== DiskSector ============================================

DiskSector::DiskSector(std::span<uint8_t> sector_data) :
    valid(false)
{
    BOOST_LOG_TRIVIAL(debug) << "   -- Sector data start byte: " << std::hex << (int)sector_data[0];
    if (sector_data[0] == 0xfb) {
        BOOST_LOG_TRIVIAL(debug) << "   -- Sector data ID byte indicates normal data sector";
    } else if (sector_data[0] == 0xf8) {
        BOOST_LOG_TRIVIAL(debug) << "   -- Sector data ID byte indicates deleted data sector";
    } else {
        BOOST_LOG_TRIVIAL(debug) << "   -- Sector data ID byte indicates unknown sector type";
    }

    if (sector_data[0] == 0xfb || sector_data[0] == 0xf8) {
        this->sector_data = sector_data.subspan(1, sector_data.size() - 3); // Exclude ID byte and CRC
        valid = true;
    }
}


// ==== DiskTrack ============================================

DiskTrack::DiskTrack(std::span<uint8_t> track_data)
{
    BOOST_LOG_TRIVIAL(info) << " DiskTrack - Track data size: " << track_data.size();

    auto data_ptr = track_data.begin();
    auto data_end = track_data.end();

    uint16_t sector{0};

    while (data_ptr < data_end) {
        BOOST_LOG_TRIVIAL(debug) << " -- ptr: " << std::hex << (data_ptr - track_data.begin())
                                 << " -- Searching for sector ID record for sector " << sector;
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

        BOOST_LOG_TRIVIAL(debug) << " -- ptr: " << std::hex << (data_ptr - track_data.begin())
                                 << " -- Track header: track " << track_nr
                                 << ", side " << side_nr
                                 << ", sector " << sector_nr
                                 << ", sector_size " << sector_size;
        data_ptr += 7; // Skip ID record and CRC

        if (data_ptr >= data_end - sector_size - 3) {
            break;
        }

        while (data_ptr < data_end && (data_ptr[0] != 0xfb && data_ptr[0] != 0xf8)) {
            ++data_ptr;
        }

        auto sector_data = std::span<uint8_t>(data_ptr, sector_size + 3); // Include ID byte and CRC
        sectors.push_back(DiskSector(sector_data));

        auto data_pos = data_ptr;
        BOOST_LOG_TRIVIAL(debug) << " -- data position: " << std::hex << (data_ptr - track_data.begin());

        data_ptr += 256;
    }

}


bool DiskTrack::get_sector(uint8_t sector, DiskSector* out_sector)
{
    if (sector >= sectors.size()) {
        return false;
    }

    out_sector = &sectors[sector];
    return true;
}



// ==== DiskSide ============================================

DiskSide::DiskSide(uint8_t side) :
    side(side)
{
    BOOST_LOG_TRIVIAL(debug) << "Added DiskSide: side " << (int)side;
}

void DiskSide::add_track(DiskTrack track)
{
    tracks.push_back(track);
}


bool DiskSide::get_track(uint8_t track, DiskTrack* out_track)
{
    if (track >= tracks.size()) {
        return false;
    }

    out_track = &tracks[track];
    return true;
}


// ==== DiskImage ============================================

DiskImage::DiskImage(const std::filesystem::path& path) :
    image_path(path),
    image_size(0),
    side_count_(0),
    tracks_count_(0),
    geometry_(0),
    data(nullptr)
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
    BOOST_LOG_TRIVIAL(info) << "DiskImage: Reading disk image file '" << image_path << "'";

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
        BOOST_LOG_TRIVIAL(warning) << "DiskImage: unable to open image file";
        return false;
    }

    if (memory_vector.size() >= 8 && std::equal(memory_vector.begin(), memory_vector.begin() + 8, "MFM_DISK")) {
        BOOST_LOG_TRIVIAL(info) << "DiskImage: MFM disk image detected";
    } else {
        BOOST_LOG_TRIVIAL(warning) << "DiskImage: unknown disk image format";
        return false;
    }

    side_count_ = static_cast<uint8_t>(read32(8));
    tracks_count_ = static_cast<uint16_t>(read32(12));
    geometry_ = static_cast<uint8_t>(read32(16));

    BOOST_LOG_TRIVIAL(info) << "DiskImage: sides: " << (int)side_count_
                            << ", tracks: " << (int)tracks_count_
                            << ", geometry: " << (int)geometry_;

    BOOST_LOG_TRIVIAL(info) << "Total size: " << image_size;
    BOOST_LOG_TRIVIAL(info) << "data start: " << (void*)data;

    for (uint8_t i = 1; i <= side_count_; ++i) {
        disk_sides.emplace_back(DiskSide(i));
    }

    size_t size_per_side = tracks_count_ * track_size;

    for (uint8_t side = 0; side < side_count_; ++side) {
        BOOST_LOG_TRIVIAL(info) << "======= DiskImage: sides: " << (int)side << " =======";

        for (uint8_t track = 0; track < tracks_count_; ++track) {
            auto track_data = std::span<uint8_t>(data + header_size + (side * size_per_side) + (track * track_size), track_size);
            if (track_data.data() - data > image_size) {
                BOOST_LOG_TRIVIAL(error) << "DiskImage: track data out of bounds";
                return false;
            }

            BOOST_LOG_TRIVIAL(info) << "======= DiskImage: track: " << (int)track << " =======";
            disk_sides[side].add_track(track_data);
        }
    }

    return true;
}


bool DiskImage::get_track(uint8_t side, uint8_t track, DiskTrack* out_track)
{
    if (side >= side_count_) {
        return false;
    }

    return disk_sides[side].get_track(track, out_track);
}
