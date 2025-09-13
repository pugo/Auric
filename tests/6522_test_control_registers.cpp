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

class MOS6522TestCR : public MOS6522Test
{};

// --- CA1 ---

TEST_F(MOS6522TestCR, CA1_input_positive_transition_no_interrupt_on_negative)
{
    mos6522->write_ca1(false);
    mos6522->write_byte(MOS6522::IER, 0xff);    // Enable interrupts for bit 0-6.
    mos6522->write_byte(MOS6522::PCR, 0x01);    // Set CA1 to positive transition.
    mos6522->write_ca1(false);

    ASSERT_EQ(mos6522->read_byte(MOS6522::IFR), 0);
}

TEST_F(MOS6522TestCR, CA1_input_positive_transition)
{
    mos6522->write_ca1(false);
    mos6522->write_byte(MOS6522::IER, 0xff);    // Enable interrupts for bit 0-6.
    mos6522->write_byte(MOS6522::PCR, 0x01);    // Set CA1 to positive transition.
    mos6522->write_ca1(true);

    ASSERT_EQ(mos6522->read_byte(MOS6522::IFR), MOS6522::IRQ_CA1 | 0x80);		// Should have CA1 + IRQ (0x80).
}

TEST_F(MOS6522TestCR, CA1InterruptOnPositiveEdge)
{
    // Set CA1 for positive edge interrupt (PCR bit 0 = 1)
    mos6522->write_byte(MOS6522::PCR, 0x01);

    // Enable CA1 interrupt
    mos6522->write_byte(MOS6522::IER, 0x80 | MOS6522::IRQ_CA1);

    // CA1 low initially
    mos6522->write_ca1(false);
    mos6522->exec();
    EXPECT_EQ(mos6522->get_state().ifr & MOS6522::IRQ_CA1, 0);

    // Rising edge should trigger interrupt
    mos6522->write_ca1(true);
    mos6522->exec();
    EXPECT_NE(mos6522->get_state().ifr & MOS6522::IRQ_CA1, 0);

    // Clear interrupt by reading ORA
    mos6522->read_byte(MOS6522::ORA);
    EXPECT_EQ(mos6522->get_state().ifr & MOS6522::IRQ_CA1, 0);
}

TEST_F(MOS6522TestCR, CA1_input_negative_transition_no_interrupt_on_positive)
{
    mos6522->write_ca1(true);
    mos6522->write_byte(MOS6522::IER, 0xff);    // Enable interrupts for bit 0-6.
    mos6522->write_byte(MOS6522::PCR, 0x00);    // Set CA1 to negative transition.
    mos6522->write_ca1(true);

    ASSERT_EQ(mos6522->read_byte(MOS6522::IFR), 0);
}

TEST_F(MOS6522TestCR, CA1_input_negative_transition)
{
    mos6522->write_ca1(true);
    mos6522->write_byte(MOS6522::IER, 0xff);    // Enable interrupts for bit 0-6.
    mos6522->write_byte(MOS6522::PCR, 0x00);    // Set CA1 to negative transition.
    mos6522->write_ca1(false);

    ASSERT_EQ(mos6522->read_byte(MOS6522::IFR), MOS6522::IRQ_CA1 | 0x80);		// Should have CA1 + IRQ (0x80).
}

TEST_F(MOS6522TestCR, CA1InterruptOnNegativeEdge)
{
    // Set CA1 for negative edge interrupt (PCR bit 0 = 0)
    mos6522->write_byte(MOS6522::PCR, 0x00);

    // Enable CA1 interrupt
    mos6522->write_byte(MOS6522::IER, 0x80 | MOS6522::IRQ_CA1);

    // CA1 high initially
    mos6522->write_ca1(true);
    mos6522->exec();
    EXPECT_EQ(mos6522->get_state().ifr & MOS6522::IRQ_CA1, 0);

    // Falling edge should trigger interrupt
    mos6522->write_ca1(false);
    mos6522->exec();
    EXPECT_NE(mos6522->get_state().ifr & MOS6522::IRQ_CA1, 0);
}

// --- CA2 ---

