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

class MOS6522TestSR : public MOS6522Test
{};

// ==== Shift in ================

// ---- In by T2 ---------------

TEST_F(MOS6522TestSR, Shift_in_by_t2)
{
    mos6522->write_byte(MOS6522::ACR, 0x04);    // Shift in under control of T2
    mos6522->write_byte(MOS6522::IER, 0xff);    // Enable interrupts for bit 0-6.
    mos6522->write_byte(MOS6522::T2C_L, 0x03);
    mos6522->write_byte(MOS6522::SR, 0x00);

    // Shift in a 1.
    mos6522->get_state().cb2 = true;

    // Count down to 0. First time it is positive transition. Expect shift.
    for (uint8_t i = 0; i < 4; i++) {
        mos6522->exec();
    }

    ASSERT_EQ(mos6522->get_state().sr, 0x01);

    // Count down again, now it is negative transition. No shift!
    for (uint8_t i = 0; i < 4; i++) {
        mos6522->exec();
    }

    ASSERT_EQ(mos6522->get_state().sr, 0x01);
}


TEST_F(MOS6522TestSR, Shift_in_by_t2_cb1_toggles)
{
    mos6522->write_byte(MOS6522::ACR, 0x04);    // Shift in under control of T2
    mos6522->write_byte(MOS6522::IER, 0xff);    // Enable interrupts for bit 0-6.
    mos6522->write_byte(MOS6522::T2C_L, 0x03);
    mos6522->write_byte(MOS6522::SR, 0x00);
    mos6522->get_state().cb1 = false;

    for (uint8_t i = 0; i < 4; i++) {
        mos6522->exec();
    }
    ASSERT_EQ(mos6522->get_state().cb1, true);

    for (uint8_t i = 0; i < 4; i++) {
        mos6522->exec();
    }
    ASSERT_EQ(mos6522->get_state().cb1, false);
}

TEST_F(MOS6522TestSR, Shift_in_by_t2_correct_value)
{
    mos6522->write_byte(MOS6522::ACR, 0x04);    // Shift in under control of T2
    mos6522->write_byte(MOS6522::IER, 0xff);    // Enable interrupts for bit 0-6.
    mos6522->write_byte(MOS6522::T2C_L, 0x03);
    mos6522->write_byte(MOS6522::SR, 0x00);

    // Shift in bits from this value.
    uint8_t value = 0x42;

    for (uint8_t b = 0; b < 8; b++) {
        mos6522->get_state().cb2 = value & 0x80;
        value <<= 1;

        for (uint8_t i = 0; i < 8; i++) {
            mos6522->exec();
        }
    }

    ASSERT_EQ(mos6522->get_state().sr, 0x42);
}

TEST_F(MOS6522TestSR, Shift_in_by_t2_stops_after_8_bits)
{
    mos6522->write_byte(MOS6522::ACR, 0x04);    // Shift in under control of T2
    mos6522->write_byte(MOS6522::IER, 0xff);    // Enable interrupts for bit 0-6.
    mos6522->write_byte(MOS6522::T2C_L, 0x03);
    mos6522->write_byte(MOS6522::SR, 0x00);

    uint8_t value = 0x42;

    // First shift in the value bit by bit.
    for (uint8_t b = 0; b < 8; b++) {
        mos6522->get_state().cb2 = value & 0x80;
        value <<= 1;

        for (uint8_t i = 0; i < 8; i++) {
            mos6522->exec();
        }
    }

    ASSERT_EQ(mos6522->get_state().sr, 0x42);

    // Try to shift in another bit. It should not work.
    mos6522->get_state().cb2 = false;
    for (uint8_t i = 0; i < 8; i++) {
        mos6522->exec();
    }

    ASSERT_EQ(mos6522->get_state().sr, 0x42);
}

TEST_F(MOS6522TestSR, Shift_in_by_t2_interrupt_when_done)
{
    mos6522->write_byte(MOS6522::ACR, 0x04);    // Shift in under control of T2
    mos6522->write_byte(MOS6522::IER, 0xff);    // Enable interrupts for bit 0-6.
    mos6522->write_byte(MOS6522::T2C_L, 0x03);
    mos6522->write_byte(MOS6522::SR, 0x00);

    // No interrupts
    ASSERT_EQ(mos6522->read_byte(MOS6522::IFR), 0);

    uint8_t value = 0x42;

    // First shift in the value bit by bit.
    for (uint8_t b = 0; b < 8; b++) {
        mos6522->get_state().cb2 = value & 0x80;
        value <<= 1;

        for (uint8_t i = 0; i < 8; i++) {
            mos6522->exec();
        }
    }

    // SR interrupt when done
    ASSERT_EQ(mos6522->read_byte(MOS6522::IFR), 0x80 + MOS6522::IRQ_SR);
}

