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

#include <csignal>

#include <print>
#include <sstream>

#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>

#include "oric.hpp"


std::unique_ptr<Oric> oric;

struct sigaction sigact;

/**
 * Handle signal
 * @param signal signal to handle
 */
static void signal_handler(int signal)
{
    if (signal == SIGINT) {
        oric->get_machine().stop();
        oric->do_break();
    }
}

/**
 * Initialize signal handler.
 */
void init_signals()
{
    sigact.sa_handler = signal_handler;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = 0;
    sigaction(SIGINT, &sigact, (struct sigaction *) nullptr);
}


int main(int argc, char *argv[])
{
    Config config;
    if (! config.parse(argc, argv)) {
        return 0;
    }

    oric = std::make_unique<Oric>(config);
    init_signals();

    try {
        oric->init();
    }
    catch (const std::exception &err) {
        std::println("Error initializing: {}", err.what());
        return 1;
    }

    oric->get_machine().reset();

    oric->run();

    return 0;
}
