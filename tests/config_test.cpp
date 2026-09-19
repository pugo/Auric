// =========================================================================
//   Copyright (C) 2009-2026 by Anders Piniesjö <pugo@pugo.org>
//
//   This program is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
// =========================================================================

#include <array>
#include <string>

#include <gtest/gtest.h>

#include "../src/config.hpp"


namespace {

using namespace std::string_literals;

template <size_t N>
bool parse_args(Config& config, std::array<std::string, N>& args)
{
    std::array<char*, N> argv{};
    for (size_t i = 0; i < N; ++i) {
        argv[i] = args[i].data();
    }

    if (! config.parse_command_line(static_cast<int>(argv.size()), argv.data())) {
        return false;
    }

    config.apply_command_line();
    return true;
}

}


TEST(ConfigTest, TapeEnablesAutostartByDefault)
{
    Config config;
    std::array args{"auric"s, "--tape"s, "game.tap"s};

    ASSERT_TRUE(parse_args(config, args));

    EXPECT_EQ(config.tape_path(), "game.tap");
    EXPECT_TRUE(config.tape_autostart_enabled());
}


TEST(ConfigTest, TapeNoAutostartDisablesStartupAutostart)
{
    Config config;
    std::array args{"auric"s, "--tape"s, "game.tap"s, "--tape-autostart-off"s};

    ASSERT_TRUE(parse_args(config, args));

    EXPECT_FALSE(config.tape_autostart_enabled());
}


TEST(ConfigTest, MonitorDisablesTapeAutostart)
{
    Config config;
    std::array args{"auric"s, "--tape"s, "game.tap"s, "--monitor"s};

    ASSERT_TRUE(parse_args(config, args));

    EXPECT_FALSE(config.tape_autostart_enabled());
}


TEST(ConfigTest, DiskDisablesTapeAutostart)
{
    Config config;
    std::array args{"auric"s, "--tape"s, "game.tap"s, "--disk1"s, "boot.dsk"s};

    ASSERT_TRUE(parse_args(config, args));

    EXPECT_FALSE(config.tape_autostart_enabled());
}