// ---- In by O2 ---------------

TEST_F(MOS6522TestSR, Shift_in_by_o2)
{
    mos6522->write_byte(MOS6522::ACR, 0x08);    // Shift in under control of O2
    mos6522->write_byte(MOS6522::IER, 0xff);    // Enable interrupts for bit 0-6.
    mos6522->write_byte(MOS6522::SR, 0x00);
    mos6522->get_state().cb1 = false;

    // Shift in a 1.
    mos6522->get_state().cb2 = true;

    // First time it is positive transition. Expect shift.
    mos6522->exec();
    ASSERT_EQ(mos6522->get_state().sr, 0x01);

    // Now it is negative transition. No shift!
    mos6522->exec();

    ASSERT_EQ(mos6522->get_state().sr, 0x01);
}

TEST_F(MOS6522TestSR, Shift_in_by_o2_cb1_toggles)
{
    mos6522->write_byte(MOS6522::ACR, 0x08);    // Shift in under control of O2
    mos6522->write_byte(MOS6522::IER, 0xff);    // Enable interrupts for bit 0-6.
    mos6522->write_byte(MOS6522::SR, 0x00);
    mos6522->get_state().cb1 = false;

    mos6522->exec();
    ASSERT_EQ(mos6522->get_state().cb1, true);

    mos6522->exec();
    ASSERT_EQ(mos6522->get_state().cb1, false);

    mos6522->exec();
    ASSERT_EQ(mos6522->get_state().cb1, true);
}

TEST_F(MOS6522TestSR, Shift_in_by_o2_correct_value)
{
    mos6522->write_byte(MOS6522::ACR, 0x08);    // Shift in under control of O2
    mos6522->write_byte(MOS6522::IER, 0xff);    // Enable interrupts for bit 0-6.
    mos6522->write_byte(MOS6522::SR, 0x00);
    mos6522->get_state().cb1 = false;

    // Shift in bits from this value.
    uint8_t value = 0x42;

    for (uint8_t b = 0; b < 8; b++) {
        mos6522->get_state().cb2 = value & 0x80;
        value <<= 1;

        mos6522->exec();
        mos6522->exec();
    }

    ASSERT_EQ(mos6522->get_state().sr, 0x42);
}

TEST_F(MOS6522TestSR, Shift_in_by_o2_stops_after_8_bits)
{
    mos6522->write_byte(MOS6522::ACR, 0x08);    // Shift in under control of O2
    mos6522->write_byte(MOS6522::IER, 0xff);    // Enable interrupts for bit 0-6.
    mos6522->write_byte(MOS6522::SR, 0x00);
    mos6522->get_state().cb1 = false;

    uint8_t value = 0x42;

    // First shift in the value bit by bit.
    for (uint8_t b = 0; b < 8; b++) {
        mos6522->get_state().cb2 = value & 0x80;
        value <<= 1;

        mos6522->exec();
        mos6522->exec();
    }

    ASSERT_EQ(mos6522->get_state().sr, 0x42);

    // Try to shift in another bit. It should not work.
    mos6522->get_state().cb2 = false;
    mos6522->exec();
    mos6522->exec();

    ASSERT_EQ(mos6522->get_state().sr, 0x42);
}

TEST_F(MOS6522TestSR, Shift_in_by_o2_interrupt_when_done)
{
    mos6522->write_byte(MOS6522::ACR, 0x08);    // Shift in under control of O2
    mos6522->write_byte(MOS6522::IER, 0xff);    // Enable interrupts for bit 0-6.
    mos6522->write_byte(MOS6522::SR, 0x00);
    mos6522->get_state().cb1 = false;

    // No interrupts
    ASSERT_EQ(mos6522->read_byte(MOS6522::IFR), 0);

    uint8_t value = 0x42;

    // First shift in the value bit by bit.
    for (uint8_t b = 0; b < 8; b++) {
        mos6522->get_state().cb2 = value & 0x80;
        value <<= 1;

        mos6522->exec();
        mos6522->exec();
    }

    // SR interrupt when done
    ASSERT_EQ(mos6522->read_byte(MOS6522::IFR), 0x80 + MOS6522::IRQ_SR);
}


