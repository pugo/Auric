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

#ifndef CHIP_MOS6502_OPCODES_H
#define CHIP_MOS6502_OPCODES_H

//   7                           0
// +---+---+---+---+---+---+---+---+
// | N | V |   | B | D | I | Z | C |
// +---+---+---+---+---+---+---+---+
#define FLAG_N      0x80
#define FLAG_V      0x40
#define FLAG_B      0x10
#define FLAG_D      0x08
#define FLAG_I      0x04
#define FLAG_Z      0x02
#define FLAG_C      0x01


#define ADC_IMM     0x69
#define ADC_ZP      0x65
#define ADC_ZP_X    0x75
#define ADC_ABS     0x6D
#define ADC_ABS_X   0x7D
#define ADC_ABS_Y   0x79
#define ADC_IND_X   0x61
#define ADC_IND_Y   0x71

#define AND_IMM     0x29
#define AND_ZP      0x25
#define AND_ZP_X    0x35
#define AND_ABS     0x2D
#define AND_ABS_X   0x3D
#define AND_ABS_Y   0x39
#define AND_IND_X   0x21
#define AND_IND_Y   0x31

#define ASL_ACC     0x0A
#define ASL_ZP      0x06
#define ASL_ZP_X    0x16
#define ASL_ABS     0x0E
#define ASL_ABS_X   0x1E

#define BIT_ZP      0x24
#define BIT_ABS     0x2C

#define BPL         0x10
#define BMI         0x30
#define BVC         0x50
#define BVS         0x70
#define BCC         0x90
#define BCS         0xB0
#define BNE         0xD0
#define BEQ         0xF0

#define BRK         0x00

#define CMP_IMM     0xC9
#define CMP_ZP      0xC5
#define CMP_ZP_X    0xD5
#define CMP_ABS     0xCD
#define CMP_ABS_X   0xDD
#define CMP_ABS_Y   0xD9
#define CMP_IND_X   0xC1
#define CMP_IND_Y   0xD1

#define CPX_IMM     0xE0
#define CPX_ZP      0xE4
#define CPX_ABS     0xEC

#define CPY_IMM     0xC0
#define CPY_ZP      0xC4
#define CPY_ABS     0xCC

#define DEC_ZP      0xC6
#define DEC_ZP_X    0xD6
#define DEC_ABS     0xCE
#define DEC_ABS_X   0xDE

#define DEX         0xCA
#define DEY         0x88

#define EOR_IMM     0x49
#define EOR_ZP      0x45
#define EOR_ZP_X    0x55
#define EOR_ABS     0x4D
#define EOR_ABS_X   0x5D
#define EOR_ABS_Y   0x59
#define EOR_IND_X   0x41
#define EOR_IND_Y   0x51

#define CLC         0x18
#define CLD         0xD8
#define CLI         0x58
#define CLV         0xB8

#define INC_ZP      0xE6
#define INC_ZP_X    0xF6
#define INC_ABS     0xEE
#define INC_ABS_X   0xFE

#define INX         0xE8
#define INY         0xC8

#define JMP_ABS     0x4C
#define JMP_IND     0x6C

#define JSR         0x20

#define LDA_IMM     0xA9
#define LDA_ZP      0xA5
#define LDA_ZP_X    0xB5
#define LDA_ABS     0xAD
#define LDA_ABS_X   0xBD
#define LDA_ABS_Y   0xB9
#define LDA_IND_X   0xA1
#define LDA_IND_Y   0xB1

#define LDX_IMM     0xA2
#define LDX_ZP      0xA6
#define LDX_ZP_Y    0xB6
#define LDX_ABS     0xAE
#define LDX_ABS_Y   0xBE

#define LDY_IMM     0xA0
#define LDY_ZP      0xA4
#define LDY_ZP_X    0xB4
#define LDY_ABS     0xAC
#define LDY_ABS_X   0xBC

#define LSR_ZP      0x46
#define LSR_ACC     0x4A
#define LSR_ABS     0x4E
#define LSR_ZP_X    0x56
#define LSR_ABS_X   0x5E