TEST_F(MOS6522TestCR, CA2_input_mode_positive_transition_no_interrupt_on_negative)
{
    mos6522->write_ca2(false);
    mos6522->write_byte(MOS6522::IER, 0xff);    // Enable interrupts for bit 0-6.
    mos6522->write_byte(MOS6522::PCR, 0x04);    // Set CA2 to positive transition.
    mos6522->write_ca2(false);

    ASSERT_EQ(mos6522->read_byte(MOS6522::IFR), 0);
}

TEST_F(MOS6522TestCR, CA2_input_mode_positive_transition)
{
    mos6522->write_ca2(false);
    mos6522->write_byte(MOS6522::IER, 0xff);    // Enable interrupts for bit 0-6.
    mos6522->write_byte(MOS6522::PCR, 0x04);    // Set CA2 to positive transition.
    mos6522->write_ca2(true);

    ASSERT_EQ(mos6522->read_byte(MOS6522::IFR), MOS6522::IRQ_CA2 | 0x80);		// Should have CA2 + IRQ (0x80).
}

TEST_F(MOS6522TestCR, CA2_input_mode_negative_transition_no_interrupt_on_positive)
{
    mos6522->write_ca2(true);
    mos6522->write_byte(MOS6522::IER, 0xff);    // Enable interrupts for bit 0-6.
    mos6522->write_byte(MOS6522::PCR, 0x00);    // Set CA2 to negative transition.
    mos6522->write_ca2(true);

    ASSERT_EQ(mos6522->read_byte(MOS6522::IFR), 0);
}

TEST_F(MOS6522TestCR, CA2_input_mode_negative_transition)
{
    mos6522->write_ca2(true);
    mos6522->write_byte(MOS6522::IER, 0xff);    // Enable interrupts for bit 0-6.
    mos6522->write_byte(MOS6522::PCR, 0x00);    // Set CA2 to negative transition.
    mos6522->write_ca2(false);

    ASSERT_EQ(mos6522->read_byte(MOS6522::IFR), MOS6522::IRQ_CA2 | 0x80);		// Should have CA2 + IRQ (0x80).
}

TEST_F(MOS6522TestCR, CA2_input_mode_clear_interrupt_on_ORA_read)
{
    mos6522->write_ca2(true);
    mos6522->write_byte(MOS6522::IER, 0xff);    // Enable interrupts for bit 0-6.
    mos6522->write_byte(MOS6522::PCR, 0x00);    // Set CA2 to negative transition.
    mos6522->write_ca2(false);

    ASSERT_EQ(mos6522->read_byte(MOS6522::IFR), MOS6522::IRQ_CA2 | 0x80);		// Should have CA2 + IRQ (0x80).

    // Reading the ORA should reset the interrupt in input mode.
    mos6522->read_byte(MOS6522::ORA);
    ASSERT_EQ(mos6522->read_byte(MOS6522::IFR), 0);
}

TEST_F(MOS6522TestCR, CA2_independent_mode_dont_clear_interrupt_on_ORA_read)
{
    mos6522->write_ca2(true);
    mos6522->write_byte(MOS6522::IER, 0xff);    // Enable interrupts for bit 0-6.
    mos6522->write_byte(MOS6522::PCR, 0x02);    // Set CA2 to negative transition, independent mode.
    mos6522->write_ca2(false);

    ASSERT_EQ(mos6522->read_byte(MOS6522::IFR), MOS6522::IRQ_CA2 | 0x80);		// Should have CA2 + IRQ (0x80).

    // Reading the ORA should *not* reset the interrupt in independent mode.
    mos6522->read_byte(MOS6522::ORA);
    ASSERT_EQ(mos6522->read_byte(MOS6522::IFR), MOS6522::IRQ_CA2 | 0x80);		// Should have CA2 + IRQ (0x80).
}

TEST_F(MOS6522TestCR, CA2_handshake_mode)
{
    mos6522->write_ca1(false);
    mos6522->write_ca2(true);
    mos6522->write_byte(MOS6522::IER, 0xff);    // Enable interrupts for bit 0-6.
    mos6522->write_byte(MOS6522::PCR, 0x08);    // Enable CA2 pulse output mode.

    ASSERT_EQ(mos6522->get_state().ca2, true);

    mos6522->read_byte(MOS6522::ORA);           // Should trigger CA2 to go low.
    ASSERT_EQ(mos6522->get_state().ca2, false);

    mos6522->exec();                            // A cycle exec should not return CA2 to high.
    ASSERT_EQ(mos6522->get_state().ca2, false);

    mos6522->write_ca1(true);                   // But a write to CA1 should reset it.
    ASSERT_EQ(mos6522->get_state().ca2, true);

    mos6522->read_byte(MOS6522::ORA);           // Should trigger CA2 to go low.
    ASSERT_EQ(mos6522->get_state().ca2, false);
}