// ==== Shift out ===============

// ---- Out by T2 - freerunning ---------------

TEST_F(MOS6522TestSR, Shift_out_freerunning)
{
    mos6522->write_byte(MOS6522::ACR, 0x10);    // Shift out by T2 - freerunning
    mos6522->write_byte(MOS6522::IER, 0xff);    // Enable interrupts for bit 0-6.
    mos6522->write_byte(MOS6522::T2C_L, 0x03);
    mos6522->write_byte(MOS6522::SR, 0xaa);

    ASSERT_EQ(mos6522->get_state().cb2, false);

    // Count down to 0. But first time it is positive transition. No shift.
    for (uint8_t i = 0; i < 4; i++) {
        mos6522->exec();
        ASSERT_EQ(mos6522->get_state().cb2, false);
    }

    // Count down again, but now it is negative transition. Shift!
    for (uint8_t i = 0; i < 3; i++) {
        mos6522->exec();
        ASSERT_EQ(mos6522->get_state().cb2, false);
    }

    mos6522->exec();  // Initial load
    ASSERT_EQ(mos6522->get_state().cb2, true);
}

TEST_F(MOS6522TestSR, Shift_out_freerunning_cb1_toggles)
{
    mos6522->write_byte(MOS6522::ACR, 0x10);    // Shift out by T2 - freerunning
    mos6522->write_byte(MOS6522::IER, 0xff);    // Enable interrupts for bit 0-6.
    mos6522->write_byte(MOS6522::T2C_L, 0x03);
    mos6522->write_byte(MOS6522::SR, 0xaa);
    mos6522->get_state().cb1 = false;

    for (uint8_t i = 0; i < 4; i++) {
        mos6522->exec();
    }
    ASSERT_EQ(mos6522->get_state().cb1, true);

    for (uint8_t i = 0; i < 4; i++) {
        mos6522->exec();
    }
    ASSERT_EQ(mos6522->get_state().cb1, false);
}

TEST_F(MOS6522TestSR, Shift_out_freerunning_correct_value)
{
    mos6522->write_byte(MOS6522::ACR, 0x10);    // Shift out by T2 - freerunning
    mos6522->write_byte(MOS6522::IER, 0xff);    // Enable interrupts for bit 0-6.
    mos6522->write_byte(MOS6522::T2C_L, 0x03);
    mos6522->write_byte(MOS6522::SR, 0x42);

    uint8_t result = 0x00;

    for (uint8_t b = 0; b < 8; b++) {
        // Double amount of execs since we only shift on negative transition.
        for (uint8_t i = 0; i < 8; i++) {
            mos6522->exec();
        }

        result <<= 1;
        result |= (mos6522->get_state().cb2 ? 1 : 0);
    }

    ASSERT_EQ(result, 0x42);
}

TEST_F(MOS6522TestSR, Shift_out_freerunning_do_not_stop_after_8_bits)
{
    mos6522->write_byte(MOS6522::ACR, 0x10);    // Shift out by T2 - freerunning
    mos6522->write_byte(MOS6522::IER, 0xff);    // Enable interrupts for bit 0-6.
    mos6522->write_byte(MOS6522::T2C_L, 0x03);
    mos6522->write_byte(MOS6522::SR, 0xaa);

    uint8_t result = 0x00;

    // First shift out 8 bits.
    for (uint8_t b = 0; b < 8; b++) {
        // Double amount of execs since we only shift on negative transition.
        for (uint8_t i = 0; i < 8; i++) {
            mos6522->exec();
        }

        result <<= 1;
        result |= (mos6522->get_state().cb2 ? 1 : 0);
    }

    // Get the final cb2 bit.
    bool cb2 = mos6522->get_state().cb2;
    uint8_t diff_count = 0;

    // Make sure cb2 changes by continuing shifting.
    for (uint8_t b = 0; b < 8; b++) {
        for (uint8_t i = 0; i < 8; i++) {
            mos6522->exec();
        }

        // Shifting should find differences.
        diff_count += (cb2 == mos6522->get_state().cb2 ? 0 : 1);
    }

    ASSERT_GT(diff_count, 0);
}

// ---- Out by T2 ---------------

