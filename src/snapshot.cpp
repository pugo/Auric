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


#include "snapshot.hpp"


Snapshot::Snapshot() :
    mos6502(),
    mos6522(),
    ay3_8919(),
    memory(),
    wd1793(),
    drive_microdrive(),
    microdrive_present(false),
    tape(),
    oric_rom_enabled(true),
    disk_rom_enabled(false),
    cycle_count(0),
    current_key_row(0),
    key_rows{},
    ula{}
{
}

Snapshot::~Snapshot()
{
}
