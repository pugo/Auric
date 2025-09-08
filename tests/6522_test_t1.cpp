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

class MOS6522TestTimerT1 : public MOS6522Test
{};


// ------ T1 counter ----------

TEST_F(MOS6522TestTimerT1, T1_tick_down)
{
    mos6522->write_byte(MOS6522::T1C_L, 0x11);
    mos6522->write_byte(MOS6522::T1C_H, 0x47);
    ASSERT_EQ(mos6522->read_byte(MOS6522::T1C_L), 0x11);
    ASSERT_EQ(mos6522->read_byte(MOS6522::T1C_H), 0x47);

    mos6522->exec();

    // Load takes one cycle, before ticking down counter. Expect original value.
    ASSERT_EQ(mos6522->read_byte(MOS6522::T1C_L), 0x11);
    ASSERT_EQ(mos6522->read_byte(MOS6522::T1C_H), 0x47);

    mos6522->exec();

    ASSERT_EQ(mos6522->read_byte(MOS6522::T1C_L), 0x10);
    ASSERT_EQ(mos6522->read_byte(MOS6522::T1C_H), 0x47);

    mos6522->exec();

    ASSERT_EQ(mos6522->read_byte(MOS6522::T1C_L), 0x0f);
    ASSERT_EQ(mos6522->read_byte(MOS6522::T1C_H), 0x47);
}

TEST_F(MOS6522TestTimerT1, T1_tick_down_low_high_boundary)
{
    mos6522->write_byte(MOS6522::T1C_L, 0x01);
    mos6522->write_byte(MOS6522::T1C_H, 0x47);

    mos6522->exec();  // Initial load
    mos6522->exec();

    ASSERT_EQ(mos6522->read_byte(MOS6522::T1C_L), 0x00);
    ASSERT_EQ(mos6522->read_byte(MOS6522::T1C_H), 0x47);

    mos6522->exec();

    ASSERT_EQ(mos6522->read_byte(MOS6522::T1C_L), 0xff);
    ASSERT_EQ(mos6522->read_byte(MOS6522::T1C_H), 0x46);
}

TEST_F(MOS6522TestTimerT1, T1_tick_down_reload_and_interrupt)
{
    mos6522->write_byte(MOS6522::IFR, 0x00);
    mos6522->write_byte(MOS6522::IER, 0xff);    // Enable interrupts for bit 0-6.

    mos6522->write_byte(MOS6522::ACR, 0x40);

    mos6522->write_byte(MOS6522::T1C_L, 0x05);
    mos6522->write_byte(MOS6522::T1C_H, 0x00);

    mos6522->exec();  // Initial load

    for (int i = 4; i > 0; i--) {
        mos6522->exec();
        ASSERT_EQ(mos6522->get_t1_counter(), (uint8_t)i);
    }

    mos6522->exec();
    ASSERT_EQ(mos6522->get_t1_counter(), 0x00);

    // Counting down to 0xffff, reload happens a cycle later.
    mos6522->exec();
    ASSERT_EQ(mos6522->get_t1_counter(), 0xffff);

    // Expect interrupt
    ASSERT_EQ(mos6522->read_byte(MOS6522::IFR), MOS6522::IRQ_T1 | 0x80);

    // Expect reload
    mos6522->exec();
    ASSERT_EQ(mos6522->get_t1_counter(), 0x0005);
}


TEST_F(MOS6522TestTimerT1, T1_interrupt_clear_on_read)
{
    mos6522->write_byte(MOS6522::IFR, 0xc0);
    mos6522->write_byte(MOS6522::IER, 0xff);    // Enable interrupts for bit 0-6.

    mos6522->write_byte(MOS6522::ACR, 0x40);

    mos6522->write_byte(MOS6522::T1C_L, 0x01);
    mos6522->write_byte(MOS6522::T1C_H, 0x00);

    mos6522->exec();  // Initial load
    mos6522->exec();
    mos6522->exec();
    mos6522->exec();

    // Expect interrupt on counter reaching 0.
    ASSERT_EQ(mos6522->read_byte(MOS6522::IFR), MOS6522::IRQ_T1 | 0x80);

    // Reading T1C_L should reset interrupts.
    mos6522->read_byte(MOS6522::T1C_L);
    ASSERT_EQ(mos6522->read_byte(MOS6522::IFR), 0);
}


TEST_F(MOS6522TestTimerT1, T1OneShotMode)
{
    // Set T1 to one-shot mode (ACR bits 7-6 = 00)
    mos6522->write_byte(MOS6522::ACR, 0x00);

    // Enable T1 interrupt
    mos6522->write_byte(MOS6522::IER, 0x80 | MOS6522::IRQ_T1);

    // Load T1 with value 3 for clearer testing
    mos6522->write_byte(MOS6522::T1L_L, 0x03);  // Low latch
    mos6522->write_byte(MOS6522::T1C_H, 0x00);  // High counter, starts timer

    // Verify timer is loaded
    EXPECT_TRUE(mos6522->get_state().t1_run);
    EXPECT_EQ(mos6522->get_t1_counter(), 3);

    // Cycle 1: Load cycle (the "+1" in N+1.5)
    mos6522->exec();
    EXPECT_EQ(mos6522->get_t1_counter(), 3); // Still at loaded value during load cycle
    EXPECT_EQ(mos6522->get_state().ifr & MOS6522::IRQ_T1, 0);

    // Cycle 2: Counter = 2 (start of N countdown cycles)
    mos6522->exec();
    EXPECT_EQ(mos6522->get_t1_counter(), 2);
    EXPECT_EQ(mos6522->get_state().ifr & MOS6522::IRQ_T1, 0);

    // Cycle 3: Counter = 1
    mos6522->exec();
    EXPECT_EQ(mos6522->get_t1_counter(), 1);
    EXPECT_EQ(mos6522->get_state().ifr & MOS6522::IRQ_T1, 0);

    // Cycle 4: Counter = 0 (N countdown cycles completed)
    mos6522->exec();
    EXPECT_EQ(mos6522->get_t1_counter(), 0);
    EXPECT_EQ(mos6522->get_state().ifr & MOS6522::IRQ_T1, 0); // Still no interrupt

    // Cycle 5: The "+0.5" part - interrupt flag gets set
    mos6522->exec();
    EXPECT_NE(mos6522->get_state().ifr & MOS6522::IRQ_T1, 0); // Interrupt now set!
    EXPECT_FALSE(mos6522->get_state().t1_run); // Timer stopped

    // Clear interrupt by reading T1C_L
    mos6522->read_byte(MOS6522::T1C_L);
    EXPECT_EQ(mos6522->get_state().ifr & MOS6522::IRQ_T1, 0);
}



