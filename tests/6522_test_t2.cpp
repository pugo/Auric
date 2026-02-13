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

class MOS6522TestTimerT2 : public MOS6522Test
{};


// ------ T2 counter ----------

TEST_F(MOS6522TestTimerT2, T2_tick_down)
{
    mos6522->write_byte(MOS6522::T2C_L, 0x11);
    mos6522->write_byte(MOS6522::T2C_H, 0x47);
    ASSERT_EQ(mos6522->read_byte(MOS6522::T2C_L), 0x11);
    ASSERT_EQ(mos6522->read_byte(MOS6522::T2C_H), 0x47);

    mos6522->exec(1);

    // Load takes one cycle, before ticking down counter. Expect original value.
    ASSERT_EQ(mos6522->read_byte(MOS6522::T2C_L), 0x11);
    ASSERT_EQ(mos6522->read_byte(MOS6522::T2C_H), 0x47);

    mos6522->exec(1);

    ASSERT_EQ(mos6522->read_byte(MOS6522::T2C_L), 0x10);
    ASSERT_EQ(mos6522->read_byte(MOS6522::T2C_H), 0x47);

    mos6522->exec(1);

    ASSERT_EQ(mos6522->read_byte(MOS6522::T2C_L), 0x0f);
    ASSERT_EQ(mos6522->read_byte(MOS6522::T2C_H), 0x47);
}

TEST_F(MOS6522TestTimerT2, T2_tick_down_low_high_boundary)
{
    mos6522->write_byte(MOS6522::T2C_L, 0x01);
    mos6522->write_byte(MOS6522::T2C_H, 0x47);

    mos6522->exec(1);  // Initial load
    mos6522->exec(1);

    ASSERT_EQ(mos6522->read_byte(MOS6522::T2C_L), 0x00);
    ASSERT_EQ(mos6522->read_byte(MOS6522::T2C_H), 0x47);

    mos6522->exec(1);

    ASSERT_EQ(mos6522->read_byte(MOS6522::T2C_L), 0xff);
    ASSERT_EQ(mos6522->read_byte(MOS6522::T2C_H), 0x46);
}

TEST_F(MOS6522TestTimerT2, T2_tick_down_and_interrupt)
{
    mos6522->write_byte(MOS6522::IFR, 0x00);
    mos6522->write_byte(MOS6522::IER, 0xff);    // Enable interrupts for bit 0-6.

    mos6522->write_byte(MOS6522::T2C_L, 0x05);
    mos6522->write_byte(MOS6522::T2C_H, 0x00);

    mos6522->exec(1);  // Initial load

    for (int i = 4; i > 0; i--) {
        mos6522->exec(1);
        ASSERT_EQ(mos6522->get_t2_counter(), (uint8_t)i);
    }

    mos6522->exec(1);
    ASSERT_EQ(mos6522->get_t2_counter(), 0x00);

    mos6522->exec(1);
    ASSERT_EQ(mos6522->get_t2_counter(), 0xffff);

    // Expect interrupt
    ASSERT_EQ(mos6522->read_byte(MOS6522::IFR), MOS6522::IRQ_T2 | 0x80);

    mos6522->exec(1);
    ASSERT_EQ(mos6522->get_t2_counter(), 0xfffe);
}

TEST_F(MOS6522TestTimerT2, T2_interrupt_clear_on_read)
{
    mos6522->write_byte(MOS6522::IFR, 0x00);
    mos6522->write_byte(MOS6522::IER, 0xff);    // Enable interrupts for bit 0-6.

    mos6522->write_byte(MOS6522::T2C_L, 0x01);
    mos6522->write_byte(MOS6522::T2C_H, 0x00);

    mos6522->exec(1);  // Initial load
    mos6522->exec(1);
    mos6522->exec(1);
    mos6522->exec(1);

    // Expect interrupt on counter reaching 0.
    ASSERT_EQ(mos6522->read_byte(MOS6522::IFR), MOS6522::IRQ_T2 | 0x80);

    // Reading T2C_L should reset interrupts.
    mos6522->read_byte(MOS6522::T2C_L);
    ASSERT_EQ(mos6522->read_byte(MOS6522::IFR), 0);
}

