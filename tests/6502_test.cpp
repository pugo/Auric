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

#include <memory>
#include <gtest/gtest.h>

#include "../config.hpp"
#include "../oric.hpp"


namespace Unittest {

using namespace testing;


class MOS6502Test : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        Config config;

        oric = new Oric(config);
        oric->init_machine();
        oric->get_machine().init_cpu();
        oric->get_machine().reset();
        oric->get_machine().cpu->set_pc(0);
    }

    virtual void TearDown()
    {
        delete oric;
    }

    void run(Machine& machine) {
        bool brk = false;
        while (! brk) {
            machine.cpu->exec(true, brk);
        }
    }

    int decToBCD(int dec)
    {
        int major = dec / 10;
        int minor = dec - major*10;
        //std::cout << "D2BCD - major: " << major << ", minor: " << minor << ", result: "
        //          << std::hex << major * 16 + minor << std::endl;
        return major * 16 + minor;
    }

    int bcdToDec(int bcd)
    {
        int major = bcd / 16;
        int minor = bcd - major*16;
        //std::cout << "BCD2D - major: " << major << ", minor: " << minor << ", result: "
        //          << std::hex << major * 16 + minor << std::endl;
        return major * 10 + minor;
    }

    Oric* oric;
};

// --- LDA ---

TEST_F(MOS6502Test, OpLDA_IMM)
{
    Machine& machine = oric->get_machine();
    machine.memory << LDA_IMM << 0x1f;
    machine.memory << BRK;
    run(machine);

    ASSERT_EQ(machine.cpu->A, 0x1f);
}

TEST_F(MOS6502Test, OpLDA_ZP)
{
    Machine& machine = oric->get_machine();
    machine.memory.mem[0x10] = 0x2f;

    machine.memory << LDA_ZP << 0x10;
    machine.memory << BRK;
    run(machine);

    ASSERT_EQ(machine.cpu->A, 0x2f);
}

TEST_F(MOS6502Test, OpLDA_ZP_X)
{
    Machine& machine = oric->get_machine();
    machine.memory.mem[0x15] = 0x3f;
    machine.cpu->X = 0x05;

    machine.memory << LDA_ZP_X << 0x10;
    machine.memory << BRK;
    run(machine);

    ASSERT_EQ(machine.cpu->A, 0x3f);
}

TEST_F(MOS6502Test, OpLDA_ABS)
{
    Machine& machine = oric->get_machine();
    machine.memory.mem[0x1234] = 0x4f;

    machine.memory << LDA_ABS << 0x34 << 0x12;
    machine.memory << BRK;
    run(machine);

    ASSERT_EQ(machine.cpu->A, 0x4f);
}

TEST_F(MOS6502Test, OpLDA_ABS_X)
{
    Machine& machine = oric->get_machine();
    machine.memory.mem[0x1122] = 0x5f;
    machine.cpu->X = 0x11;

    machine.memory << LDA_ABS_X << 0x11 << 0x11;
    machine.memory << BRK;
    run(machine);

    ASSERT_EQ(machine.cpu->A, 0x5f);
}

TEST_F(MOS6502Test, OpLDA_ABS_Y)
{
    Machine& machine = oric->get_machine();
    machine.memory.mem[0x2233] = 0x6f;
    machine.cpu->Y = 0x11;

    machine.memory << LDA_ABS_Y << 0x22 << 0x22;
    machine.memory << BRK;
    run(machine);

    ASSERT_EQ(machine.cpu->A, 0x6f);
}

TEST_F(MOS6502Test, OpLDA_IND_X)
{
    Machine& machine = oric->get_machine();
    machine.memory.mem[0x4711] = 0x7f;
    machine.memory.mem[0x14] = 0x11;
    machine.memory.mem[0x15] = 0x47;
    machine.cpu->X = 0x04;

    machine.memory << LDA_IND_X << 0x10;
    machine.memory << BRK;
    run(machine);

    ASSERT_EQ(machine.cpu->X, 0x04);
    ASSERT_EQ(machine.cpu->A, 0x7f);
}

TEST_F(MOS6502Test, OpLDA_IND_Y)
{
    Machine& machine = oric->get_machine();
    machine.memory.mem[0x4711] = 0x8f;
    machine.memory.mem[0x10] = 0x00;
    machine.memory.mem[0x11] = 0x47;
    machine.cpu->Y = 0x11;

    machine.memory << LDA_IND_Y << 0x10;
    machine.memory << BRK;
    run(machine);

    ASSERT_EQ(machine.cpu->Y, 0x11);
    ASSERT_EQ(machine.cpu->A, 0x8f);
}


// --- LDX ---