TEST_F(MOS6522TestSR, Shift_out_by_t2)
{
    mos6522->write_byte(MOS6522::ACR, 0x14);    // Shift out under control of T2
    mos6522->write_byte(MOS6522::IER, 0xff);    // Enable interrupts for bit 0-6.
    mos6522->write_byte(MOS6522::T2C_L, 0x03);
    mos6522->write_byte(MOS6522::SR, 0xaa);

    ASSERT_EQ(mos6522->get_state().cb2, false);

    // Count down to 0. But first time it is positive transition. No shift.
    for (uint8_t i = 0; i < 4; i++) {
        mos6522->exec();
        ASSERT_EQ(mos6522->get_state().cb2, false);
    }

    // Count down again, but now it is negative transition. Shift!
    for (uint8_t i = 0; i < 3; i++) {
        mos6522->exec();
        ASSERT_EQ(mos6522->get_state().cb2, false);
    }

    mos6522->exec();  // Initial load
    ASSERT_EQ(mos6522->get_state().cb2, true);
}

TEST_F(MOS6522TestSR, Shift_out_by_t2_cb1_toggles)
{
    mos6522->write_byte(MOS6522::ACR, 0x14);    // Shift out under control of T2
    mos6522->write_byte(MOS6522::IER, 0xff);    // Enable interrupts for bit 0-6.
    mos6522->write_byte(MOS6522::T2C_L, 0x03);
    mos6522->write_byte(MOS6522::SR, 0xaa);
    mos6522->get_state().cb1 = false;

    for (uint8_t i = 0; i < 4; i++) {
        mos6522->exec();
    }
    ASSERT_EQ(mos6522->get_state().cb1, true);

    for (uint8_t i = 0; i < 4; i++) {
        mos6522->exec();
    }
    ASSERT_EQ(mos6522->get_state().cb1, false);
}

TEST_F(MOS6522TestSR, Shift_out_by_t2_correct_value)
{
    mos6522->write_byte(MOS6522::ACR, 0x14);    // Shift out under control of T2
    mos6522->write_byte(MOS6522::IER, 0xff);    // Enable interrupts for bit 0-6.
    mos6522->write_byte(MOS6522::T2C_L, 0x03);
    mos6522->write_byte(MOS6522::SR, 0x42);

    uint8_t result = 0x00;

    for (uint8_t b = 0; b < 8; b++) {
        // Double amount of execs since we only shift on negative transition.
        for (uint8_t i = 0; i < 8; i++) {
            mos6522->exec();
        }

        result <<= 1;
        result |= (mos6522->get_state().cb2 ? 1 : 0);
    }

    ASSERT_EQ(result, 0x42);
}

TEST_F(MOS6522TestSR, Shift_out_by_t2_stops_after_8_bits)
{
    mos6522->write_byte(MOS6522::ACR, 0x14);    // Shift out under control of T2
    mos6522->write_byte(MOS6522::IER, 0xff);    // Enable interrupts for bit 0-6.
    mos6522->write_byte(MOS6522::T2C_L, 0x03);
    mos6522->write_byte(MOS6522::SR, 0x42);

    uint8_t result = 0x00;

    // First shift out 8 bits.
    for (uint8_t b = 0; b < 8; b++) {
        // Double amount of execs since we only shift on negative transition.
        for (uint8_t i = 0; i < 8; i++) {
            mos6522->exec();
        }

        result <<= 1;
        result |= (mos6522->get_state().cb2 ? 1 : 0);
    }

    // Get the final cb2 bit.
    bool cb2 = mos6522->get_state().cb2;

    // Ensure cb2 does not change from execs, i.e. shifting has stopped.
    for (uint8_t b = 0; b < 8; b++) {
        // Double amount of execs since we only shift on negative transition.
        for (uint8_t i = 0; i < 8; i++) {
            mos6522->exec();
        }

        ASSERT_EQ(cb2, mos6522->get_state().cb2);
    }
}

TEST_F(MOS6522TestSR, Shift_out_by_t2_interrupt_when_done)
{
    mos6522->write_byte(MOS6522::ACR, 0x14);    // Shift out under control of T2
    mos6522->write_byte(MOS6522::IER, 0xff);    // Enable interrupts for bit 0-6.
    mos6522->write_byte(MOS6522::T2C_L, 0x03);
    mos6522->write_byte(MOS6522::SR, 0x42);

    // No interrupts
    ASSERT_EQ(mos6522->read_byte(MOS6522::IFR), 0);

    // First shift out 8 bits.
    for (uint8_t b = 0; b < 8; b++) {
        // Double amount of execs since we only shift on negative transition.
        for (uint8_t i = 0; i < 8; i++) {
            mos6522->exec();
        }
    }

    // SR interrupt when done
    ASSERT_EQ(mos6522->read_byte(MOS6522::IFR), 0x80 + MOS6522::IRQ_SR);
}

