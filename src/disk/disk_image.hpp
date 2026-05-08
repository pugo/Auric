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


/**
 * Represents a single sector on a disk.
 */
class DiskSector
{
public:
    /**
     * @param sector_number Number of the sector.
     * @param sector_data Sector data as a byte span.
     */
    DiskSector(uint16_t sector_number, std::span<uint8_t> sector_data);

    uint16_t sector_number;
    std::span<uint8_t> data;
    uint8_t sector_mark;

private:
    bool valid;
};

/**
 * Represents a single track on a disk.
 */
class DiskTrack
{
public:
    /**
     * @param track_data Track data as a byte span.
     */
    DiskTrack(std::span<uint8_t> track_data);

    /**
     * Get sector from the disk track.
     * @param sector_number number of sector to retrieve.
     * @return Pointer to the disk sector if found, nullptr otherwise.
     */
    DiskSector* get_sector(uint16_t sector_number);
    uint8_t sector_count() const { return sectors.size(); }

    /**
     * Sector data. Public for performance reasons.
     */
    std::span<uint8_t> data;

private:
    std::vector<DiskSector> sectors;
};

/**
 * Represents a side of a disk with multiple tracks.
 */
class DiskSide
{
public:
    /**
     * @param side Side number (0-based).
     */
    DiskSide(uint8_t side);

    /**
     * Add track to the side.
     * @param track Track to add.
     */
    void add_track(DiskTrack track);

    /**
     * Get a track from the disk side.
     * @param track Track number (0-based).
     * @return Pointer to the disk track if found, nullptr otherwise.
     */
    DiskTrack* get_track(uint8_t track);

protected:
    uint8_t side;
    std::vector<DiskTrack> tracks;
};

/**
 * Represents a disk image with multiple sides and tracks.
 * Normally the number of sides is 1 or 2, depending on the disk type.
 */
class DiskImage
{
public:
    /**
     * @param path Path to disk image file.
     */
    DiskImage(const std::filesystem::path& path);

    ~DiskImage();

    /**
     * Initialize the disk image from the provided file path.
     * @return True if initialization was successful, false otherwise.
     */
    bool init();

    void mark_dirty();
    void flush_if_dirty();

    /**
     * Get a track from the specified side and track number.
     * @param side Side number (0-based).
     * @param track Track number (0-based).
     * @return Pointer to the disk track if found, nullptr otherwise.
     */
    DiskTrack* get_track(uint8_t side, uint8_t track);

    /**
     * Get the total number of tracks on the disk.
     * @return Total number of tracks.
     */
    uint8_t get_track_count();

    /**
     * Get the number of sides on the disk.
     * @return Number of sides.
     */
    uint8_t side_count() const { return side_count_; }

    /**
     * Get the number of tracks per side.
     * @return Number of tracks.
     */
    uint8_t tracks_count() const { return tracks_count_; }

protected:
    uint32_t read32(uint32_t offset) const;

    std::filesystem::path image_path;
    size_t image_size;

    uint8_t side_count_;
    uint16_t tracks_count_;
    uint8_t geometry_;

    std::vector<uint8_t> memory_vector;

    bool dirty;
    std::chrono::steady_clock::time_point last_write{};

    uint8_t* data;

    std::vector<DiskSide> disk_sides;
};



#endif // DISK_IMAGE_H
