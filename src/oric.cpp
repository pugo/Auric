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


#include <format>
#include <iostream>
#include <print>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/log/trivial.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>

#include "oric.hpp"
#include "memory.hpp"
#include "frontends/sdl/frontend.hpp"

namespace po = boost::program_options;

const std::string_view rom_basic10{"ROMS/basic10.rom"};
const std::string_view rom_basic11b{"ROMS/basic11b.rom"};
const std::string_view rom_microdisk{"ROMS/microdis.rom"};


Oric::Oric(Config& config) :
    config(config),
    state(STATE_RUN),
    frontend(nullptr),
    machine(nullptr),
    last_address(0)
{
    if (config.start_in_monitor()) {
        state = STATE_MON;
    }
}

Oric::~Oric()
{
}

void Oric::init()
{
    machine = std::make_unique<Machine>(*this);
    frontend = std::make_unique<Frontend>(*this);

    machine->init(frontend.get());
    frontend->init_graphics();
    frontend->init_sound();

    frontend->get_status_bar().show_text_for("Starting ORIC!", std::chrono::seconds(3));

    machine->set_disassemble_execution(false);

    try {
        if (config.use_oric1_rom()) {
            //    	machine->memory.load("ROMS/test108k.rom", 0xc000);
            machine->oric_rom.load(rom_basic10, 0x0000);
        }
        else {
            machine->oric_rom.load(rom_basic11b, 0x0000);
        }
    }
    catch (const std::runtime_error& err) {
        throw(std::runtime_error(std::format("Failed loading ROM: {}", err.what())));
    }

    try {
        machine->disk_rom.load(rom_microdisk, 0x0000);
    }
    catch (const std::runtime_error& err) {
        throw(std::runtime_error(std::format("Failed loading disk drive ROM: {}", err.what())));
    }
}

void Oric::init_machine()
{
    machine = std::make_unique<Machine>(*this);
}

void Oric::run()
{
    bool do_run{true};

    while (do_run) {
        switch (state) {
            case STATE_RUN:
                machine->run(this);
                break;
            case STATE_MON:
            {
                std::string cmd;
                std::print(">> ");

                if (! getline(std::cin, cmd)) {
                    state = STATE_QUIT;
                    break;
                }

                state = handle_command(cmd);
                break;
            }
            case STATE_QUIT:
                do_run = false;
                break;
        }
    }
    frontend->close_sound();
}


void Oric::do_break()
{
    last_command = "";
    last_address = 0;

    if (state == STATE_MON) {
        std::println("\n\n - Bye! - \n\n");
    }
    else
    {
        std::println();
        std::println("* Oric Monitor *\n");
        std::println("        Ctrl-c : to exit the emulator");
        std::println("    g <return> : to continue the emulation");
        std::println("    h <return> : for help (more commands)\n");
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
    if (command_line.empty()) {
        if (last_command.empty()) {
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
        std::println("Available monitor commands:\n");
        std::println("ay              : print AY-3-8912 sound chip info");
        std::println("bs <address>    : set breakpoint for address");
        std::println("d               : disassemble from last address or PC");
        std::println("d <address> <n> : disassemble from address and n bytes ahead (example: d c000 10)");
        std::println("debug           : show debug output at run time");
        std::println("g               : go (continue)");
        std::println("g <address>     : go to address and run (example: g 1f00)");
        std::println("h               : help (showing this text)");
        std::println("i               : print machine info");
        std::println("m <address> <n> : dump memory from address and n bytes ahead (example: m 1f00 20)");
        std::println("pc <address>    : set program counter to address");
        std::println("quiet           : prevent debug output at run time");
        std::println("q               : quit");
        std::println("s [n]           : step one or possible n steps");
        std::println("sr, softreset   : soft reset oric");
        std::println("v               : print VIA (6522) info\n");
        return STATE_MON;
    }
    else if (cmd == "ay") { // info
        machine->ay3->print_status();
    }
    else if (cmd == "bs") { // set breakpoint <address>
        if (parts.size() < 2) {
            std::println("Error: missing address");
            return STATE_MON;
        }
        uint16_t addr = string_to_word(parts[1]);
        machine->cpu->set_breakpoint(addr);
    }
    else if (cmd == "d") { // info
        if (parts.size() == 1) {
            uint16_t addr = (last_address == 0) ? machine->cpu->get_pc() : last_address;
            last_address = machine->get_monitor().disassemble(addr, 30);
            return STATE_MON;
        }
        if (parts.size() < 3) {
            std::println("Use: d <start address> <length>");
            return STATE_MON;
        }
        last_address = machine->get_monitor().disassemble(string_to_word(parts[1]), string_to_word(parts[2]));
    }
    else if (cmd == "debug") {
        machine->set_disassemble_execution(true);
        std::println("Debug mode enabled");
    }
    else if (cmd == "g") { // go <address>
        return STATE_RUN;
    }
    else if (cmd == "i") { // info
        std::println("PC: ${:04X}", machine->cpu->get_pc());
        machine->PrintStat();
    }
    else if (cmd == "m") { // info
        if (parts.size() < 3) {
            std::println("Use: m <start address> <length>");
            return STATE_MON;
        }
        machine->memory.show(string_to_word(parts[1]), string_to_word(parts[2]));
    }
    else if (cmd == "pc") { // set pc
        if (parts.size() < 2) {
            std::println("Error: missing address");
            return STATE_MON;
        }
        uint16_t addr = string_to_word(parts[1]);
        machine->cpu->set_pc(addr);
        machine->PrintStat();
    }
    else if (cmd == "q") { // quit
        std::println("quit");
        return STATE_QUIT;
    }
    else if (cmd == "quiet") {
        machine->set_disassemble_execution(false);
        std::println("Quiet mode enabled");
    }
    else if (cmd == "s") { // step
        if (parts.size() == 2) {
            machine->run(std::stol(parts[1]), this);
        }
        else {
            bool brk = false;
            while (! machine->cpu->exec(false, brk)) {}
            if (brk) {
                std::println("Instruction BRK executed.");
            }
        }
        machine->PrintStat();
    }
    else if (cmd == "sr" || cmd == "softreset") {
        machine->cpu->NMI();
        std::println("NMI triggered");
    }
    else if (cmd == "v") { // info
        machine->mos_6522->get_state().print();
    }
    else {
        println("Unknown command \"{}\". Use command \"h\" to get help.", cmd);
    }

    return STATE_MON;
}