TEST_F(MOS6502Test, OpLDX_IMM)
{
    Machine& machine = oric->get_machine();
    machine.memory << LDX_IMM << 0x1f;
    machine.memory << BRK;
    run(machine);

    ASSERT_EQ(machine.cpu->X, 0x1f);
}

TEST_F(MOS6502Test, OpLDX_ZP)
{
    Machine& machine = oric->get_machine();
    machine.memory.mem[0x10] = 0x2f;

    machine.memory << LDX_ZP << 0x10;
    machine.memory << BRK;
    run(machine);

    ASSERT_EQ(machine.cpu->X, 0x2f);
}

TEST_F(MOS6502Test, OpLDX_ZP_Y)
{
    Machine& machine = oric->get_machine();
    machine.memory.mem[0x15] = 0x3f;
    machine.cpu->Y = 0x05;

    machine.memory << LDX_ZP_Y << 0x10;
    machine.memory << BRK;
    run(machine);

    ASSERT_EQ(machine.cpu->X, 0x3f);
}

TEST_F(MOS6502Test, OpLDX_ABS)
{
    Machine& machine = oric->get_machine();
    machine.memory.mem[0x1234] = 0x4f;

    machine.memory << LDX_ABS << 0x34 << 0x12;
    machine.memory << BRK;
    run(machine);

    ASSERT_EQ(machine.cpu->X, 0x4f);
}

TEST_F(MOS6502Test, OpLDX_ABS_Y)
{
    Machine& machine = oric->get_machine();
    machine.memory.mem[0x1122] = 0x5f;
    machine.cpu->Y = 0x11;

    machine.memory << LDX_ABS_Y << 0x11 << 0x11;
    machine.memory << BRK;
    run(machine);

    ASSERT_EQ(machine.cpu->X, 0x5f);
}


// --- LDY ---

TEST_F(MOS6502Test, OpLDY_IMM)
{
    Machine& machine = oric->get_machine();
    machine.memory << LDY_IMM << 0x1f;
    machine.memory << BRK;
    run(machine);

    ASSERT_EQ(machine.cpu->Y, 0x1f);
}

TEST_F(MOS6502Test, OpLDY_ZP)
{
    Machine& machine = oric->get_machine();
    machine.memory.mem[0x10] = 0x2f;

    machine.memory << LDY_ZP << 0x10;
    machine.memory << BRK;
    run(machine);

    ASSERT_EQ(machine.cpu->Y, 0x2f);
}

TEST_F(MOS6502Test, OpLDY_ZP_X)
{
    Machine& machine = oric->get_machine();
    machine.memory.mem[0x15] = 0x3f;
    machine.cpu->X = 0x05;

    machine.memory << LDY_ZP_X << 0x10;
    machine.memory << BRK;
    run(machine);

    ASSERT_EQ(machine.cpu->Y, 0x3f);
}

TEST_F(MOS6502Test, OpLDY_ABS)
{
    Machine& machine = oric->get_machine();
    machine.memory.mem[0x1234] = 0x4f;

    machine.memory << LDY_ABS << 0x34 << 0x12;
    machine.memory << BRK;
    run(machine);

    ASSERT_EQ(machine.cpu->Y, 0x4f);
}

TEST_F(MOS6502Test, OpLDY_ABS_X)
{
    Machine& machine = oric->get_machine();
    machine.memory.mem[0x1122] = 0x5f;
    machine.cpu->X = 0x11;

    machine.memory << LDY_ABS_X << 0x11 << 0x11;
    machine.memory << BRK;
    run(machine);

    ASSERT_EQ(machine.cpu->Y, 0x5f);
}



// --- STA ---

TEST_F(MOS6502Test, OpSTA_ZP)
{
    Machine& machine = oric->get_machine();
    machine.cpu->A = 0x1f;

    machine.memory << STA_ZP << 0x10;
    machine.memory << BRK;
    run(machine);

    ASSERT_EQ(machine.memory.mem[0x10], 0x1f);
}

TEST_F(MOS6502Test, OpSTA_ZP_X)
{
    Machine& machine = oric->get_machine();
    machine.cpu->A = 0x2f;
    machine.cpu->X = 0x05;

    machine.memory << STA_ZP_X << 0x10;
    machine.memory << BRK;
    run(machine);

    ASSERT_EQ(machine.memory.mem[0x15], 0x2f);
}

TEST_F(MOS6502Test, OpSTA_ABS)
{
    Machine& machine = oric->get_machine();
    machine.cpu->A = 0x3f;

    machine.memory << STA_ABS << 0x34 << 0x12;
    machine.memory << BRK;
    run(machine);

    ASSERT_EQ(machine.memory.mem[0x1234], 0x3f);
}

TEST_F(MOS6502Test, OpSTA_ABS_X)
{
    Machine& machine = oric->get_machine();
    machine.cpu->A = 0x4f;
    machine.cpu->X = 0x11;

    machine.memory << STA_ABS_X << 0x11 << 0x11;
    machine.memory << BRK;
    run(machine);

    ASSERT_EQ(machine.memory.mem[0x1122], 0x4f);
}