#define NOP         0xEA

#define ORA_IMM     0x09
#define ORA_ZP      0x05
#define ORA_ZP_X    0x15
#define ORA_ABS     0x0D
#define ORA_ABS_X   0x1D
#define ORA_ABS_Y   0x19
#define ORA_IND_X   0x01
#define ORA_IND_Y   0x11

#define PHP         0x08
#define PHA         0x48
#define PLA         0x68
#define PLP         0x28

#define ROL_ACC     0x2A
#define ROL_ZP      0x26
#define ROL_ZP_X    0x36
#define ROL_ABS     0x2E
#define ROL_ABS_X   0x3E

#define ROR_ACC     0x6A
#define ROR_ZP      0x66
#define ROR_ZP_X    0x76
#define ROR_ABS     0x6E
#define ROR_ABS_X   0x7E

#define RTI         0x40
#define RTS         0x60

#define SBC_IMM     0xE9
#define SBC_ZP      0xE5
#define SBC_ZP_X    0xF5
#define SBC_ABS     0xED
#define SBC_ABS_X   0xFD
#define SBC_ABS_Y   0xF9
#define SBC_IND_X   0xE1
#define SBC_IND_Y   0xF1

#define SEC         0x38
#define SED         0xF8
#define SEI         0x78

#define STA_ZP      0x85
#define STA_ZP_X    0x95
#define STA_ABS     0x8D
#define STA_ABS_X   0x9D
#define STA_ABS_Y   0x99
#define STA_IND_X   0x81
#define STA_IND_Y   0x91

#define STX_ZP      0x86
#define STX_ZP_Y    0x96
#define STX_ABS     0x8E

#define STY_ZP      0x84
#define STY_ZP_X    0x94
#define STY_ABS     0x8C

#define TAX         0xAA
#define TAY         0xA8
#define TSX         0xBA
#define TXA         0x8A
#define TXS         0x9A
#define TYA         0x98

#define ILL_LAX_ZP      0xA7
#define ILL_LAX_ZP_Y    0xB7
#define ILL_LAX_ABS     0xAF
#define ILL_LAX_ABS_Y   0xBF
#define ILL_LAX_IND_X   0xA3
#define ILL_LAX_IND_Y   0xB3   // <-- the one you hit

#define ILL_NOP_IMP_1A  0x1A
#define ILL_NOP_IMP_3A  0x3A
#define ILL_NOP_IMP_5A  0x5A
#define ILL_NOP_IMP_7A  0x7A
#define ILL_NOP_IMP_DA  0xDA
#define ILL_NOP_IMP_FA  0xFA

#define ILL_NOP_IMM_80  0x80
#define ILL_NOP_IMM_82  0x82
#define ILL_NOP_IMM_89  0x89
#define ILL_NOP_IMM_C2  0xC2
#define ILL_NOP_IMM_E2  0xE2

#define ILL_NOP_ZP_04   0x04
#define ILL_NOP_ZP_44   0x44
#define ILL_NOP_ZP_64   0x64

#define ILL_NOP_ZPX_14  0x14
#define ILL_NOP_ZPX_34  0x34
#define ILL_NOP_ZPX_54  0x54
#define ILL_NOP_ZPX_74  0x74
#define ILL_NOP_ZPX_D4  0xD4
#define ILL_NOP_ZPX_F4  0xF4

#define ILL_NOP_ABS_0C  0x0C

#define ILL_NOP_ABS_X_1C  0x1C
#define ILL_NOP_ABS_X_3C  0x3C
#define ILL_NOP_ABS_X_5C  0x5C
#define ILL_NOP_ABS_X_7C  0x7C
#define ILL_NOP_ABS_X_DC  0xDC
#define ILL_NOP_ABS_X_FC  0xFC

#define ILL_SLO_IND_X 0x03
#define ILL_SLO_IND_Y 0x13

#define ILL_RLA_IND_Y 0x33

#define ILL_SBX       0xCB

#define ILL_ISC_ABS_X 0xFF


#endif // CHIP_MOS6502_OPCODES_H
