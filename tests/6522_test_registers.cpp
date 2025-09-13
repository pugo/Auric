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

// === Port A =================================================================================

// Set and read data direction register A. Expect equal value.
TEST_F(MOS6522TestRegisters, WriteReadDDRA)
{
    mos6522->write_byte(MOS6522::DDRA, 0xff);
    ASSERT_EQ(mos6522->read_byte(MOS6522::DDRA), 0xff);
}

// Set data direction to input for all bits. Expect no result from ORA.
TEST_F(MOS6522TestRegisters, ReadORAAllInputs)
{
    mos6522->write_byte(MOS6522::DDRA, 0x00);
    mos6522->write_byte(MOS6522::ORA, 0xff);
    ASSERT_EQ(mos6522->read_byte(MOS6522::ORA), 0x00);
}

// Set data direction to output for all bits. Expect result from ORA.
TEST_F(MOS6522TestRegisters, ReadORAAllOutputs)
{
    mos6522->write_byte(MOS6522::DDRA, 0xff);
    mos6522->write_byte(MOS6522::ORA, 0xff);
    ASSERT_EQ(mos6522->read_byte(MOS6522::ORA), 0xff);
}

TEST_F(MOS6522TestRegisters, ReadORALatching)
{
    mos6522->write_byte(MOS6522::ACR, 0x01);   // enable PA latching
    mos6522->write_byte(MOS6522::DDRA, 0x00);  // all inputs

    // Default PCR=0 → CA1 active on falling edge.
    // Baseline CA1 high (no IRQ yet).
    mos6522->write_ca1(true);

    // Ensure pins are 0 before the edge so the latch will capture 0.
    mos6522->set_ira_bit(0, false);

    // Active falling edge → IFR(CA1)=1 and latch = 0.
    mos6522->write_ca1(false);

    // Now change the live pin to 1. The latch must still read 0 until CA1 is cleared.
    mos6522->set_ira_bit(0, true);

    // First ORA read uses the latch (0) and clears IFR(CA1):
    EXPECT_EQ(mos6522->read_byte(MOS6522::ORA), 0x00);

    // Second ORA read uses live pins (1):
    EXPECT_EQ(mos6522->read_byte(MOS6522::ORA), 0x01);
}

TEST_F(MOS6522TestRegisters, PortALatchingEnabled)
{
    mos6522->write_byte(MOS6522::ACR, 0x01);    // enable PA latching
    mos6522->write_byte(MOS6522::DDRA, 0x00);   // all inputs
    mos6522->write_byte(MOS6522::PCR, 0x01);    // CA1 rising active

    mos6522->write_ca1(false);

    // Put pins at 0x55
    for (int i = 0; i < 8; ++i) mos6522->set_ira_bit(i, !(i & 1));

    // Generate the *active* edge now -> latch should capture 0x55 here
    mos6522->write_ca1(true);

    ASSERT_EQ(mos6522->get_state().ira_latch, 0x55)           << "Latch didn't capture 0x55";

    // Change pins to 0xAA (should not affect the latched value)
    for (int i = 0; i < 8; ++i) mos6522->set_ira_bit(i, i & 1);

    // First ORA read returns latched 0x55, second returns live 0xAA
    EXPECT_EQ(mos6522->read_byte(MOS6522::ORA), 0x55);
    EXPECT_EQ(mos6522->read_byte(MOS6522::ORA), 0xAA);
}

TEST_F(MOS6522TestRegisters, PortAInputOutput)
{
    // Set PA 0-3 as outputs, PA 4-7 as inputs
    mos6522->write_byte(MOS6522::DDRA, 0x0F);

    // Write to ORA
    mos6522->write_byte(MOS6522::ORA, 0xFF);

    // Only lower 4 bits should be output
    EXPECT_EQ(mos6522->read_ora(), 0x0F);

    // Set input data on upper bits
    mos6522->set_ira_bit(4, true);
    mos6522->set_ira_bit(5, false);
    mos6522->set_ira_bit(6, true);
    mos6522->set_ira_bit(7, false);

    // Reading ORA should combine output and input data
    uint8_t combined = mos6522->read_byte(MOS6522::ORA);
    EXPECT_EQ(combined & 0x0F, 0x0F); // Output bits
    EXPECT_EQ((combined >> 4) & 0x0F, 0x05); // Input bits (0101)
}

// === Port B =================================================================================

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


TEST_F(MOS6522TestRegisters, PortBLatchingEnabled)
{
    // Enable PB latching (ACR bit 1 = 1)
    mos6522->write_byte(MOS6522::ACR, 0x02);

    // Set PB3 as input (keyboard sense)
    mos6522->write_byte(MOS6522::DDRB, 0xF7);

    mos6522->write_byte(MOS6522::PCR, 0x10); // set CB1 control: rising edge active

    // Set initial keyboard state
    mos6522->set_irb_bit(3, true);

    // Trigger CB1 to latch data
    mos6522->write_cb1(true);
    mos6522->exec();

    // Change keyboard state
    mos6522->set_irb_bit(3, false);

    // Reading ORB should return latched state
    uint8_t latched = mos6522->read_byte(MOS6522::ORB);
    EXPECT_NE(latched & 0x08, 0); // Bit 3 should still be high (latched)
}

TEST_F(MOS6522TestRegisters, PortBInputOutput)
{
    // Set PB 0-2 as outputs (keyboard demux), PB 3 as input
    mos6522->write_byte(MOS6522::DDRB, 0x07);

    // Write keyboard row selection
    mos6522->write_byte(MOS6522::ORB, 0x03); // Select row 3

    // Only lower 3 bits should be output
    EXPECT_EQ(mos6522->read_orb(), 0x03);

    // Simulate keyboard input on PB3
    mos6522->set_irb_bit(3, true); // Key pressed

    // Reading ORB should show combined data
    uint8_t combined = mos6522->read_byte(MOS6522::ORB);
    EXPECT_EQ(combined & 0x07, 0x03); // Output bits
    EXPECT_EQ((combined >> 3) & 0x01, 0x01); // Input bit
}

// === Control lines ======================================================================

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