// =========================================================================
//   Copyright (C) 2009-2025 by Anders Piniesjö <pugo@pugo.org>
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

#ifndef MONITOR_H
#define MONITOR_H

#include <map>
#include <string_view>

#include "memory.hpp"
#include "chip/memory_interface.hpp"


enum class Addressing
{
    immediate,
    absolute,
    zero_page,
    implied,
    indirect_absolute,
    absolute_indexed_x,
    absolute_indexed_y,
    zero_page_indexed_x,
    zero_page_indexed_y,
    indexed_indirect_x,
    indirect_indexed_y,
    relative,
    accumulator
};


struct Opcode
{
    uint8_t opcode;
    std::string_view name;
    Addressing addressing;
};


class Monitor
{
public:
    explicit Monitor(Machine& machine);

    /**
     * Disassemble the instruction at given address.
     * @param address address to disassemble
     * @return next address, after instruction
     */
    uint16_t disassemble(uint16_t address);

    /**
     * Disassemble instruction at given address and instructions up to bytes.
     * @param address address to disassemble
     * @param bytes number of bytes to disassemble
     * @return next address, after last instruction
     */
    uint16_t disassemble(uint16_t address, size_t bytes);

    f_memory_read_byte_handler memory_read_byte_handler;

private:
    Machine& machine;

    std::map<uint8_t, Opcode> opcodes;
};


#endif // MONITOR_H