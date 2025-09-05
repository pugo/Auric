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


class MOS6522Test : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        Config config;

        oric = new Oric(config);
        oric->init_machine();
        oric->get_machine().init_mos6522();

        mos6522 = oric->get_machine().mos_6522.get();
        mos6522->orb_changed_handler = test_via_orb_changed_callback;
        mos6522->ca2_changed_handler = test_ca2_changed_callback;
        mos6522->cb2_changed_handler = test_cb2_changed_callback;
        mos6522->psg_changed_handler = test_psg_changed_callback;
        mos6522->irq_handler = test_irq_callback;
        mos6522->irq_clear_handler = test_irq_clear_callback;
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

    static void test_via_orb_changed_callback(Machine& a_Machine, uint8_t orb)
    {}

    static void test_ca2_changed_callback(Machine& a_Machine, bool ca2)
    {}

    static void test_cb2_changed_callback(Machine& a_Machine, bool cb2)
    {}

    static void test_psg_changed_callback(Machine& a_Machine)
    {}

    static void test_irq_callback(Machine& a_Machine)
    {}

    static void test_irq_clear_callback(Machine& a_Machine)
    {}

    Oric* oric;
    MOS6522* mos6522;
};


} // Unittest