// In pulse output mode CA2 goes low one cycle after read or write of ORB.
TEST_F(MOS6522TestCR, CA2_pulse_output_mode)
{
    mos6522->write_ca2(true);
    mos6522->write_byte(MOS6522::IER, 0xff);    // Enable interrupts for bit 0-6.
    mos6522->write_byte(MOS6522::PCR, 0x0a);    // Enable CA2 pulse output mode.

    ASSERT_EQ(mos6522->get_state().ca2, true);

    mos6522->read_byte(MOS6522::ORA);           // Should trigger CA2 to go low 1 cyle.
    ASSERT_EQ(mos6522->get_state().ca2, false);

    mos6522->exec();                            // Should make CA2 high again.
    ASSERT_EQ(mos6522->get_state().ca2, true);
}

TEST_F(MOS6522TestCR, CA2OutputModeManual)
{
    // Set CA2 to manual output low (PCR bits 3-1 = 110)
    mos6522->write_byte(MOS6522::PCR, 0x0C);

    EXPECT_FALSE(mos6522->get_state().ca2);

    // Set CA2 to manual output high (PCR bits 3-1 = 111)
    mos6522->write_byte(MOS6522::PCR, 0x0E);

    EXPECT_TRUE(mos6522->get_state().ca2);
}

TEST_F(MOS6522TestCR, CA2OutputModePulse)
{
    // Set CA2 to pulse output (PCR bits 3-1 = 101)
    mos6522->write_byte(MOS6522::PCR, 0x0A);

    // CA2 should be high initially
    EXPECT_TRUE(mos6522->get_state().ca2);

    // Reading/writing ORA should cause pulse
    mos6522->write_byte(MOS6522::ORA, 0x55);

    // CA2 should go low for one cycle
    EXPECT_TRUE(mos6522->get_state().ca2_do_pulse);

    // After exec, CA2 should return high
    mos6522->exec();
    EXPECT_TRUE(mos6522->get_state().ca2);
    EXPECT_FALSE(mos6522->get_state().ca2_do_pulse);
}


// --- CB1 ---

TEST_F(MOS6522TestCR, CB1_input_positive_transition_no_interrupt_on_negative)
{
    mos6522->write_cb1(false);
    mos6522->write_byte(MOS6522::IER, 0xff);    // Enable interrupts for bit 0-6.
    mos6522->write_byte(MOS6522::PCR, 0x10);    // Set CB1 to positive transition.
    mos6522->write_cb1(false);

    ASSERT_EQ(mos6522->read_byte(MOS6522::IFR), 0);
}

TEST_F(MOS6522TestCR, CB1_input_positive_transition)
{
    mos6522->write_cb1(false);
    mos6522->write_byte(MOS6522::IER, 0xff);    // Enable interrupts for bit 0-6.
    mos6522->write_byte(MOS6522::PCR, 0x10);    // Set CB1 to positive transition.
    mos6522->write_cb1(true);

    ASSERT_EQ(mos6522->read_byte(MOS6522::IFR), MOS6522::IRQ_CB1 | 0x80);		// Should have CB1 + IRQ (0x80).
}

TEST_F(MOS6522TestCR, CB1_input_negative_transition_no_interrupt_on_positive)
{
    mos6522->write_cb1(true);
    mos6522->write_byte(MOS6522::IER, 0xff);    // Enable interrupts for bit 0-6.
    mos6522->write_byte(MOS6522::PCR, 0x00);    // Set CB1 to negative transition.
    mos6522->write_cb1(true);

    ASSERT_EQ(mos6522->read_byte(MOS6522::IFR), 0);
}

