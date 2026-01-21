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


DiskTrack::DiskTrack(uint8_t* track_data, size_t track_size)
{
    
}


uint8_t DiskTrack::read_byte(size_t offset) const
{
    return 0x00;
}


DiskSide::DiskSide(uint8_t side) :
    side(side)
{
    BOOST_LOG_TRIVIAL(debug) << "Added DiskSide: side " << (int)side;
}


void DiskSide::add_track(DiskTrack track)
{
    tracks.push_back(track);
}


DiskImage::DiskImage(const std::filesystem::path& path) :
    image_path(path),
    image_size(0),
    side_count(0),
    tracks_count(0),
    geometry(0),
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

    side_count = static_cast<uint8_t>(read32(8));
    tracks_count = static_cast<uint16_t>(read32(12));
    geometry = static_cast<uint8_t>(read32(16));

    BOOST_LOG_TRIVIAL(info) << "DiskImage: sides: " << (int)side_count
                            << ", tracks: " << (int)tracks_count
                            << ", geometry: " << (int)geometry;

    BOOST_LOG_TRIVIAL(info) << "Total size: " << image_size;

    for (uint8_t i = 1; i <= side_count; ++i) {
        disk_sides.emplace_back(DiskSide(i));
    }

    size_t size_per_side = tracks_count * track_size;

    for (uint8_t side = 0; side < side_count; ++side) {
        for (uint8_t track = 0; track < tracks_count; ++track) {
            disk_sides[side].add_track(DiskTrack(data + header_size + side * track * track_size, track_size));
            BOOST_LOG_TRIVIAL(info) << (tracks_count * side) + track << ": " << (header_size + side * size_per_side + track * track_size);
        }
    }

    return true;
}

bool DiskImage::set_track(uint8_t track)
{
    uint32_t track_offset = header_size + track * track_size;

    if (track_offset + track_size > image_size) {
        BOOST_LOG_TRIVIAL(error) << "DiskImage: track " << static_cast<int>(track) << " out of bounds";
        return false;
    }

    return true;
}

