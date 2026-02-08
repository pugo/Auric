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

#ifndef MEMORY_INTERFACE_HPP
#define MEMORY_INTERFACE_HPP

class Machine;


typedef uint8_t (*f_memory_read_byte_handler)(Machine &oric, uint16_t address);
typedef uint8_t (*f_memory_read_byte_zp_handler)(Machine &oric, uint8_t address);

typedef uint16_t (*f_memory_read_word_handler)(Machine &oric, uint16_t address);
typedef uint16_t (*f_memory_read_word_zp_handler)(Machine &oric, uint8_t address);

typedef void (*f_memory_write_byte_handler)(Machine &oric, uint16_t address, uint8_t val);
typedef void (*f_memory_write_byte_zp_handler)(Machine &oric, uint8_t address, uint8_t val);

#endif //ORIC_MEMORY_INTERFACE_HPP