TEST_F(MOS6522TestTimerT1, T1ContinuousMode)
{
    // Set T1 to continuous mode (ACR bits 7-6 = 01)
    mos6522->write_byte(MOS6522::ACR, 0x40);

    // Enable T1 interrupt
    mos6522->write_byte(MOS6522::IER, 0x80 | MOS6522::IRQ_T1);

    // Load T1 with value 2 for faster testing
    mos6522->write_byte(MOS6522::T1L_L, 0x02);
    mos6522->write_byte(MOS6522::T1C_H, 0x00);

    EXPECT_TRUE(mos6522->get_state().t1_run);
    EXPECT_EQ(mos6522->get_t1_counter(), 2);

    // First cycle: Load cycle
    mos6522->exec();
    EXPECT_EQ(mos6522->get_t1_counter(), 2);
    EXPECT_EQ(mos6522->get_state().ifr & MOS6522::IRQ_T1, 0);

    // Cycles 2-3: Countdown N=2
    mos6522->exec(); // Counter = 1
    EXPECT_EQ(mos6522->get_t1_counter(), 1);
    mos6522->exec(); // Counter = 0
    EXPECT_EQ(mos6522->get_t1_counter(), 0);

    // Cycle 4: +0.5 - interrupt set, continuous mode reloads
    mos6522->exec();
    EXPECT_NE(mos6522->get_state().ifr & MOS6522::IRQ_T1, 0);
    EXPECT_TRUE(mos6522->get_state().t1_run); // Continuous mode continues

    // Clear interrupt
    mos6522->read_byte(MOS6522::T1C_L);
    EXPECT_EQ(mos6522->get_state().ifr & MOS6522::IRQ_T1, 0);

    // Cycle 5: Should reload and start new countdown
    mos6522->exec();
    EXPECT_EQ(mos6522->get_t1_counter(), 2); // Reloaded from latch

    // Run another complete cycle to verify continuous operation
    mos6522->exec(); // Counter = 1
    mos6522->exec(); // Counter = 0
    mos6522->exec(); // Interrupt set again

    EXPECT_NE(mos6522->get_state().ifr & MOS6522::IRQ_T1, 0);
}


TEST_F(MOS6522TestTimerT1, T1OneShotWithPB7Output)
{
    // Set T1 to one-shot with PB7 output (ACR bits 7-6 = 10)
    mos6522->write_byte(MOS6522::ACR, 0x80);

    // Set PB7 as output
    mos6522->write_byte(MOS6522::DDRB, 0x80);

    // Load timer with value 2
    mos6522->write_byte(MOS6522::T1L_L, 0x02);
    mos6522->write_byte(MOS6522::T1C_H, 0x00);

    // Initial state - PB7 should be low during countdown
    uint8_t initial_pb7 = mos6522->read_orb() & 0x80;

    // Load cycle
    mos6522->exec();
    EXPECT_EQ(mos6522->read_orb() & 0x80, initial_pb7);

    // Countdown cycles
    mos6522->exec(); // Counter = 1
    EXPECT_EQ(mos6522->read_orb() & 0x80, initial_pb7);
    mos6522->exec(); // Counter = 0
    EXPECT_EQ(mos6522->read_orb() & 0x80, initial_pb7);

    // Interrupt cycle - PB7 should change state
    mos6522->exec();
    uint8_t final_pb7 = mos6522->read_orb() & 0x80;
    EXPECT_NE(final_pb7, initial_pb7);
    EXPECT_NE(final_pb7, 0); // Should be high
}


TEST_F(MOS6522TestTimerT1, T1ContinuousWithPB7SquareWave)
{
    // Set T1 to continuous with PB7 square wave (ACR bits 7-6 = 11)
    mos6522->write_byte(MOS6522::ACR, 0xC0);

    // Set PB7 as output
    mos6522->write_byte(MOS6522::DDRB, 0x80);

    // Load timer with value 1 for fast square wave
    mos6522->write_byte(MOS6522::T1L_L, 0x01);
    mos6522->write_byte(MOS6522::T1C_H, 0x00);

    uint8_t initial_pb7 = mos6522->read_orb() & 0x80;

    // First complete cycle: Load + N(1) + 0.5
    mos6522->exec(); // Load cycle
    mos6522->exec(); // Counter = 0
    mos6522->exec(); // Interrupt, PB7 toggles

    uint8_t toggled_pb7 = mos6522->read_orb() & 0x80;
    EXPECT_NE(toggled_pb7, initial_pb7);

    // Second complete cycle: Reload + N(1) + 0.5
    mos6522->exec(); // Reload cycle, Counter = 1
    mos6522->exec(); // Counter = 0
    mos6522->exec(); // Interrupt, PB7 toggles back

    EXPECT_EQ(mos6522->read_orb() & 0x80, initial_pb7); // Back to original state
}


} // Unittest