TEST_F(MOS6502Test, OpSTA_ABS_Y)
{
    Machine& machine = oric->get_machine();
    machine.cpu->A = 0x5f;
    machine.cpu->Y = 0x11;

    machine.memory << STA_ABS_Y << 0x22 << 0x22;
    machine.memory << BRK;
    run(machine);

    ASSERT_EQ(machine.memory.mem[0x2233], 0x5f);
}

TEST_F(MOS6502Test, OpSTA_IND_X)
{
    Machine& machine = oric->get_machine();
    machine.memory.mem[0x14] = 0x11;
    machine.memory.mem[0x15] = 0x47;
    machine.cpu->A = 0x6f;
    machine.cpu->X = 0x04;

    machine.memory << STA_IND_X << 0x10;
    machine.memory << BRK;
    run(machine);

    ASSERT_EQ(machine.cpu->X, 0x04);
    ASSERT_EQ(machine.memory.mem[0x4711], 0x6f);
}

TEST_F(MOS6502Test, OpSTA_IND_Y)
{
    Machine& machine = oric->get_machine();
    machine.memory.mem[0x10] = 0x00;
    machine.memory.mem[0x11] = 0x47;
    machine.cpu->A = 0x7f;
    machine.cpu->Y = 0x11;

    machine.memory << STA_IND_Y << 0x10;
    machine.memory << BRK;
    run(machine);

    ASSERT_EQ(machine.cpu->Y, 0x11);
    ASSERT_EQ(machine.memory.mem[0x4711], 0x7f);
}


// --- STX ---

TEST_F(MOS6502Test, OpSTX_ZP)
{
    Machine& machine = oric->get_machine();
    machine.cpu->X = 0x1f;

    machine.memory << STX_ZP << 0x10;
    machine.memory << BRK;
    run(machine);

    ASSERT_EQ(machine.memory.mem[0x10], 0x1f);
}

TEST_F(MOS6502Test, OpSTX_ZP_Y)
{
    Machine& machine = oric->get_machine();
    machine.cpu->X = 0x2f;
    machine.cpu->Y = 0x05;

    machine.memory << STX_ZP_Y << 0x10;
    machine.memory << BRK;
    run(machine);

    ASSERT_EQ(machine.memory.mem[0x15], 0x2f);
}

TEST_F(MOS6502Test, OpSTX_ABS)
{
    Machine& machine = oric->get_machine();
    machine.cpu->X = 0x3f;

    machine.memory << STX_ABS << 0x34 << 0x12;
    machine.memory << BRK;
    run(machine);

    ASSERT_EQ(machine.memory.mem[0x1234], 0x3f);
}


// --- STY ---

TEST_F(MOS6502Test, OpSTY_ZP)
{
    Machine& machine = oric->get_machine();
    machine.cpu->Y = 0x1f;

    machine.memory << STY_ZP << 0x10;
    machine.memory << BRK;
    run(machine);

    ASSERT_EQ(machine.memory.mem[0x10], 0x1f);
}

TEST_F(MOS6502Test, OpSTY_ZP_X)
{
    Machine& machine = oric->get_machine();
    machine.cpu->Y = 0x2f;
    machine.cpu->X = 0x05;

    machine.memory << STY_ZP_X << 0x10;
    machine.memory << BRK;
    run(machine);

    ASSERT_EQ(machine.memory.mem[0x15], 0x2f);
}

TEST_F(MOS6502Test, OpSTY_ABS)
{
    Machine& machine = oric->get_machine();
    machine.cpu->Y = 0x3f;

    machine.memory << STY_ABS << 0x34 << 0x12;
    machine.memory << BRK;
    run(machine);

    ASSERT_EQ(machine.memory.mem[0x1234], 0x3f);
}


// --- ADC ---


// Normal mode
TEST_F(MOS6502Test, OpADC_IMM)
{
    Machine& machine = oric->get_machine();

    for (int i=0; i<12; i++)
    {
        machine.cpu->D = false;
        machine.cpu->A = 0x11;

        machine.memory.set_mem_pos(0);
        machine.memory << CLC;
        machine.memory << ADC_IMM;
        machine.memory << (0x13 * i);
        machine.memory << BRK;

        run(machine);
        ASSERT_EQ(machine.cpu->A, 0x11 + 0x13 * i);
    }
}