TEST_F(MOS6522TestTimerT2, T2_pulse_counting)
{
    mos6522->write_byte(MOS6522::ACR, 0x20);    // Enable T2 pulse counting mode.
    mos6522->write_byte(MOS6522::IFR, 0x00);
    mos6522->write_byte(MOS6522::IER, 0xff);    // Enable interrupts for bit 0-6.

    mos6522->write_byte(MOS6522::T2C_L, 0x05);
    mos6522->write_byte(MOS6522::T2C_H, 0x00);

    mos6522->exec(1);  // Initial load

    ASSERT_EQ(mos6522->get_t2_counter(), 0x05);

    for (int i = 4; i > 0; i--) {
        mos6522->set_irb_bit(6, true);
        mos6522->set_irb_bit(6, false);
        ASSERT_EQ(mos6522->get_t2_counter(), (uint8_t)i);
    }

    mos6522->set_irb_bit(6, true);
    mos6522->set_irb_bit(6, false);
    ASSERT_EQ(mos6522->get_t2_counter(), 0);

    // Expect interrupt
    ASSERT_EQ(mos6522->read_byte(MOS6522::IFR), MOS6522::IRQ_T2 | 0x80);
}

/**
 * Test T1 Timer modes according to ACR bits 7-6
 * 00: Timed interrupt each time T1 is loaded (one-shot mode)
 * 01: Continuous interrupts
 * 10: Timed interrupt each time T1 is loaded, one shot output on PB7
 * 11: Continuous interrupts, square wave output on PB7
 */

TEST_F(MOS6522TestTimerT2, T2OneShotMode)
{
    // Set T2 to timed interrupt mode (ACR bit 5 = 0)
    mos6522->write_byte(MOS6522::ACR, 0x00);

    // Enable T2 interrupt
    mos6522->write_byte(MOS6522::IER, 0x80 | MOS6522::IRQ_T2);

    // Load T2 with value 2
    mos6522->write_byte(MOS6522::T2C_L, 0x02);
    mos6522->write_byte(MOS6522::T2C_H, 0x00);

    EXPECT_TRUE(mos6522->get_state().t2_run);
    EXPECT_EQ(mos6522->get_t2_counter(), 2);

    // Load cycle
    mos6522->exec(1);
    EXPECT_EQ(mos6522->get_t2_counter(), 2);
    EXPECT_EQ(mos6522->get_state().ifr & MOS6522::IRQ_T2, 0);

    // Countdown cycles
    mos6522->exec(1); // Counter = 1
    EXPECT_EQ(mos6522->get_t2_counter(), 1);
    mos6522->exec(1); // Counter = 0
    EXPECT_EQ(mos6522->get_t2_counter(), 0);

    // Interrupt cycle
    mos6522->exec(1);
    EXPECT_NE(mos6522->get_state().ifr & MOS6522::IRQ_T2, 0);
    EXPECT_FALSE(mos6522->get_state().t2_run); // One-shot stops

    // Clear interrupt
    mos6522->read_byte(MOS6522::T2C_L);
    EXPECT_EQ(mos6522->get_state().ifr & MOS6522::IRQ_T2, 0);
}

TEST_F(MOS6522TestTimerT2, T2PulseCountingMode)
{
    // Set T2 to pulse counting mode (ACR bit 5 = 1)
    mos6522->write_byte(MOS6522::ACR, 0x20);

    // Set PB6 as input
    mos6522->write_byte(MOS6522::DDRB, 0x00);

    // Load T2 with value 3
    mos6522->write_byte(MOS6522::T2C_L, 0x03);
    mos6522->write_byte(MOS6522::T2C_H, 0x00);

    // Load cycle
    mos6522->exec(1);
    EXPECT_EQ(mos6522->get_t2_counter(), 3);

    // In pulse counting mode, timer doesn't decrement with clock cycles
    mos6522->exec(1);
    EXPECT_EQ(mos6522->get_t2_counter(), 3);

    // Simulate negative edges on PB6 to decrement counter
    mos6522->set_irb_bit(6, true);   // PB6 high
    mos6522->exec(1);
    mos6522->set_irb_bit(6, false);  // PB6 low (negative edge)
    mos6522->exec(1);

    EXPECT_EQ(mos6522->get_t2_counter(), 2); // Should decrement on negative edge

    // Continue pulsing until timer expires
    for (int pulses = 0; pulses < 2; pulses++) {
        mos6522->set_irb_bit(6, true);
        mos6522->exec(1);
        mos6522->set_irb_bit(6, false);
        mos6522->exec(1);
    }

    EXPECT_EQ(mos6522->get_t2_counter(), 0);

    // One more pulse to trigger interrupt
    mos6522->set_irb_bit(6, true);
    mos6522->exec(1);
    mos6522->set_irb_bit(6, false);
    mos6522->exec(1);

    EXPECT_NE(mos6522->get_state().ifr & MOS6522::IRQ_T2, 0);
}

} // Unittest