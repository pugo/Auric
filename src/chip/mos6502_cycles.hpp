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

#ifndef OPCODE_CYCLES_H
#define OPCODE_CYCLES_H


uint8_t opcode_cycles[] = {
 // 0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
    7, 6, 0, 8, 3, 3, 5, 0, 3, 2, 2, 0, 4, 4, 6, 0,  // 0x00
    2, 5, 0, 8, 4, 4, 6, 0, 2, 4, 2, 0, 4, 4, 7, 0,  // 0x10
    6, 6, 0, 0, 3, 3, 5, 0, 4, 2, 2, 0, 4, 4, 6, 0,  // 0x20
    2, 5, 0, 8, 4, 4, 6, 0, 2, 4, 2, 0, 4, 4, 7, 0,  // 0x30
    6, 6, 0, 0, 3, 3, 5, 0, 3, 2, 2, 0, 3, 4, 6, 0,  // 0x40
    2, 5, 0, 0, 4, 4, 6, 0, 2, 4, 2, 0, 4, 4, 7, 0,  // 0x50
    6, 6, 0, 0, 3, 3, 5, 0, 4, 2, 2, 0, 5, 4, 6, 0,  // 0x60
    2, 5, 0, 0, 4, 4, 6, 0, 2, 4, 2, 0, 4, 4, 7, 0,  // 0x70
    2, 6, 2, 0, 3, 3, 3, 0, 2, 2, 2, 0, 4, 4, 4, 0,  // 0x80
    2, 6, 0, 0, 4, 4, 4, 0, 2, 5, 2, 0, 0, 5, 0, 0,  // 0x90
    2, 6, 2, 0, 3, 3, 3, 0, 2, 2, 2, 0, 4, 4, 4, 0,  // 0xA0
    2, 5, 0, 0, 4, 4, 4, 0, 2, 4, 2, 0, 4, 4, 4, 0,  // 0xB0
    2, 6, 2, 0, 3, 3, 5, 0, 2, 2, 2, 2, 4, 4, 6, 0,  // 0xC0
    2, 5, 0, 0, 4, 4, 6, 0, 2, 4, 2, 0, 4, 4, 7, 0,  // 0xD0
    2, 6, 2, 0, 3, 3, 5, 0, 2, 2, 2, 0, 4, 4, 6, 0,  // 0xE0
    2, 5, 0, 0, 4, 4, 6, 0, 2, 4, 2, 0, 4, 4, 7, 7   // 0xF0
};

#endif // OPCODE_CYCLES_H
