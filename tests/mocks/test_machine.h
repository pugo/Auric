
// =========================================================================
//   Copyright (C) 2009-2025 by Anders Piniesjö <pugo@pugo.org>
// =========================================================================

#ifndef TESTS_6522_TEST_HPP
#define TESTS_6522_TEST_HPP

#include <gtest/gtest.h>
#include <memory>
#include <cstdint>

// Forward declarations
class Memory;
class Snapshot;

// Simple test machine mock for unit tests
class TestMachine
{
public:
    TestMachine();
    ~TestMachine() = default;

    // Mock memory for basic testing
    struct TestMemory {
        uint8_t mem[65536];

        TestMemory() {
            std::fill(std::begin(mem), std::end(mem), 0);
        }

        uint8_t& operator[](uint16_t addr) {
            return mem[addr];
        }

        const uint8_t& operator[](uint16_t addr) const {
            return mem[addr];
        }
    };

    TestMemory memory;
};

#include "chip/mos6522.hpp"

#endif // TESTS_6522_TEST_HPP