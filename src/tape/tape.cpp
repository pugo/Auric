// =========================================================================
//   Copyright (C) 2009-2026 by Anders Piniesjö <pugo@pugo.org>
//
//   This program is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 2 of the License, or
//   (at your option) any later version.
// =========================================================================

#include "snapshot.hpp"
#include "tape.hpp"

#include <format>

std::string Tape::debug_status() const
{
    return std::format("motor={} ", motor_running ? "on" : "off");
}


void Tape::save_to_snapshot(Snapshot& snapshot) const
{
    snapshot.tape = {};
    snapshot.tape.motor_running = motor_running;
}


void Tape::load_from_snapshot(const Snapshot& snapshot)
{
    motor_running = snapshot.tape.motor_running;
}
