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

class MOS6522TestCounters : public MOS6522Test
{};


// ------ T1 counter ----------

TEST_F(MOS6522TestCounters, T1_tick_down)
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

TEST_F(MOS6522TestCounters, T1_tick_down_low_high_boundary)
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

TEST_F(MOS6522TestCounters, T1_tick_down_reload_and_interrupt)
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

TEST_F(MOS6522TestCounters, T1_interrupt_clear_on_read)
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


// ------ T2 counter ----------

TEST_F(MOS6522TestCounters, T2_tick_down)
{
    mos6522->write_byte(MOS6522::T2C_L, 0x11);
    mos6522->write_byte(MOS6522::T2C_H, 0x47);
    ASSERT_EQ(mos6522->read_byte(MOS6522::T2C_L), 0x11);
    ASSERT_EQ(mos6522->read_byte(MOS6522::T2C_H), 0x47);

    mos6522->exec();

    // Load takes one cycle, before ticking down counter. Expect original value.
    ASSERT_EQ(mos6522->read_byte(MOS6522::T2C_L), 0x11);
    ASSERT_EQ(mos6522->read_byte(MOS6522::T2C_H), 0x47);

    mos6522->exec();

    ASSERT_EQ(mos6522->read_byte(MOS6522::T2C_L), 0x10);
    ASSERT_EQ(mos6522->read_byte(MOS6522::T2C_H), 0x47);

    mos6522->exec();

    ASSERT_EQ(mos6522->read_byte(MOS6522::T2C_L), 0x0f);
    ASSERT_EQ(mos6522->read_byte(MOS6522::T2C_H), 0x47);
}

TEST_F(MOS6522TestCounters, T2_tick_down_low_high_boundary)
{
    mos6522->write_byte(MOS6522::T2C_L, 0x01);
    mos6522->write_byte(MOS6522::T2C_H, 0x47);

    mos6522->exec();  // Initial load
    mos6522->exec();

    ASSERT_EQ(mos6522->read_byte(MOS6522::T2C_L), 0x00);
    ASSERT_EQ(mos6522->read_byte(MOS6522::T2C_H), 0x47);

    mos6522->exec();

    ASSERT_EQ(mos6522->read_byte(MOS6522::T2C_L), 0xff);
    ASSERT_EQ(mos6522->read_byte(MOS6522::T2C_H), 0x46);
}

TEST_F(MOS6522TestCounters, T2_tick_down_and_interrupt)
{
    mos6522->write_byte(MOS6522::IFR, 0x00);
    mos6522->write_byte(MOS6522::IER, 0xff);    // Enable interrupts for bit 0-6.

    mos6522->write_byte(MOS6522::T2C_L, 0x05);
    mos6522->write_byte(MOS6522::T2C_H, 0x00);

    mos6522->exec();  // Initial load

    for (int i = 4; i > 0; i--) {
        mos6522->exec();
        ASSERT_EQ(mos6522->get_t2_counter(), (uint8_t)i);
    }

    mos6522->exec();
    ASSERT_EQ(mos6522->get_t2_counter(), 0x00);

    mos6522->exec();
    ASSERT_EQ(mos6522->get_t2_counter(), 0xffff);

    // Expect interrupt
    ASSERT_EQ(mos6522->read_byte(MOS6522::IFR), MOS6522::IRQ_T2 | 0x80);

    mos6522->exec();
    ASSERT_EQ(mos6522->get_t2_counter(), 0xfffe);
}

TEST_F(MOS6522TestCounters, T2_interrupt_clear_on_read)
{
    mos6522->write_byte(MOS6522::IFR, 0x00);
    mos6522->write_byte(MOS6522::IER, 0xff);    // Enable interrupts for bit 0-6.

    mos6522->write_byte(MOS6522::T2C_L, 0x01);
    mos6522->write_byte(MOS6522::T2C_H, 0x00);

    mos6522->exec();  // Initial load
    mos6522->exec();
    mos6522->exec();
    mos6522->exec();

    // Expect interrupt on counter reaching 0.
    ASSERT_EQ(mos6522->read_byte(MOS6522::IFR), MOS6522::IRQ_T2 | 0x80);

    // Reading T2C_L should reset interrupts.
    mos6522->read_byte(MOS6522::T2C_L);
    ASSERT_EQ(mos6522->read_byte(MOS6522::IFR), 0);
}

TEST_F(MOS6522TestCounters, T2_pulse_counting)
{
    mos6522->write_byte(MOS6522::ACR, 0x20);    // Enable T2 pulse counting mode.
    mos6522->write_byte(MOS6522::IFR, 0x00);
    mos6522->write_byte(MOS6522::IER, 0xff);    // Enable interrupts for bit 0-6.

    mos6522->write_byte(MOS6522::T2C_L, 0x05);
    mos6522->write_byte(MOS6522::T2C_H, 0x00);

    mos6522->exec();  // Initial load

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

} // Unittest