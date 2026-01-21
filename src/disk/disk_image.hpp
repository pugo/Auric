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

#ifndef DISK_IMAGE_H
#define DISK_IMAGE_H

#include <filesystem>


class DiskTrack
{
public:
    DiskTrack(uint8_t* track_data, size_t track_size);

    uint8_t read_byte(size_t offset) const;
};


class DiskSide
{
public:
    DiskSide(uint8_t side);

    void add_track(DiskTrack track);

protected:
    uint8_t side;
    std::vector<DiskTrack> tracks;
};


class DiskImage
{
public:
    DiskImage(const std::filesystem::path& path);

    bool init();
    bool set_track(uint8_t track);


protected:
    uint32_t read32(uint32_t offset) const;

    std::filesystem::path image_path;
    size_t image_size;

    uint8_t side_count;
    uint16_t tracks_count;
    uint8_t geometry;

    std::vector<uint8_t> memory_vector;
    uint8_t* data;

    std::vector<DiskSide> disk_sides;
};



#endif // DISK_IMAGE_H
