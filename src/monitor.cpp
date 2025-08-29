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

#include <iostream>
#include <iomanip>

#include <map>
#include <vector>

#include "oric.hpp"
#include "memory.hpp"
#include "monitor.hpp"


std::vector<Opcode> opcodes_list = {
        {ADC_IMM,   "ADC", Addressing::immediate},
        {ADC_ZP,    "ADC", Addressing::zero_page},
        {ADC_ZP_X,  "ADC", Addressing::zero_page_indexed_x},
        {ADC_ABS,   "ADC", Addressing::absolute},
        {ADC_ABS_X, "ADC", Addressing::absolute_indexed_x},
        {ADC_ABS_Y, "ADC", Addressing::absolute_indexed_y},
        {ADC_IND_X, "ADC", Addressing::indexed_indirect_x},
        {ADC_IND_Y, "ADC", Addressing::indirect_indexed_y},

        {AND_IMM,   "AND", Addressing::immediate},
        {AND_ZP,    "AND", Addressing::zero_page},
        {AND_ZP_X,  "AND", Addressing::zero_page_indexed_x},
        {AND_ABS,   "AND", Addressing::absolute},
        {AND_ABS_X, "AND", Addressing::absolute_indexed_x},
        {AND_ABS_Y, "AND", Addressing::absolute_indexed_y},
        {AND_IND_X, "AND", Addressing::indexed_indirect_x},
        {AND_IND_Y, "AND", Addressing::indirect_indexed_y},

        {ASL_ACC,   "ASL", Addressing::accumulator},
        {ASL_ZP,    "ASL", Addressing::zero_page},
        {ASL_ZP_X,  "ASL", Addressing::zero_page_indexed_x},
        {ASL_ABS,   "ASL", Addressing::absolute},
        {ASL_ABS_X, "ASL", Addressing::absolute_indexed_x},

        {BCC,       "BCC", Addressing::relative},

        {BCS,       "BCS", Addressing::relative},

        {BEQ,       "BEQ", Addressing::relative},

        {BIT_ZP,    "BIT", Addressing::zero_page},
        {BIT_ABS,   "BIT", Addressing::absolute},

        {BMI,       "BMI", Addressing::relative},

        {BNE,       "BNE", Addressing::relative},

        {BPL,       "BPL", Addressing::relative},

        {BRK,       "BRK", Addressing::implied},

        {BVC,       "BVC", Addressing::relative},

        {BVS,       "BVS", Addressing::relative},

        {CLC,       "CLC", Addressing::implied},

        {CLD,       "CLD", Addressing::implied},

        {CLI,       "CLI", Addressing::implied},

        {CLV,       "CLV", Addressing::implied},

        {CMP_IMM,   "CMP", Addressing::immediate},
        {CMP_ZP,    "CMP", Addressing::zero_page},
        {CMP_ZP_X,  "CMP", Addressing::zero_page_indexed_x},
        {CMP_ABS,   "CMP", Addressing::absolute},
        {CMP_ABS_X, "CMP", Addressing::absolute_indexed_x},
        {CMP_ABS_Y, "CMP", Addressing::absolute_indexed_y},
        {CMP_IND_X, "CMP", Addressing::indexed_indirect_x},
        {CMP_IND_Y, "CMP", Addressing::indirect_indexed_y},

        {CPX_IMM,   "CPX", Addressing::immediate},
        {CPX_ZP,    "CPX", Addressing::zero_page},
        {CPX_ABS,   "CPX", Addressing::absolute},

        {CPY_IMM,   "CPY", Addressing::immediate},
        {CPY_ZP,    "CPY", Addressing::zero_page},
        {CPY_ABS,   "CPY", Addressing::absolute},

        {DEC_ZP,    "DEC", Addressing::zero_page},
        {DEC_ZP_X,  "DEC", Addressing::zero_page_indexed_x},
        {DEC_ABS,   "DEC", Addressing::absolute},
        {DEC_ABS_X, "DEC", Addressing::absolute_indexed_x},

        {DEX,       "DEX", Addressing::implied},

        {DEY,       "DEY", Addressing::implied},

        {EOR_IMM,   "EOR", Addressing::immediate},
        {EOR_ZP,    "EOR", Addressing::zero_page},
        {EOR_ZP_X,  "EOR", Addressing::zero_page_indexed_x},
        {EOR_ABS,   "EOR", Addressing::absolute},
        {EOR_ABS_X, "EOR", Addressing::absolute_indexed_x},
        {EOR_ABS_Y, "EOR", Addressing::absolute_indexed_y},
        {EOR_IND_X, "EOR", Addressing::indexed_indirect_x},
        {EOR_IND_Y, "EOR", Addressing::indirect_indexed_y},

        {INC_ZP,    "INC", Addressing::zero_page},
        {INC_ZP_X,  "INC", Addressing::zero_page_indexed_x},
        {INC_ABS,   "INC", Addressing::absolute},
        {INC_ABS_X, "INC", Addressing::absolute_indexed_x},

        {INX,       "INX", Addressing::implied},

        {INY,       "INY", Addressing::implied},

        {JMP_ABS,   "JMP", Addressing::absolute},
        {JMP_IND,   "JMP", Addressing::indirect_absolute},

        {JSR,       "JSR", Addressing::absolute},

        {LDA_IMM,   "LDA", Addressing::immediate},
        {LDA_ZP,    "LDA", Addressing::zero_page},
        {LDA_ZP_X,  "LDA", Addressing::zero_page_indexed_x},
        {LDA_ABS,   "LDA", Addressing::absolute},
        {LDA_ABS_X, "LDA", Addressing::absolute_indexed_x},
        {LDA_ABS_Y, "LDA", Addressing::absolute_indexed_y},
        {LDA_IND_X, "LDA", Addressing::indexed_indirect_x},
        {LDA_IND_Y, "LDA", Addressing::indirect_indexed_y},

        {LDX_IMM,   "LDX", Addressing::immediate},
        {LDX_ZP,    "LDX", Addressing::zero_page},
        {LDA_ZP_X,  "LDX", Addressing::zero_page_indexed_y},
        {LDX_ABS,   "LDX", Addressing::absolute},
        {LDX_ABS_Y, "LDX", Addressing::absolute_indexed_y},

        {LDY_IMM,   "LDY", Addressing::immediate},
        {LDY_ZP,    "LDY", Addressing::zero_page},
        {LDY_ZP_X,  "LDY", Addressing::zero_page_indexed_x},
        {LDY_ABS,   "LDY", Addressing::absolute},
        {LDY_ABS_X, "LDY", Addressing::absolute_indexed_x},

        {LSR_ACC,   "LSR", Addressing::accumulator},
        {LSR_ZP,    "LSR", Addressing::zero_page},
        {LSR_ZP_X,  "LSR", Addressing::zero_page_indexed_x},
        {LSR_ABS,   "LSR", Addressing::absolute},
        {LSR_ABS_X, "LSR", Addressing::absolute_indexed_x},

        {NOP,       "NOP", Addressing::implied},

        {ORA_IMM,   "ORA", Addressing::immediate},
        {ORA_ZP,    "ORA", Addressing::zero_page},
        {ORA_ZP_X,  "ORA", Addressing::zero_page_indexed_x},
        {ORA_ABS,   "ORA", Addressing::absolute},
        {ORA_ABS_X, "ORA", Addressing::absolute_indexed_x},
        {ORA_ABS_Y, "ORA", Addressing::absolute_indexed_y},
        {ORA_IND_X, "ORA", Addressing::indexed_indirect_x},
        {ORA_IND_Y, "ORA", Addressing::indirect_indexed_y},

        {PHA,       "PHA", Addressing::implied},

        {PHP,       "PHP", Addressing::implied},

        {PLA,       "PLA", Addressing::implied},

        {PLP,       "PLP", Addressing::implied},

        {ROL_ACC,   "ROL", Addressing::accumulator},
        {ROL_ZP,    "ROL", Addressing::zero_page},
        {ROL_ZP_X,  "ROL", Addressing::zero_page_indexed_x},
        {ROL_ABS,   "ROL", Addressing::absolute},
        {ROL_ABS_X, "ROL", Addressing::absolute_indexed_x},

        {ROR_ACC,   "ROR", Addressing::accumulator},
        {ROR_ZP,    "ROR", Addressing::zero_page},
        {ROR_ZP_X,  "ROR", Addressing::zero_page_indexed_x},
        {ROR_ABS,   "ROR", Addressing::absolute},
        {ROR_ABS_X, "ROR", Addressing::absolute_indexed_x},

        {RTI,       "RTI", Addressing::implied},

        {RTS,       "RTS", Addressing::implied},

        {SBC_IMM,   "SBC", Addressing::immediate},
        {SBC_ZP,    "SBC", Addressing::zero_page},
        {SBC_ZP_X,  "SBC", Addressing::zero_page_indexed_x},
        {SBC_ABS,   "SBC", Addressing::absolute},
        {SBC_ABS_X, "SBC", Addressing::absolute_indexed_x},
        {SBC_ABS_Y, "SBC", Addressing::absolute_indexed_y},
        {SBC_IND_X, "SBC", Addressing::indexed_indirect_x},
        {SBC_IND_Y, "SBC", Addressing::indirect_indexed_y},

        {SEC,       "SEC", Addressing::implied},

        {SED,       "SED", Addressing::implied},

        {SEI,       "SEI", Addressing::implied},

        {STA_ZP,    "STA", Addressing::zero_page},
        {STA_ZP_X,  "STA", Addressing::zero_page_indexed_x},
        {STA_ABS,   "STA", Addressing::absolute},
        {STA_ABS_X, "STA", Addressing::absolute_indexed_x},
        {STA_ABS_Y, "STA", Addressing::absolute_indexed_y},
        {STA_IND_X, "STA", Addressing::indexed_indirect_x},
        {STA_IND_Y, "STA", Addressing::indirect_indexed_y},

        {STX_ZP,    "STX", Addressing::zero_page},
        {STX_ZP_Y,  "STX", Addressing::zero_page_indexed_y},
        {STX_ABS,   "STX", Addressing::absolute},

        {STY_ZP,    "STY", Addressing::zero_page},
        {STY_ZP_X,  "STY", Addressing::zero_page_indexed_x},
        {STY_ABS,   "STY", Addressing::absolute},

        {TAX,       "TAX", Addressing::implied},

        {TAY,       "TAY", Addressing::implied},

        {TSX,       "TSX", Addressing::implied},

        {TXA,       "TXA", Addressing::implied},

        {TXS,       "TXS", Addressing::implied},

        {TYA,       "TYA", Addressing::implied},

        {ILL_RLA_IND_Y, "(RLA)", Addressing::zero_page_indexed_y}
};


