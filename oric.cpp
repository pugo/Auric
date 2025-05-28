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


#include <sstream>
#include <cstdlib>
#include <vector>
#include <string>

#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>

#include "oric.hpp"
#include "memory.hpp"
#include "frontend.hpp"

namespace po = boost::program_options;


Oric::Oric(Config& config) :
    config(config),
    state(STATE_RUN),
    frontend(nullptr),
    machine(nullptr),
    last_command(""),
    last_address(0)
{
    if (config.start_in_monitor()) {
        state = STATE_MON;
    }
}


void Oric::init()
{
    machine = new Machine(this);
    frontend = new Frontend(this);

    machine->init(frontend);
    frontend->init_graphics();
    frontend->init_sound();
    frontend->init_fonts();

    machine->cpu->set_quiet(true);

    if (config.use_atmos_rom()) {
        machine->memory.load("ROMS/basic11b.rom", 0xc000);
    }
    else {
//    	machine->memory.load("ROMS/test108k.rom", 0xc000);
        machine->memory.load("ROMS/basic10.rom", 0xc000);
    }
}

void Oric::init_machine()
{
    machine = new Machine(this);
}


Oric::~Oric()
{
    if (machine) {
        delete machine;
    }

    if (frontend) {
        delete frontend;
    }
}


void Oric::run()
{
    while (true) {
        switch (state) {
            case STATE_RUN:
                machine->run(this);
                break;
            case STATE_MON:
            {
                std::string cmd;
                std::cout << ">> " << std::flush;

                if (! getline(std::cin, cmd)) {
                    state = STATE_QUIT;
                    break;
                }

                state = handle_command(cmd);
                break;
            }
            case STATE_QUIT:
                return;
        }
    }
}


void Oric::do_break()
{
    last_command = "";
    last_address = 0;

    if (state == STATE_MON) {
        std::cout << std::endl << std::endl << " - Bye! -" << std::endl << std::endl;
    }
    else
    {
        std::cout << std::endl;
        std::cout << "* Oric Monitor *" << std::endl << std::endl;
        std::cout << "        Ctrl-c : to exit the emulator" << std::endl;
        std::cout << "    g <return> : to continue the emulation" << std::endl;
        std::cout << "    h <return> : for help" << std::endl << std::endl;
    }

    state = STATE_MON;
}


void Oric::do_quit()
{
    state = STATE_QUIT;
}


uint16_t Oric::string_to_word(std::string& addr)
{
    uint16_t x;
    std::stringstream ss;
    ss << std::hex << addr;
    ss >> x;
    return x;
}


Oric::State Oric::handle_command(std::string& command_line)
{
    if (command_line.length() == 0) {
        if (last_command.length() == 0) {
            return STATE_MON;
        }
        command_line = last_command;
    }
    else {
        last_command = command_line;
    }

    std::vector<std::string> parts;
    boost::split(parts, command_line, boost::is_any_of("\t "));
    std::string cmd = parts[0];

    if (cmd == "h") {
        std::cout << "Available monitor commands:" << std::endl << std::endl;
        std::cout << "ay              : print AY-3-8912 sound chip info" << std::endl;
        std::cout << "bs <address>    : set breakpoint for address" << std::endl;
        std::cout << "d               : disassemble from PC" << std::endl;
        std::cout << "d <address> <n> : disassemble from address and n bytes ahead" << std::endl;
        std::cout << "debug           : show debug output at run time" << std::endl;
        std::cout << "g               : go (continue)" << std::endl;
        std::cout << "g <address>     : go to address and run" << std::endl;
        std::cout << "h               : help (showing this text)" << std::endl;
        std::cout << "i               : print machine info" << std::endl;
        std::cout << "m <address> <n> : dump memory from address and n bytes ahead" << std::endl;
        std::cout << "pc <address>    : set program counter to address" << std::endl;
        std::cout << "quiet           : prevent debug output at run time" << std::endl;
        std::cout << "q               : quit" << std::endl;
        std::cout << "s [n]           : step one or possible n steps" << std::endl;
        std::cout << "sr, softreset   : soft reset oric" << std::endl;
        std::cout << "v               : print VIA (6522) info" << std::endl;
        std::cout << "" << std::endl;
        return STATE_MON;
    }
    else if (cmd == "ay") { // info
        machine->ay3->print_status();
    }
    else if (cmd == "bs") { // set breakpoint <address>
        if (parts.size() < 2) {
            std::cout << "Error: missing address" << std::endl;
            return STATE_MON;
        }
        uint16_t addr = string_to_word(parts[1]);
        machine->cpu->set_breakpoint(addr);
    }
    else if (cmd == "d") { // info
        if (parts.size() == 1) {
            uint16_t addr = (last_address == 0) ? machine->cpu->get_pc() : last_address;
            last_address = machine->cpu->get_monitor().disassemble(addr, 30);
            return STATE_MON;
        }
        if (parts.size() < 3) {
            std::cout << "Use: d <start address> <length>" << std::endl;
            return STATE_MON;
        }
        last_address = machine->cpu->get_monitor().disassemble(string_to_word(parts[1]), string_to_word(parts[2]));
    }
    else if (cmd == "debug") {
        machine->cpu->set_quiet(false);
        std::cout << "Debug mode enabled" << std::endl;
    }
    else if (cmd == "g") { // go <address>
        return STATE_RUN;
    }
    else if (cmd == "i") { // info
        std::cout << "PC: " << machine->cpu->get_pc() << std::endl;
        machine->cpu->PrintStat();
    }
    else if (cmd == "m") { // info
        if (parts.size() < 3) {
            std::cout << "Use: m <start address> <length>" << std::endl;
            return STATE_MON;
        }
        machine->memory.show(string_to_word(parts[1]), string_to_word(parts[2]));
    }
    else if (cmd == "pc") { // set pc
        if (parts.size() < 2) {
            std::cout << "Error: missing address" << std::endl;
            return STATE_MON;
        }
        uint16_t addr = string_to_word(parts[1]);
        machine->cpu->set_pc(addr);
        machine->cpu->PrintStat();
    }
    else if (cmd == "q") { // quit
        std::cout << "quit" << std::endl;
        return STATE_QUIT;
    }
    else if (cmd == "quiet") {
        machine->cpu->set_quiet(true);
        std::cout << "Quiet mode enabled" << std::endl;
    }
    else if (cmd == "s") { // step
        if (parts.size() == 2) {
            machine->run(std::stol(parts[1]), this);
        }
        else {
            bool brk = false;
            while (! machine->cpu->exec(brk)) {}
            if (brk) {
                std::cout << "Instruction BRK executed." << std::endl;
            }
        }
        machine->cpu->PrintStat();
    }
    else if (cmd == "sr" || cmd == "softreset") {
        machine->cpu->NMI();
        std::cout << "NMI triggered" << std::endl;
    }
    else if (cmd == "v") { // info
        machine->mos_6522->get_state().print();
    }

    return STATE_MON;
}
