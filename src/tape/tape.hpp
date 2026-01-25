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

#ifndef TAPE_H
#define TAPE_H


class Tape
{
public:
    Tape() :
        motor_running(false)
    {}

    virtual ~Tape() = default;

    /**
     * Initialize tape.
     * @return true on success
     */
    virtual bool init() = 0;

    /**
     * Reset tape postion.
     */
    virtual void reset() = 0;

    /**
     * Print tape status to console.
     */
    virtual void print_stat() = 0;

    /**
     * Set motor state.
     * @param motor_on true if motor is set to on
     */
    virtual void motor_on(bool motor_on) = 0;

    /**
     * Execute one cycle.
     */
    virtual void exec(uint8_t cycles) = 0;

    /**
     * Check if motor is running.
     * @return true if motor is running.
     */
    bool is_motor_running() { return motor_running; };

protected:
    bool motor_running;
};

#endif // TAPE_TAP_H
