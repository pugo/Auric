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

#include "6522_test.hpp"


namespace Unittest {

using namespace testing;


class MOS6522TestRegisters : public MOS6522Test
{};

// === PA =================================================================================

// Set and read data direction register A. Expect equal value.
TEST_F(MOS6522TestRegisters, WriteReadDDRA)
{
    mos6522->write_byte(MOS6522::DDRA, 0xff);
    ASSERT_EQ(mos6522->read_byte(MOS6522::DDRA), 0xff);
}

// Set data direction to input for all bits. Expect no result from ORA.
TEST_F(MOS6522TestRegisters, ReadORAAllInputs)
{
    std::cout << "-- 1" << std::endl;
    mos6522->write_byte(MOS6522::DDRA, 0x00);
    std::cout << "-- 2" << std::endl;
    mos6522->write_byte(MOS6522::ORA, 0xff);
    std::cout << "-- 3" << std::endl;
    ASSERT_EQ(mos6522->read_byte(MOS6522::ORA), 0x00);
}

// Set data direction to output for all bits. Expect result from ORA.
TEST_F(MOS6522TestRegisters, ReadORAAllOutputs)
{
    mos6522->write_byte(MOS6522::DDRA, 0xff);
    mos6522->write_byte(MOS6522::ORA, 0xff);
    ASSERT_EQ(mos6522->read_byte(MOS6522::ORA), 0xff);
}

// Ensure latching of ORB is controlled by interrupt if enabled.
TEST_F(MOS6522TestRegisters, ReadORALatching)
{
    mos6522->write_byte(MOS6522::IER, 0xff);            // Enable interrupts for bit 0-6.
    mos6522->write_byte(MOS6522::DDRA, 0x00);
    mos6522->get_state().ora = 0x00;
    mos6522->get_state().ca1 = true;                    // Start with ca1 high as CA1 ctrl = 0 means high to low interrupt
    mos6522->write_byte(MOS6522::ACR, 0x01);            // Enable PA latching

    mos6522->set_ira_bit(0, true);                      // Set ira bit 1 high. This should not be latched until interrupt
    ASSERT_EQ(mos6522->read_byte(MOS6522::ORA), 0x00);  // Expect no change

    mos6522->write_ca1(false);                          // Interrupt by high -> low transition
    ASSERT_EQ(mos6522->read_byte(MOS6522::ORA), 0x01);  // Now expect value to be latched by interrupt.
}

// === PB =================================================================================

// Set and read data direction register B. Expect equal value.
TEST_F(MOS6522TestRegisters, WriteReadDDRB)
{
    mos6522->write_byte(MOS6522::DDRB, 0xff);
    ASSERT_EQ(mos6522->read_byte(MOS6522::DDRB), 0xff);
}

// Set data direction to input for all bits. Expect no result from ORB.
TEST_F(MOS6522TestRegisters, ReadORBAllInputs)
{
    mos6522->write_byte(MOS6522::DDRB, 0x00);
    mos6522->write_byte(MOS6522::ORB, 0xff);
    ASSERT_EQ(mos6522->read_byte(MOS6522::ORB), 0x00);
}

// Set data direction to output for all bits. Expect result from ORA.
TEST_F(MOS6522TestRegisters, ReadORBAllOutputs)
{
    mos6522->write_byte(MOS6522::DDRB, 0xff);
    mos6522->write_byte(MOS6522::ORB, 0xff);
    ASSERT_EQ(mos6522->read_byte(MOS6522::ORB), 0xff);
}

// Ensure latching of ORB is controlled by interrupt if enabled.
TEST_F(MOS6522TestRegisters, ReadORBLatching)
{
    mos6522->write_byte(MOS6522::IER, 0xff);            // Enable interrupts for bit 0-6.
    mos6522->write_byte(MOS6522::DDRB, 0x00);
    mos6522->get_state().orb = 0x00;
    mos6522->get_state().cb1 = true;                    // Start with cb1 high as CB1 ctrl = 0 means high to low interrupt
    mos6522->write_byte(MOS6522::ACR, 0x02);            // Enable PB latching

    mos6522->set_irb_bit(1, true);                      // Set irb bit 1 high. This should not be latched until interrupt
    ASSERT_EQ(mos6522->read_byte(MOS6522::ORB), 0x00);  // Expect no change

    mos6522->write_cb1(false);                          // Interrupt by high -> low transition
    ASSERT_EQ(mos6522->read_byte(MOS6522::ORB), 0x02);  // Now expect value to be latched by interrupt.
}

// === T1 =================================================================================

// Writing T1L_L should not transfer to any other T1 registers.
TEST_F(MOS6522TestRegisters, WriteT1L_L)
{
    mos6522->write_byte(MOS6522::T1L_L, 0xee);
    ASSERT_EQ(mos6522->read_byte(MOS6522::T1L_L), 0xee);
    ASSERT_EQ(mos6522->read_byte(MOS6522::T1L_H), 0x00);
    ASSERT_EQ(mos6522->read_byte(MOS6522::T1C_L), 0x00);
    ASSERT_EQ(mos6522->read_byte(MOS6522::T1C_H), 0x00);
}

// Writing T1L_H should not transfer to any other T1 registers.
TEST_F(MOS6522TestRegisters, WriteT1L_H)
{
    mos6522->write_byte(MOS6522::T1L_H, 0x44);
    ASSERT_EQ(mos6522->read_byte(MOS6522::T1L_L), 0x00);
    ASSERT_EQ(mos6522->read_byte(MOS6522::T1L_H), 0x44);
    ASSERT_EQ(mos6522->read_byte(MOS6522::T1C_L), 0x00);
    ASSERT_EQ(mos6522->read_byte(MOS6522::T1C_H), 0x00);
}

// Writing T1L_H should clear interrupt.
TEST_F(MOS6522TestRegisters, WriteT1L_H_interrupt_clear)
{
    mos6522->write_byte(MOS6522::IER, 0xff);                        // Enable interrupts for bit 0-6.
    mos6522->set_ifr(0x80 | MOS6522::IRQ_T1);

    ASSERT_EQ(mos6522->read_byte(MOS6522::IFR), MOS6522::IRQ_T1 | 0x80);

    mos6522->write_byte(MOS6522::T1L_H, 0x88);
    ASSERT_EQ(mos6522->read_byte(MOS6522::IFR), 0);
}

// Writing both T1L_L and T1L_H should not transfer to any other T1 registers.
TEST_F(MOS6522TestRegisters, WriteT1L_LH)
{
    mos6522->write_byte(MOS6522::T1L_L, 0x12);
    mos6522->write_byte(MOS6522::T1L_H, 0x34);
    ASSERT_EQ(mos6522->read_byte(MOS6522::T1L_L), 0x12);
    ASSERT_EQ(mos6522->read_byte(MOS6522::T1L_H), 0x34);
    ASSERT_EQ(mos6522->read_byte(MOS6522::T1C_L), 0x00);
    ASSERT_EQ(mos6522->read_byte(MOS6522::T1C_H), 0x00);
}

// Writing T1C_L should only write to T1L_L.
TEST_F(MOS6522TestRegisters, WriteT1C_L)
{
    mos6522->write_byte(MOS6522::T1C_L, 0x77);
    ASSERT_EQ(mos6522->read_byte(MOS6522::T1L_L), 0x77);
    ASSERT_EQ(mos6522->read_byte(MOS6522::T1L_H), 0x00);
    ASSERT_EQ(mos6522->read_byte(MOS6522::T1C_L), 0x00);
    ASSERT_EQ(mos6522->read_byte(MOS6522::T1C_H), 0x00);
}

// Reading T1C_L should clear interrupt.
TEST_F(MOS6522TestRegisters, ReadT1C_L_interrupt_clear)
{
    mos6522->write_byte(MOS6522::IER, 0xff);                        // Enable interrupts for bit 0-6.
    mos6522->set_ifr(0x80 | MOS6522::IRQ_T1);

    ASSERT_EQ(mos6522->read_byte(MOS6522::IFR), MOS6522::IRQ_T1 | 0x80);

    mos6522->read_byte(MOS6522::T1C_L);
    ASSERT_EQ(mos6522->read_byte(MOS6522::IFR), 0);
}

// Writing T1C_H should write to T1L_H and T1C_H and transfer T1L_L to T1C_L.
TEST_F(MOS6522TestRegisters, WriteT1C_H)
{
    mos6522->write_byte(MOS6522::IER, 0xff);                        // Enable interrupts for bit 0-6.
    mos6522->write_byte(MOS6522::IFR, 0x80 + MOS6522::IRQ_T1);      // Start with T1 interrupt set.

    mos6522->write_byte(MOS6522::T1C_L, 0x11);
    mos6522->write_byte(MOS6522::T1C_H, 0x88);

    ASSERT_EQ(mos6522->read_byte(MOS6522::T1L_L), 0x11);
    ASSERT_EQ(mos6522->read_byte(MOS6522::T1L_H), 0x88);
    ASSERT_EQ(mos6522->read_byte(MOS6522::T1C_L), 0x11);
    ASSERT_EQ(mos6522->read_byte(MOS6522::T1C_H), 0x88);
}

// Writing T1C_H should clear interrupt.
TEST_F(MOS6522TestRegisters, WriteT1C_H_interrupt_clear)
{
    mos6522->write_byte(MOS6522::IER, 0xff);                        // Enable interrupts for bit 0-6.
    mos6522->set_ifr(0x80 | MOS6522::IRQ_T1);

    ASSERT_EQ(mos6522->read_byte(MOS6522::IFR), MOS6522::IRQ_T1 | 0x80);

    mos6522->write_byte(MOS6522::T1C_H, 0x88);
    ASSERT_EQ(mos6522->read_byte(MOS6522::IFR), 0);
}

// === T2 =================================================================================

// Writing T2C_L only writes to T2L_L, not T2C_L until transfer.
TEST_F(MOS6522TestRegisters, WriteT2C_L)
{
    mos6522->write_byte(MOS6522::T2C_L, 0x99);
    ASSERT_EQ(mos6522->read_byte(MOS6522::T2C_L), 0x00);
    ASSERT_EQ(mos6522->read_byte(MOS6522::T2C_H), 0x00);
}

// Reading T2C_L should clear interrupt.
TEST_F(MOS6522TestRegisters, ReadT2C_L_interrupt_clear)
{
    mos6522->write_byte(MOS6522::IER, 0xff);                        // Enable interrupts for bit 0-6.
    mos6522->set_ifr(0x80 | MOS6522::IRQ_T2);

    ASSERT_EQ(mos6522->read_byte(MOS6522::IFR), MOS6522::IRQ_T2 | 0x80);

    mos6522->read_byte(MOS6522::T2C_L);
    ASSERT_EQ(mos6522->read_byte(MOS6522::IFR), 0);
}

// Writing T2C_H transfers T2L_L to T2C_L.
TEST_F(MOS6522TestRegisters, WriteT2C_H)
{
    mos6522->write_byte(MOS6522::T2C_L, 0xaa);
    mos6522->write_byte(MOS6522::T2C_H, 0xbb);
    ASSERT_EQ(mos6522->read_byte(MOS6522::T2C_L), 0xaa);
    ASSERT_EQ(mos6522->read_byte(MOS6522::T2C_H), 0xbb);
}

// Writing T2C_H should clear interrupt.
TEST_F(MOS6522TestRegisters, WriteT2C_H_interrupt_clear)
{
    mos6522->write_byte(MOS6522::IER, 0xff);                        // Enable interrupts for bit 0-6.
    mos6522->set_ifr(0x80 | MOS6522::IRQ_T2);

    ASSERT_EQ(mos6522->read_byte(MOS6522::IFR), MOS6522::IRQ_T2 | 0x80);

    mos6522->write_byte(MOS6522::T2C_H, 0x88);
    ASSERT_EQ(mos6522->read_byte(MOS6522::IFR), 0);
}

// === Other ============================================================================

TEST_F(MOS6522TestRegisters, WriteSR)
{
    mos6522->write_byte(MOS6522::SR, 0xaa);
    ASSERT_EQ(mos6522->read_byte(MOS6522::SR), 0xaa);
}

TEST_F(MOS6522TestRegisters, WriteACR)
{
    mos6522->write_byte(MOS6522::ACR, 0xbb);
    ASSERT_EQ(mos6522->read_byte(MOS6522::ACR), 0xbb);
}

TEST_F(MOS6522TestRegisters, WritePCR)
{
    mos6522->write_byte(MOS6522::PCR, 0xcc);
    ASSERT_EQ(mos6522->read_byte(MOS6522::PCR), 0xcc);
}

} // Unittest