Monitor::Monitor(Memory& memory) :
    memory(memory)
{
    for (auto& opcode : opcodes_list) {
        opcodes[opcode.opcode] = opcode;
    }

}

uint16_t Monitor::disassemble(uint16_t address, size_t bytes)
{
    uint16_t orig_address = address;

    while(address <= orig_address + bytes) {
        address = disassemble(address);
        std::cout << std::endl;
    }

    return address;
}


/* This function disassembles the opcode at the PC and outputs it in *output */
uint16_t Monitor::disassemble(uint16_t address)
{
    std::cout << std::hex << std::setfill('0') << std::uppercase
              << "$" << std::setw(4) << address << "\t";

    uint8_t op = memory.mem[address++];

    std::cout << "$" << std::setw(2) << (int)op << "\t";

    if (auto it = opcodes.find(op); it != opcodes.end())
    {
        std::cout << it->second.name;

        switch(it->second.addressing)
        {
            case Addressing::immediate:
            {
                uint8_t value = memory.mem[address++];
                std::cout << " #$" << std::setw(2) << (int)value << "\t";
                break;
            }
            case Addressing::zero_page:
            {
                uint8_t value = memory.mem[address++];
                std::cout << " $" << std::setw(2) << (int)value << "\t\t";
                break;
            }
            case Addressing::zero_page_indexed_x:
            {
                uint8_t value = memory.mem[address++];
                std::cout << " $" << std::setw(2) << (int)value << ",X\t";
                break;
            }
            case Addressing::zero_page_indexed_y:
            {
                uint8_t value = memory.mem[address++];
                std::cout << " $" << std::setw(2) << (int)value << ",Y\t";
                break;
            }
            case Addressing::implied:
            {
                std::cout << "\t\t";
                break;
            }
            case Addressing::indirect_absolute:
            {
                uint16_t value = memory.mem[address++];
                value += memory.mem[address++] << 8;
                std::cout << " ($" << std::setw(4) << (int)value << ")\t";
                break;
            }
            case Addressing::absolute:
            {
                uint16_t value = memory.mem[address++];
                value += memory.mem[address++] << 8;
                std::cout << " $" << std::setw(4) << (int)value << "\t";
                break;
            }
            case Addressing::absolute_indexed_x:
            {
                uint16_t value = memory.mem[address++];
                value += memory.mem[address++] << 8;
                std::cout << " $" << std::setw(4) << (int)value << ",X\t";
                break;
            }
            case Addressing::absolute_indexed_y:
            {
                uint16_t value = memory.mem[address++];
                value += memory.mem[address++] << 8;
                std::cout << " $" << std::setw(4) << (int)value << ",Y\t";
                break;
            }
            case Addressing::indexed_indirect_x:
            {
                uint8_t value = memory.mem[address++];
                std::cout << " ($" << std::setw(2) << (int)value << ",X)\t";
                break;
            }
            case Addressing::indirect_indexed_y:
            {
                uint8_t value = memory.mem[address++];
                std::cout << " ($" << std::setw(2) << (int)value << "),Y\t";
                break;
            }
            case Addressing::relative:
            {
                uint8_t rel = memory.mem[address++];
                uint16_t addr = rel & 0x80 ? (address - ((rel ^ 0xff)+1)) : (address + rel);
                std::cout << " $" << std::setw(4) << (int)addr << "\t";
                break;
            }
            case Addressing::accumulator:
                std::cout << " A\t\t";
                break;
        }

    }

    return address;
}

