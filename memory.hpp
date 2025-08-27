// =========================================================================
//   Copyright (C) 2009-2024 by Anders Piniesjö <pugo@pugo.org>
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

#ifndef MEMORY_H
#define MEMORY_H

#include <iostream>
#include <ostream>
#include <cstdint>
#include <vector>


class Snapshot;


class Memory
{
public:
    Memory(uint32_t size);
    ~Memory() = default;

    /**
     * Load file from given path to given address.
     * @param path path to load data from
     * @param address address to start storing file at
     */
    void load(const std::string& path, uint32_t address);

    /**
     * Get size of memory.
     * @return size of memory
     */
    uint32_t get_size() { return size; }

    /**
     * Save memory to snapshot.
     * @param snapshot snapshot to save to
     */
    void save_to_snapshot(Snapshot& snapshot);

    /**
     * Load memory from snapshot.
     * @param snapshot snapshot to load from
     */
    void load_from_snapshot(Snapshot& snapshot);

    /**
     * Set position of memory for later use of << operator.
     * @param address new memory position
     */
    void set_mem_pos(uint16_t address) { mempos = address; }

    /**
     * Save given value to current memory position and increment position
     * @param os Memory to save to
     * @param in value to write
     * @return Memory
     */
    friend Memory& operator<<(Memory& os, unsigned int in) {
        os.mem[os.mempos++] = static_cast<uint8_t>(in & 0xff);
        return os;
    }

    /**
     * Show memory at given position, showing given length ammount of bytes.
     * @param pos position in memory to show from
     * @param length number of bytes to show
     */
    void show(uint32_t pos, uint32_t length);

    // This is an emulator where speed is important. No smart pointers or getters for memory!
    uint8_t* mem;

protected:
    uint32_t size;
    uint32_t mempos;
    std::vector<uint8_t> memory;
};



#endif // MEMORY_H
