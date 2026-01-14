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

#ifndef DISK_H
#define DISK_H


class Disk
{
public:
    virtual ~Disk() = default;

    /**
     * Initialize disk.
     * @return true on success
     */
    virtual bool init() = 0;

    /**
     * Reset disk.
     */
    virtual void reset() = 0;

    /**
     * Print disk status to console.
     */
    virtual void print_stat() = 0;

    /**
     * Execute one cycle.
     */
    virtual void exec() = 0;
};

#endif // DISK_H