// Decimal mode
TEST_F(MOS6502Test, OpADC_IMM_DEC)
{
    // Decimal mode
    short a[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
    short b[] = {0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11};
    short v[] = {0,    1,    1,    1,    1,    1,    1,    0 };

    Machine& machine = oric->get_machine();

    for (int i=0; i < 8; i++)
    {
        machine.cpu->D = true;
        machine.cpu->A = a[i];

        machine.memory.set_mem_pos(0);
        machine.memory << CLC;
        machine.memory << ADC_IMM;
        machine.memory << b[i];
        machine.memory << BRK;

        run(machine);
        ASSERT_EQ(machine.cpu->A, 0x99);
        ASSERT_EQ(machine.cpu->V, v[i]);
    }
}

// Decimal mode
TEST_F(MOS6502Test, OpADC_IMM_DEC2)
{
    Machine& machine = oric->get_machine();

    for (int a=0; a <= 99; a++)
    {
        for (int b=0; b <= 99; b++)
        {
            machine.cpu->D = true;
            machine.cpu->A = decToBCD(a);

            machine.memory.set_mem_pos(0);
            machine.memory << CLC;
            machine.memory << ADC_IMM;
            machine.memory << decToBCD(b);
            machine.memory << BRK;

            run(machine);
            ASSERT_EQ(bcdToDec(machine.cpu->A), (a+b) % 100);
        }
    }
}

// Normal mode
TEST_F(MOS6502Test, OpADC_ZP)
{
    Machine& machine = oric->get_machine();

    for (int i=0; i<12; i++)
    {
        machine.cpu->D = false;
        machine.cpu->A = 0x11;
        machine.memory.mem[0x15] = 0x13 * i;

        machine.memory.set_mem_pos(0);
        machine.memory << CLC;
        machine.memory << ADC_ZP;
        machine.memory << 0x15;
        machine.memory << BRK;

        run(machine);
        ASSERT_EQ(machine.cpu->A, 0x11 + 0x13 * i);
    }
}

// Decimal mode
TEST_F(MOS6502Test, OpADC_ZP_DEC)
{
    Machine& machine = oric->get_machine();

    for (int a=0; a <= 99; a++)
    {
        for (int b=0; b <= 99; b++)
        {
            machine.cpu->D = true;
            machine.cpu->A = decToBCD(a);
            machine.memory.mem[0x15] = decToBCD(b);

            machine.memory.set_mem_pos(0);
            machine.memory << CLC;
            machine.memory << ADC_ZP;
            machine.memory << 0x15;
            machine.memory << BRK;

            run(machine);
            ASSERT_EQ(bcdToDec(machine.cpu->A), (a+b) % 100);
        }
    }
}


// Normal mode
TEST_F(MOS6502Test, OpADC_ZP_X)
{
    Machine& machine = oric->get_machine();

    for (int i=0; i<12; i++)
    {
        machine.cpu->D = false;
        machine.cpu->A = 0x11;
        machine.cpu->X = 0x05;
        machine.memory.mem[0x15] = 0x13 * i;

        machine.memory.set_mem_pos(0);
        machine.memory << CLC;
        machine.memory << ADC_ZP_X;
        machine.memory << 0x10;
        machine.memory << BRK;

        run(machine);
        ASSERT_EQ(machine.cpu->A, 0x11 + 0x13 * i);
    }
}

// Decimal mode
TEST_F(MOS6502Test, OpADC_ZP_X_DEC)
{
    Machine& machine = oric->get_machine();

    for (int a=0; a <= 99; a++)
    {
        for (int b=0; b <= 99; b++)
        {
            machine.cpu->D = true;
            machine.cpu->A = decToBCD(a);
            machine.cpu->X = 0x05;
            machine.memory.mem[0x15] = decToBCD(b);

            machine.memory.set_mem_pos(0);
            machine.memory << CLC;
            machine.memory << ADC_ZP_X;
            machine.memory << 0x10;
            machine.memory << BRK;

            run(machine);
            ASSERT_EQ(bcdToDec(machine.cpu->A), (a+b) % 100);
        }
    }
}

// Normal mode
TEST_F(MOS6502Test, OpADC_ABS)
{
    Machine& machine = oric->get_machine();

    for (int i=0; i < 12; i++)
    {
        machine.cpu->D = false;
        machine.cpu->A = 0x12;
        machine.memory.mem[0x1234] = 0x13 * i;

        machine.memory.set_mem_pos(0);
        machine.memory << CLC;
        machine.memory << ADC_ABS;
        machine.memory << 0x34 << 0x12;
        machine.memory << BRK;

        run(machine);
        ASSERT_EQ(machine.cpu->A, 0x12 + 0x13 * i);
    }
}

// Decimal mode
TEST_F(MOS6502Test, OpADC_ABS_DEC)
{
    Machine& machine = oric->get_machine();

    for (int a=0; a <= 99; a++)
    {
        for (int b=0; b <= 99; b++)
        {
            machine.cpu->D = true;
            machine.cpu->A = decToBCD(a);
            machine.memory.mem[0x1234] = decToBCD(b);

            machine.memory.set_mem_pos(0);
            machine.memory << CLC;
            machine.memory << ADC_ABS;
            machine.memory << 0x34 << 0x12;
            machine.memory << BRK;

            run(machine);
            ASSERT_EQ(bcdToDec(machine.cpu->A), (a+b) % 100);
        }
    }
}

// Normal mode
TEST_F(MOS6502Test, OpADC_ABS_X)
{
    Machine& machine = oric->get_machine();

    for (int i=0; i < 12; i++)
    {
        machine.cpu->D = false;
        machine.cpu->A = 0x12;
        machine.cpu->X = 0x11;
        machine.memory.mem[0x1245] = 0x13 * i;

        machine.memory.set_mem_pos(0);
        machine.memory << CLC;
        machine.memory << ADC_ABS_X;
        machine.memory << 0x34 << 0x12;
        machine.memory << BRK;

        run(machine);
        ASSERT_EQ(machine.cpu->A, 0x12 + 0x13 * i);
    }
}

// Decimal mode
TEST_F(MOS6502Test, OpADC_ABS_X_DEC)
{
    Machine& machine = oric->get_machine();

    for (int a=0; a <= 99; a++)
    {
        for (int b=0; b <= 99; b++)
        {
            machine.cpu->D = true;
            machine.cpu->A = decToBCD(a);
            machine.cpu->X = 0x11;
            machine.memory.mem[0x1245] = decToBCD(b);

            machine.memory.set_mem_pos(0);
            machine.memory << CLC;
            machine.memory << ADC_ABS_X;
            machine.memory << 0x34 << 0x12;
            machine.memory << BRK;

            run(machine);
            ASSERT_EQ(bcdToDec(machine.cpu->A), (a+b) % 100);
        }
    }
}

// Normal mode
TEST_F(MOS6502Test, OpADC_ABS_Y)
{
    Machine& machine = oric->get_machine();

    for (int i=0; i < 12; i++)
    {
        machine.cpu->D = false;
        machine.cpu->A = 0x12;
        machine.cpu->Y = 0x12;
        machine.memory.mem[0x1246] = 0x13 * i;

        machine.memory.set_mem_pos(0);
        machine.memory << CLC;
        machine.memory << ADC_ABS_Y;
        machine.memory << 0x34 << 0x12;
        machine.memory << BRK;

        run(machine);
        ASSERT_EQ(machine.cpu->A, 0x12 + 0x13 * i);
    }
}

// Decimal mode
TEST_F(MOS6502Test, OpADC_ABS_Y_DEC)
{
    Machine& machine = oric->get_machine();

    for (int a=0; a <= 99; a++)
    {
        for (int b=0; b <= 99; b++)
        {
            machine.cpu->D = true;
            machine.cpu->A = decToBCD(a);
            machine.cpu->Y = 0x12;
            machine.memory.mem[0x1246] = decToBCD(b);

            machine.memory.set_mem_pos(0);
            machine.memory << CLC;
            machine.memory << ADC_ABS_Y;
            machine.memory << 0x34 << 0x12;
            machine.memory << BRK;

            run(machine);
            ASSERT_EQ(bcdToDec(machine.cpu->A), (a+b) % 100);
        }
    }
}

// Normal mode
TEST_F(MOS6502Test, OpADC_IND_X)
{
    Machine& machine = oric->get_machine();

    for (int i=0; i < 12; i++)
    {
        machine.cpu->D = false;
        machine.cpu->A = 0x12;
        machine.cpu->X = 0x04;

        machine.memory.mem[0x14] = 0x11;
        machine.memory.mem[0x15] = 0x47;
        machine.memory.mem[0x4711] = 0x13 * i;

        machine.memory.set_mem_pos(0);
        machine.memory << CLC;
        machine.memory << ADC_IND_X;
        machine.memory << 0x10;
        machine.memory << BRK;

        run(machine);
        ASSERT_EQ(machine.cpu->A, 0x12 + 0x13 * i);
    }
}

// Decimal mode
TEST_F(MOS6502Test, OpADC_IND_X_DEC)
{
    Machine& machine = oric->get_machine();

    for (int a=0; a <= 99; a++)
    {
        for (int b=0; b <= 99; b++)
        {
            machine.cpu->D = true;
            machine.cpu->A = decToBCD(a);
            machine.cpu->X = 0x04;

            machine.memory.mem[0x14] = 0x11;
            machine.memory.mem[0x15] = 0x47;
            machine.memory.mem[0x4711] = decToBCD(b);

            machine.memory.set_mem_pos(0);
            machine.memory << CLC;
            machine.memory << ADC_IND_X;
            machine.memory << 0x10;
            machine.memory << BRK;

            run(machine);
            ASSERT_EQ(bcdToDec(machine.cpu->A), (a+b) % 100);
        }
    }
}

// Normal mode
TEST_F(MOS6502Test, OpADC_IND_Y)
{
    Machine& machine = oric->get_machine();

    for (int i=0; i < 12; i++)
    {
        machine.cpu->D = false;
        machine.cpu->A = 0x12;
        machine.cpu->Y = 0x11;

        machine.memory.mem[0x10] = 0x00;
        machine.memory.mem[0x11] = 0x47;
        machine.memory.mem[0x4711] = 0x13 * i;

        machine.memory.set_mem_pos(0);
        machine.memory << CLC;
        machine.memory << ADC_IND_Y;
        machine.memory << 0x10;
        machine.memory << BRK;

        run(machine);
        ASSERT_EQ(machine.cpu->A, 0x12 + 0x13 * i);
    }
}

// Decimal mode
TEST_F(MOS6502Test, OpADC_IND_Y_DEC)
{
    Machine& machine = oric->get_machine();

    for (int a=0; a <= 99; a++)
    {
        for (int b=0; b <= 99; b++)
        {
            machine.cpu->D = true;
            machine.cpu->A = decToBCD(a);
            machine.cpu->Y = 0x11;

            machine.memory.mem[0x10] = 0x00;
            machine.memory.mem[0x11] = 0x47;
            machine.memory.mem[0x4711] = decToBCD(b);

            machine.memory.set_mem_pos(0);
            machine.memory << CLC;
            machine.memory << ADC_IND_Y;
            machine.memory << 0x10;
            machine.memory << BRK;

            run(machine);
            ASSERT_EQ(bcdToDec(machine.cpu->A), (a+b) % 100);
        }
    }
}


// --- SBC ---

// Normal mode
TEST_F(MOS6502Test, OpSBC_IMM)
{
    Machine& machine = oric->get_machine();

    for (int i=0; i < 12; i++)
    {
        machine.cpu->D = false;
        machine.cpu->A = 0x11;

        machine.memory.set_mem_pos(0);
        machine.memory << SEC;
        machine.memory << SBC_IMM;
        machine.memory << (0x13 * i);
        machine.memory << BRK;

        run(machine);
        int ok = 0x11 - 0x13*i;
        if (ok < 0) ok = 256 - abs(ok);
        ASSERT_EQ(machine.cpu->A, ok);
    }
}

// Decimal mode
TEST_F(MOS6502Test, OpSBC_IMM_DEC)
{
    Machine& machine = oric->get_machine();

    for (int a=0; a <= 99; a++)
    {
        for (int b=0; b <= 99; b++)
        {
            machine.cpu->D = true;
            machine.cpu->A = decToBCD(a);

            machine.memory.set_mem_pos(0);
            machine.memory << SEC;
            machine.memory << SBC_IMM;
            machine.memory << decToBCD(b);
            machine.memory << BRK;

            run(machine);
            int ok = a - b;
            if (ok < 0) ok = 100 - abs(ok);
            ASSERT_EQ(bcdToDec(machine.cpu->A), ok);
        }
    }
}

// Normal mode
TEST_F(MOS6502Test, OpSBC_ZP)
{
    Machine& machine = oric->get_machine();

    for (int i=0; i < 12; i++)
    {
        machine.cpu->D = false;
        machine.cpu->A = 0x11;
        machine.memory.mem[0x15] = 0x13 * i;

        machine.memory.set_mem_pos(0);
        machine.memory << SEC;
        machine.memory << SBC_ZP;
        machine.memory << 0x15;
        machine.memory << BRK;

        run(machine);
        int ok = 0x11 - 0x13*i;
        if (ok < 0) ok = 256 - abs(ok);
        ASSERT_EQ(machine.cpu->A, ok);
    }
}

// Decimal mode
TEST_F(MOS6502Test, OpSBC_ZP_DEC)
{
    Machine& machine = oric->get_machine();

    for (int a=0; a <= 99; a++)
    {
        for (int b=0; b <= 99; b++)
        {
            machine.cpu->D = true;
            machine.cpu->A = decToBCD(a);
            machine.memory.mem[0x15] = decToBCD(b);

            machine.memory.set_mem_pos(0);
            machine.memory << SEC;
            machine.memory << SBC_ZP;
            machine.memory << 0x15;
            machine.memory << BRK;

            run(machine);
            int ok = a - b;
            if (ok < 0) ok = 100 - abs(ok);
            ASSERT_EQ(bcdToDec(machine.cpu->A), ok);
        }
    }
}

// Normal mode
TEST_F(MOS6502Test, OpSBC_ZP_X)
{
    Machine& machine = oric->get_machine();

    for (int i=0; i < 12; i++)
    {
        machine.cpu->D = false;
        machine.cpu->A = 0x11;
        machine.cpu->X = 0x05;
        machine.memory.mem[0x15] = 0x13 * i;

        machine.memory.set_mem_pos(0);
        machine.memory << SEC;
        machine.memory << SBC_ZP_X;
        machine.memory << 0x10;
        machine.memory << BRK;

        run(machine);
        int ok = 0x11 - 0x13*i;
        if (ok < 0) ok = 256 - abs(ok);
        ASSERT_EQ(machine.cpu->A, ok);
    }
}

// Decimal mode
TEST_F(MOS6502Test, OpSBC_ZP_X_DEC)
{
    Machine& machine = oric->get_machine();

    for (int a=0; a <= 99; a++)
    {
        for (int b=0; b <= 99; b++)
        {
            machine.cpu->D = true;
            machine.cpu->A = decToBCD(a);
            machine.cpu->X = 0x05;
            machine.memory.mem[0x15] = decToBCD(b);

            machine.memory.set_mem_pos(0);
            machine.memory << SEC;
            machine.memory << SBC_ZP_X;
            machine.memory << 0x10;
            machine.memory << BRK;

            run(machine);
            int ok = a - b;
            if (ok < 0) ok = 100 - abs(ok);
            ASSERT_EQ(bcdToDec(machine.cpu->A), ok);
        }
    }
}

// Normal mode
TEST_F(MOS6502Test, OpSBC_ABS)
{
    Machine& machine = oric->get_machine();

    for (int i=0; i < 12; i++)
    {
        machine.cpu->D = false;
        machine.cpu->A = 0x12;
        machine.memory.mem[0x1234] = 0x13 * i;

        machine.memory.set_mem_pos(0);
        machine.memory << SEC;
        machine.memory << SBC_ABS;
        machine.memory << 0x34 << 0x12;
        machine.memory << BRK;

        run(machine);
        int ok = 0x12 - 0x13*i;
        if (ok < 0) ok = 256 - abs(ok);
        ASSERT_EQ(machine.cpu->A, ok);
    }
}

// Decimal mode
TEST_F(MOS6502Test, OpSBC_ABS_DEC)
{
    Machine& machine = oric->get_machine();

    for (int a=0; a <= 99; a++)
    {
        for (int b=0; b <= 99; b++)
        {
            machine.cpu->D = true;
            machine.cpu->A = decToBCD(a);
            machine.memory.mem[0x1234] = decToBCD(b);

            machine.memory.set_mem_pos(0);
            machine.memory << SEC;
            machine.memory << SBC_ABS;
            machine.memory << 0x34 << 0x12;
            machine.memory << BRK;

            run(machine);
            int ok = a - b;
            if (ok < 0) ok = 100 - abs(ok);
            ASSERT_EQ(bcdToDec(machine.cpu->A), ok);
        }
    }
}

// Normal mode
TEST_F(MOS6502Test, OpSBC_ABS_X)
{
    Machine& machine = oric->get_machine();

    for (int i=0; i < 12; i++)
    {
        machine.cpu->D = false;
        machine.cpu->A = 0x12;
        machine.cpu->X = 0x11;
        machine.memory.mem[0x1245] = 0x13 * i;

        machine.memory.set_mem_pos(0);
        machine.memory << SEC;
        machine.memory << SBC_ABS_X;
        machine.memory << 0x34 << 0x12;
        machine.memory << BRK;

        run(machine);
        int ok = 0x12 - 0x13*i;
        if (ok < 0) ok = 256 - abs(ok);
        ASSERT_EQ(machine.cpu->A, ok);
    }
}

// Decimal mode
TEST_F(MOS6502Test, OpSBC_ABS_X_DEC)
{
    Machine& machine = oric->get_machine();

    for (int a=0; a <= 99; a++)
    {
        for (int b=0; b <= 99; b++)
        {
            machine.cpu->D = true;
            machine.cpu->A = decToBCD(a);
            machine.cpu->X = 0x11;
            machine.memory.mem[0x1245] = decToBCD(b);

            machine.memory.set_mem_pos(0);
            machine.memory << SEC;
            machine.memory << SBC_ABS_X;
            machine.memory << 0x34 << 0x12;
            machine.memory << BRK;

            run(machine);
            int ok = a - b;
            if (ok < 0) ok = 100 - abs(ok);
            ASSERT_EQ(bcdToDec(machine.cpu->A), ok);
        }
    }
}

// Normal mode
TEST_F(MOS6502Test, OpSBC_ABS_Y)
{
    Machine& machine = oric->get_machine();

    for (int i=0; i < 12; i++)
    {
        machine.cpu->D = false;
        machine.cpu->A = 0x12;
        machine.cpu->Y = 0x12;
        machine.memory.mem[0x1246] = 0x13 * i;

        machine.memory.set_mem_pos(0);
        machine.memory << SEC;
        machine.memory << SBC_ABS_Y;
        machine.memory << 0x34 << 0x12;
        machine.memory << BRK;

        run(machine);
        int ok = 0x12 - 0x13*i;
        if (ok < 0) ok = 256 - abs(ok);
        ASSERT_EQ(machine.cpu->A, ok);
    }
}

// Decimal mode
TEST_F(MOS6502Test, OpSBC_ABS_Y_DEC)
{
    Machine& machine = oric->get_machine();

    for (int a=0; a <= 99; a++)
    {
        for (int b=0; b <= 99; b++)
        {
            machine.cpu->D = true;
            machine.cpu->A = decToBCD(a);
            machine.cpu->Y = 0x12;
            machine.memory.mem[0x1246] = decToBCD(b);

            machine.memory.set_mem_pos(0);
            machine.memory << SEC;
            machine.memory << SBC_ABS_Y;
            machine.memory << 0x34 << 0x12;
            machine.memory << BRK;

            run(machine);
            int ok = a - b;
            if (ok < 0) ok = 100 - abs(ok);
            ASSERT_EQ(bcdToDec(machine.cpu->A), ok);
        }
    }
}

// Normal mode
TEST_F(MOS6502Test, OpSBC_IND_X)
{
    Machine& machine = oric->get_machine();

    for (int i=0; i < 12; i++)
    {
        machine.cpu->D = false;
        machine.cpu->A = 0x12;
        machine.cpu->X = 0x04;
        machine.memory.mem[0x14] = 0x11;
        machine.memory.mem[0x15] = 0x47;
        machine.memory.mem[0x4711] = 0x13 * i;

        machine.memory.set_mem_pos(0);
        machine.memory << SEC;
        machine.memory << SBC_IND_X;
        machine.memory << 0x10;
        machine.memory << BRK;

        run(machine);
        int ok = 0x12 - 0x13*i;
        if (ok < 0) ok = 256 - abs(ok);
        ASSERT_EQ(machine.cpu->A, ok);
    }
}

// Decimal mode
TEST_F(MOS6502Test, OpSBC_IND_X_DEC)
{
    Machine& machine = oric->get_machine();

    for (int a=0; a <= 99; a++)
    {
        for (int b=0; b <= 99; b++)
        {
            machine.cpu->D = true;
            machine.cpu->A = decToBCD(a);
            machine.cpu->X = 0x04;
            machine.memory.mem[0x14] = 0x11;
            machine.memory.mem[0x15] = 0x47;
            machine.memory.mem[0x4711] = decToBCD(b);

            machine.memory.set_mem_pos(0);
            machine.memory << SEC;
            machine.memory << SBC_IND_X;
            machine.memory << 0x10;
            machine.memory << BRK;

            run(machine);
            int ok = a - b;
            if (ok < 0) ok = 100 - abs(ok);
            ASSERT_EQ(bcdToDec(machine.cpu->A), ok);
        }
    }
}

// Normal mode
TEST_F(MOS6502Test, OpSBC_IND_Y)
{
    Machine& machine = oric->get_machine();

    for (int i=0; i < 12; i++)
    {
        machine.cpu->D = false;
        machine.cpu->A = 0x12;
        machine.cpu->Y = 0x11;
        machine.memory.mem[0x10] = 0x00;
        machine.memory.mem[0x11] = 0x47;
        machine.memory.mem[0x4711] = 0x13 * i;

        machine.memory.set_mem_pos(0);
        machine.memory << SEC;
        machine.memory << SBC_IND_Y;
        machine.memory << 0x10;
        machine.memory << BRK;

        run(machine);
        int ok = 0x12 - 0x13*i;
        if (ok < 0) ok = 256 - abs(ok);
        ASSERT_EQ(machine.cpu->A, ok);
    }
}

// Decimal mode
TEST_F(MOS6502Test, OpSBC_IND_Y_DEC)
{
    Machine& machine = oric->get_machine();

    for (int a=0; a <= 99; a++)
    {
        for (int b=0; b <= 99; b++)
        {
            machine.cpu->D = true;
            machine.cpu->A = decToBCD(a);
            machine.cpu->Y = 0x11;
            machine.memory.mem[0x10] = 0x00;
            machine.memory.mem[0x11] = 0x47;
            machine.memory.mem[0x4711] = decToBCD(b);

            machine.memory.set_mem_pos(0);
            machine.memory << SEC;
            machine.memory << SBC_IND_Y;
            machine.memory << 0x10;
            machine.memory << BRK;

            run(machine);
            int ok = a - b;
            if (ok < 0) ok = 100 - abs(ok);
            ASSERT_EQ(bcdToDec(machine.cpu->A), ok);
        }
    }
}

} // Unittest