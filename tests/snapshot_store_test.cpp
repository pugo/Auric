// =========================================================================
//   Copyright (C) 2009-2026 by Anders Piniesjö <pugo@pugo.org>
//
//   This program is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 2 of the License, or
//   (at your option) any later version.
// =========================================================================

#include <chrono>
#include <filesystem>
#include <fstream>

#include <gtest/gtest.h>

#include "../src/snapshot_store.hpp"

namespace
{

std::filesystem::path test_directory()
{
    return std::filesystem::temp_directory_path()
        / ("auric_snapshot_store_"
           + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
}

class SnapshotStoreTest : public testing::Test
{
protected:
    void TearDown() override
    {
        std::filesystem::remove_all(directory);
    }

    std::filesystem::path directory = test_directory();
};

}


TEST_F(SnapshotStoreTest, SavesAndLoadsGeneralSnapshot)
{
    Snapshot snapshot;
    snapshot.memory = {0x12, 0x34, 0x56};
    snapshot.cycle_count = 1234;
    snapshot.ula.pixels = {0xaa, 0xbb};

    SnapshotStore store(directory);
    std::string error;
    ASSERT_TRUE(store.save(snapshot, {}, error)) << error;

    const auto loaded = store.load({}, error);
    ASSERT_TRUE(loaded.has_value()) << error;
    EXPECT_EQ(loaded->memory, snapshot.memory);
    EXPECT_EQ(loaded->cycle_count, snapshot.cycle_count);
    EXPECT_EQ(loaded->ula.pixels, snapshot.ula.pixels);
    EXPECT_TRUE(std::filesystem::exists(directory / "general.snap"));
}


TEST_F(SnapshotStoreTest, SeparatesMediaContexts)
{
    SnapshotStore store(directory);
    Snapshot tape_snapshot;
    tape_snapshot.tape.kind = TapeSnapshotKind::TapNormal;
    tape_snapshot.tape.path = "games/Hunchback.tap";
    tape_snapshot.memory = {1};

    Snapshot other_tape_snapshot = tape_snapshot;
    other_tape_snapshot.tape.path = "games/ManicMiner.tap";
    other_tape_snapshot.memory = {2};

    std::string error;
    const auto first_context = SnapshotStore::context_from_snapshot(tape_snapshot);
    const auto second_context = SnapshotStore::context_from_snapshot(other_tape_snapshot);
    ASSERT_NE(first_context, second_context);
    ASSERT_TRUE(store.save(tape_snapshot, first_context, error)) << error;
    ASSERT_TRUE(store.save(other_tape_snapshot, second_context, error)) << error;

    const auto first_loaded = store.load(first_context, error);
    const auto second_loaded = store.load(second_context, error);
    ASSERT_TRUE(first_loaded.has_value()) << error;
    ASSERT_TRUE(second_loaded.has_value()) << error;
    EXPECT_EQ(first_loaded->memory, std::vector<uint8_t>({1}));
    EXPECT_EQ(second_loaded->memory, std::vector<uint8_t>({2}));
}


TEST_F(SnapshotStoreTest, DoesNotLoadAnotherMediaContext)
{
    Snapshot snapshot;
    snapshot.memory = {0x42};
    SnapshotContext saved_context;
    saved_context.disks[0] = "/games/hunchback.dsk";

    SnapshotStore store(directory);
    std::string error;
    ASSERT_TRUE(store.save(snapshot, saved_context, error)) << error;

    SnapshotContext wrong_context;
    wrong_context.disks[0] = "/games/other.dsk";
    EXPECT_FALSE(store.load(wrong_context, error).has_value());
    EXPECT_TRUE(error.empty());
}


TEST_F(SnapshotStoreTest, RejectsCorruptSnapshot)
{
    SnapshotStore store(directory);
    std::string error;
    ASSERT_TRUE(store.save(Snapshot{}, {}, error)) << error;

    {
        std::ofstream file(directory / "general.snap", std::ios::binary | std::ios::trunc);
        file << "not a snapshot";
    }

    EXPECT_FALSE(store.load({}, error).has_value());
    EXPECT_FALSE(error.empty());
}
