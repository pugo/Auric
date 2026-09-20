// =========================================================================
//   Copyright (C) 2009-2026 by Anders Piniesjö <pugo@pugo.org>
//
//   This program is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 2 of the License, or
//   (at your option) any later version.
// =========================================================================

#include <gtest/gtest.h>

#include "../src/memory.hpp"
#include "../src/snapshot.hpp"


TEST(SnapshotTest, RestoresMemoryWithoutInvalidatingRawPointer)
{
    Memory memory(16);
    Snapshot snapshot;

    memory.mem[3] = 0x42;
    memory.save_to_snapshot(snapshot);

    memory.mem[3] = 0x99;
    memory.load_from_snapshot(snapshot);

    EXPECT_EQ(memory.mem[3], 0x42);
    EXPECT_EQ(memory.get_memory_vector()[3], 0x42);
    EXPECT_EQ(memory.mem, memory.get_memory_vector().data());
}