TEST_F(MOS6522TestCR, CB1_input_negative_transition)
{
    mos6522->write_cb1(true);
    mos6522->write_byte(MOS6522::IER, 0xff);    // Enable interrupts for bit 0-6.
    mos6522->write_byte(MOS6522::PCR, 0x00);    // Set CB1 to negative transition.
    mos6522->write_cb1(false);

    ASSERT_EQ(mos6522->read_byte(MOS6522::IFR), MOS6522::IRQ_CB1 | 0x80);		// Should have CB1 + IRQ (0x80).
}


TEST_F(MOS6522TestCR, CB1InterruptTapeInput)
{
    // Set CB1 for positive edge interrupt (for tape input)
    mos6522->write_byte(MOS6522::PCR, 0x10);

    // Enable CB1 interrupt
    mos6522->write_byte(MOS6522::IER, 0x80 | MOS6522::IRQ_CB1);

    // Simulate tape signal rising edge
    mos6522->write_cb1(false);
    mos6522->exec();
    mos6522->write_cb1(true);
    mos6522->exec();

    EXPECT_NE(mos6522->get_state().ifr & MOS6522::IRQ_CB1, 0);

    // Clear interrupt by reading ORB
    mos6522->read_byte(MOS6522::ORB);
    EXPECT_EQ(mos6522->get_state().ifr & MOS6522::IRQ_CB1, 0);
}

// --- CB2 ---

TEST_F(MOS6522TestCR, CB2_input_mode_positive_transition_no_interrupt_on_negative)
{
    mos6522->write_cb2(false);
    mos6522->write_byte(MOS6522::IER, 0xff);    // Enable interrupts for bit 0-6.
    mos6522->write_byte(MOS6522::PCR, 0x40);    // Set CB2 to positive transition.
    mos6522->write_cb2(false);

    ASSERT_EQ(mos6522->read_byte(MOS6522::IFR), 0);
}

TEST_F(MOS6522TestCR, CB2_input_mode_positive_transition)
{
    mos6522->write_cb2(false);
    mos6522->write_byte(MOS6522::IER, 0xff);    // Enable interrupts for bit 0-6.
    mos6522->write_byte(MOS6522::PCR, 0x40);    // Set CB2 to positive transition.
    mos6522->write_cb2(true);

    ASSERT_EQ(mos6522->read_byte(MOS6522::IFR), MOS6522::IRQ_CB2 | 0x80);		// Should have CA2 + IRQ (0x80).
}

TEST_F(MOS6522TestCR, CB2_input_mode_negative_transition_no_interrupt_on_positive)
{
    mos6522->write_cb2(true);
    mos6522->write_byte(MOS6522::IFR, 0x00);
    mos6522->write_byte(MOS6522::IER, 0xff);    // Enable interrupts for bit 0-6.
    mos6522->write_byte(MOS6522::PCR, 0x00);    // Set CB2 to negative transition.
    mos6522->write_cb2(true);

    ASSERT_EQ(mos6522->read_byte(MOS6522::IFR), 0);
}

TEST_F(MOS6522TestCR, CB2_input_mode_negative_transition)
{
    mos6522->write_cb2(true);
    mos6522->write_byte(MOS6522::IER, 0xff);    // Enable interrupts for bit 0-6.
    mos6522->write_byte(MOS6522::PCR, 0x00);    // Set CB2 to negative transition.
    mos6522->write_cb2(false);

    ASSERT_EQ(mos6522->read_byte(MOS6522::IFR), MOS6522::IRQ_CB2 | 0x80);		// Should have CA2 + IRQ (0x80).
}

TEST_F(MOS6522TestCR, CB2_input_mode_clear_interrupt_on_ORA_read)
{
    mos6522->write_cb2(true);
    mos6522->write_byte(MOS6522::IER, 0xff);    // Enable interrupts for bit 0-6.
    mos6522->write_byte(MOS6522::PCR, 0x00);    // Set CB2 to negative transition.
    mos6522->write_cb2(false);

    ASSERT_EQ(mos6522->read_byte(MOS6522::IFR), MOS6522::IRQ_CB2 | 0x80);		// Should have CB2 + IRQ (0x80).

    // Reading the ORB should reset the interrupt in input mode.
    mos6522->read_byte(MOS6522::ORB);
    ASSERT_EQ(mos6522->read_byte(MOS6522::IFR), 0);
}

