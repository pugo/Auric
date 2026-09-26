// =========================================================================
//   Copyright (C) 2009-2026 by Anders Piniesjö <pugo@pugo.org>
//
//   This program is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 2 of the License, or
//   (at your option) any later version.
// =========================================================================

#ifndef SNAPSHOT_STORE_H
#define SNAPSHOT_STORE_H

#include <array>
#include <filesystem>
#include <optional>
#include <string>

#include "snapshot.hpp"

struct SnapshotContext
{
    std::string tape;
    std::array<std::string, 4> disks{};

    bool operator==(const SnapshotContext&) const = default;
};


class SnapshotStore
{
public:
    explicit SnapshotStore(std::filesystem::path directory);

    static SnapshotContext context_from_snapshot(const Snapshot& snapshot);

    bool save(const Snapshot& snapshot, const SnapshotContext& context, std::string& error) const;
    std::optional<Snapshot> load(const SnapshotContext& context, std::string& error) const;

private:
    std::filesystem::path slot_path(const SnapshotContext& context) const;

    std::filesystem::path directory;
};

#endif // SNAPSHOT_STORE_H
