# Windows build notes

This project was built on Windows with CMake + vcpkg using the following toolchain.

## Build tools used

| Tool | Version / Value | Notes |
|---|---|---|
| Operating System | Windows 10 | Build/run environment |
| CMake | 4.3.1 | `cmake --version` |
| vcpkg | 2026-03-04 (`4b3e4c276b5b87a649e66341e11553e8c577459c`) | `vcpkg version` |
| Visual Studio C++ Build Tools | 17.14.28 | VS 2022 toolset (MSVC) |
| CMake generator | `Visual Studio 17 2022` | From `build-vs2022/CMakeCache.txt` |
| vcpkg toolchain file | `C:/Users/dagfi/vcpkg/scripts/buildsystems/vcpkg.cmake` | From `build-vs2022/CMakeCache.txt` |
| vcpkg triplet | `x64-windows` | From `build-vs2022/CMakeCache.txt` |

Environment settings used:

- `VCPKG_ROOT=C:\...\vcpkg`
- User `PATH` includes `C:\...\vcpkg` and CMake `bin` directory.

Build output:

- Executable: `build-vs2022\src\Release\auric.exe`

## How the executable was run

Working directory and command matter because the app uses relative paths (`auric.yaml`, `ROMS`, `images`, `fonts`).

**From repo root (recommended)**
   - Directory: `C:\...\Auric`
   - Command: `.\build-vs2022\src\Release\auric.exe`

## Problems solved to get a Windows build

1. **Preset mismatch on Windows**
   - `CMakePresets.json` uses `Unix Makefiles`, which required `make` and failed in this Windows setup.
   - Solution: configured and built with the Visual Studio 2022 generator and vcpkg toolchain instead.

2. **Toolset compatibility issue**
   - Dependency build (`yaml-cpp` via vcpkg) failed under older VS 2019 toolset.
   - Solution: installed VS 2022 Build Tools and rebuilt with that toolset.

3. **Windows portability fixes in source**
   - `src\tape\tape_tap.hpp`
     - Changed member type from `std::string` to `std::filesystem::path`.
   - `src\memory.cpp`
     - Removed POSIX-only headers (`fcntl.h`, `unistd.h`).
     - Replaced `open(..., O_RDONLY)` check with `std::ifstream` open check.
   - `src\machine.cpp`
     - Removed POSIX-only include `unistd.h`.
   - `src\config.cpp`
     - Changed `YAML::LoadFile(config_path)` to `YAML::LoadFile(config_path.string())`.
   - `src\main.cpp`
     - Added `_WIN32` guard: use `std::signal(SIGINT, ...)` on Windows.
     - Kept `sigaction` path for non-Windows platforms.

4. **Runtime asset issue (not build-system failure)**
   - App initially failed at runtime because required ROM files were missing from `.\ROMS`.
   - After ROMs were added, the emulator launched and ran.
   - Command ZAP worked in emulator!