TEST_F(MOS6522TestCR, CB2_independent_mode_dont_clear_interrupt_on_ORA_read)
{
    mos6522->write_cb2(true);
    mos6522->write_byte(MOS6522::IER, 0xff);    // Enable interrupts for bit 0-6.
    mos6522->write_byte(MOS6522::PCR, 0x20);    // Set CB2 to negative transition, independent mode.
    mos6522->write_cb2(false);

    ASSERT_EQ(mos6522->read_byte(MOS6522::IFR), MOS6522::IRQ_CB2 | 0x80);		// Should have CB2 + IRQ (0x80).

    // Reading the ORB should *not* reset the interrupt in independent mode.
    mos6522->read_byte(MOS6522::ORB);
    ASSERT_EQ(mos6522->read_byte(MOS6522::IFR), MOS6522::IRQ_CB2 | 0x80);		// Should have CB2 + IRQ (0x80).
}

TEST_F(MOS6522TestCR, CB2_handshake_mode)
{
    mos6522->write_cb1(false);
    mos6522->write_cb2(true);
    mos6522->write_byte(MOS6522::IER, 0xff);    // Enable interrupts for bit 0-6.
    mos6522->write_byte(MOS6522::PCR, 0x80);    // Enable CB2 pulse output mode.

    ASSERT_EQ(mos6522->get_state().cb2, true);

    mos6522->read_byte(MOS6522::ORB);           // Should trigger CB2 to go low.
    ASSERT_EQ(mos6522->get_state().cb2, false);

    mos6522->exec();                            // A cycle exec should not return CB2 to high.
    ASSERT_EQ(mos6522->get_state().cb2, false);

    mos6522->write_cb1(true);                   // But a write to CB1 should reset it.
    ASSERT_EQ(mos6522->get_state().cb2, true);

    mos6522->read_byte(MOS6522::ORB);           // Should trigger CB2 to go low.
    ASSERT_EQ(mos6522->get_state().cb2, false);
}

// In pulse output mode CB2 goes low one cycle after read or write of ORB.
TEST_F(MOS6522TestCR, CB2_pulse_output_mode)
{
    mos6522->write_cb2(true);
    mos6522->write_byte(MOS6522::IER, 0xff);    // Enable interrupts for bit 0-6.
    mos6522->write_byte(MOS6522::PCR, 0xa0);    // Enable CB2 pulse output mode.

    ASSERT_EQ(mos6522->get_state().cb2, true);

    mos6522->read_byte(MOS6522::ORB);           // Should trigger CB2 to go low 1 cyle.
    ASSERT_EQ(mos6522->get_state().cb2, false);

    mos6522->exec();                            // Should make CB2 high again.
    ASSERT_EQ(mos6522->get_state().cb2, true);
}

TEST_F(MOS6522TestCR, CB2PSGControl)
{
    mos6522->write_byte(MOS6522::PCR, 0xC0);    // Set CB2 to manual output for PSG BDIR control

    mos6522->exec();
    EXPECT_FALSE(mos6522->get_state().cb2);

    mos6522->write_byte(MOS6522::PCR, 0xE0);    // Set CB2 high for PSG write operation

    mos6522->exec();
    EXPECT_TRUE(mos6522->get_state().cb2);
}

TEST_F(MOS6522TestCR, WriteIER)
{
    ASSERT_EQ(mos6522->read_byte(MOS6522::IER), 0x80);		// Initial IER

    for (uint8_t i = 0; i < 7; i++)
    {
        mos6522->write_byte(MOS6522::IER, 0x7f);
        ASSERT_EQ(mos6522->read_byte(MOS6522::IER), 0x80);

        uint8_t b = 1 << i;
        mos6522->write_byte(MOS6522::IER, 0x80 | b);
        ASSERT_EQ(mos6522->read_byte(MOS6522::IER), 0x80 | b);
        mos6522->write_byte(MOS6522::IER, b);
        ASSERT_EQ(mos6522->read_byte(MOS6522::IER), 0x80);
    }
}

TEST_F(MOS6522TestCR, WriteIORA2)
{
    mos6522->write_byte(MOS6522::DDRA, 0xff);
    mos6522->write_byte(MOS6522::IORA2, 0xff);
    ASSERT_EQ(mos6522->read_byte(MOS6522::IORA2), 0xff);
}


} // Unittest