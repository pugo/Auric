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

constexpr uint32_t track_size = 4600;   // bytes per track
constexpr uint32_t header_size = 256;   // bytes of header



DiskImage::DiskImage(const std::filesystem::path& path) :
    image_path(path),
    image_size(0),
    data(nullptr)
{
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

    return true;
}

bool DiskImage::set_track(uint8_t track)
{
    uint32_t track_offset = header_size + track * track_size;

    


    return true;
}