// ---- Out by O2 ---------------

TEST_F(MOS6522TestSR, Shift_out_by_o2)
{
    mos6522->write_byte(MOS6522::ACR, 0x18);    // Shift out under control of O2
    mos6522->write_byte(MOS6522::IER, 0xff);    // Enable interrupts for bit 0-6.
    mos6522->write_byte(MOS6522::SR, 0xaa);
    mos6522->get_state().cb1 = false;


    mos6522->exec();
    ASSERT_EQ(mos6522->get_state().cb2, false); // Positive transition, no shift

    mos6522->exec();
    ASSERT_EQ(mos6522->get_state().cb2, true);  // Negative transition, shift
}

TEST_F(MOS6522TestSR, Shift_out_by_o2_cb1_toggles)
{
    mos6522->write_byte(MOS6522::ACR, 0x18);    // Shift out under control of O2
    mos6522->write_byte(MOS6522::IER, 0xff);    // Enable interrupts for bit 0-6.
    mos6522->write_byte(MOS6522::SR, 0xaa);
    mos6522->get_state().cb1 = false;

    mos6522->exec();
    ASSERT_EQ(mos6522->get_state().cb1, true);

    mos6522->exec();
    ASSERT_EQ(mos6522->get_state().cb1, false);

    mos6522->exec();
    ASSERT_EQ(mos6522->get_state().cb1, true);
}

TEST_F(MOS6522TestSR, Shift_out_by_o2_correct_value)
{
    mos6522->write_byte(MOS6522::ACR, 0x18);    // Shift out under control of O2
    mos6522->write_byte(MOS6522::IER, 0xff);    // Enable interrupts for bit 0-6.
    mos6522->write_byte(MOS6522::SR, 0x42);
    mos6522->get_state().cb1 = false;

    uint8_t result = 0x00;

    for (uint8_t b = 0; b < 8; b++) {
        mos6522->exec();
        mos6522->exec();

        result <<= 1;
        result |= (mos6522->get_state().cb2 ? 1 : 0);
    }

    ASSERT_EQ(result, 0x42);
}

TEST_F(MOS6522TestSR, Shift_out_by_o2_stops_after_8_bits)
{
    mos6522->write_byte(MOS6522::ACR, 0x18);    // Shift out under control of O2
    mos6522->write_byte(MOS6522::IER, 0xff);    // Enable interrupts for bit 0-6.
    mos6522->write_byte(MOS6522::SR, 0x42);
    mos6522->get_state().cb1 = false;

    uint8_t result = 0x00;

    // First shift out 8 bits.
    for (uint8_t b = 0; b < 8; b++) {
        mos6522->exec();
        mos6522->exec();

        result <<= 1;
        result |= (mos6522->get_state().cb2 ? 1 : 0);
    }

    // Get the final cb2 bit.
    bool cb2 = mos6522->get_state().cb2;

    // Ensure cb2 does not change from execs, i.e. shifting has stopped.
    for (uint8_t b = 0; b < 8; b++) {
        mos6522->exec();
        mos6522->exec();

        ASSERT_EQ(cb2, mos6522->get_state().cb2);
    }
}

TEST_F(MOS6522TestSR, Shift_out_by_o2_interrupt_when_done)
{
    mos6522->write_byte(MOS6522::ACR, 0x18);    // Shift out under control of O2
    mos6522->write_byte(MOS6522::IER, 0xff);    // Enable interrupts for bit 0-6.
    mos6522->write_byte(MOS6522::SR, 0x42);
    mos6522->get_state().cb1 = false;

    // No interrupts
    ASSERT_EQ(mos6522->read_byte(MOS6522::IFR), 0);

    // First shift out 8 bits.
    for (uint8_t b = 0; b < 8; b++) {
        mos6522->exec();
        mos6522->exec();
    }

    // SR interrupt when done
    ASSERT_EQ(mos6522->read_byte(MOS6522::IFR), 0x80 + MOS6522::IRQ_SR);
}


} // Unittest