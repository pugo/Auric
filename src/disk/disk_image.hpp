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
#include <span>


class DiskSector
{
public:
    DiskSector(uint16_t sector_number, std::span<uint8_t> sector_data);

    uint16_t sector_number;
    std::span<uint8_t> data;
    uint8_t sector_mark;

private:
    bool valid;
};


class DiskTrack
{
public:
    DiskTrack(std::span<uint8_t> track_data);

    DiskSector* get_sector(uint16_t sector_number);
    uint8_t sector_count() const { return sectors.size(); }

    std::span<uint8_t> data;
private:
    std::vector<DiskSector> sectors;
};


class DiskSide
{
public:
    DiskSide(uint8_t side);

    void add_track(DiskTrack track);

    DiskTrack* get_track(uint8_t track);

protected:
    uint8_t side;
    std::vector<DiskTrack> tracks;
};


class DiskImage
{
public:
    DiskImage(const std::filesystem::path& path);

    bool init();

    DiskTrack* get_track(uint8_t side, uint8_t track);

    uint8_t get_track_count();

    uint8_t side_count() const { return side_count_; }
    uint8_t tracks_count() const { return tracks_count_; }

protected:
    uint32_t read32(uint32_t offset) const;

    std::filesystem::path image_path;
    size_t image_size;

    uint8_t side_count_;
    uint16_t tracks_count_;
    uint8_t geometry_;

    std::vector<uint8_t> memory_vector;
    uint8_t* data;

    std::vector<DiskSide> disk_sides;
};



#endif // DISK_IMAGE_H
