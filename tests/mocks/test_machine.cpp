// =========================================================================
//   Copyright (C) 2009-2025 by Anders Piniesjö <pugo@pugo.org>
// =========================================================================

#include "test_machine.h"
#include "../6522_test.hpp"

TestMachine::TestMachine()
    : memory()
{
    // Grundläggande initialisering för testmaskinen
}

//
//// Grundläggande tester kan läggas här också om du vill
//TEST(MOS6522BasicTest, Construction)
//{
//TestMachine machine;
//MOS6522 via(machine);
//
//// Testa att VIA:n startar i ett känt tillstånd
//EXPECT_EQ(via.get_state().ora, 0x00);
//EXPECT_EQ(via.get_state().orb, 0x00);
//EXPECT_EQ(via.get_state().ddra, 0x00);
//EXPECT_EQ(via.get_state().ddrb, 0x00);
//EXPECT_FALSE(via.get_state().t1_run);
//EXPECT_FALSE(via.get_state().t2_run);
//}
//
//TEST(MOS6522BasicTest, RegisterReadWrite)
//{
//TestMachine machine;
//MOS6522 via(machine);
//
//// Testa grundläggande registerläsning/skrivning
//via.write_byte(MOS6522::DDRA, 0xFF);
//EXPECT_EQ(via.read_byte(MOS6522::DDRA), 0xFF);
//
//via.write_byte(MOS6522::DDRB, 0xAA);
//EXPECT_EQ(via.read_byte(MOS6522::DDRB), 0xAA);
//
//via.write_byte(MOS6522::ORA, 0x55);
//EXPECT_EQ(via.read_byte(MOS6522::ORA), 0x55);
//}