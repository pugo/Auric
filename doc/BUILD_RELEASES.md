# Release packages

The GitHub Release workflow produces portable archives for the supported CI
targets:

| Archive | Platform |
|---|---|
| `auric-VERSION-linux-x64.tar.gz` | Linux x64 |
| `auric-VERSION-windows-x64.zip` | Windows x64 |
| `auric-VERSION-macos-arm64.zip` | macOS arm64 |

Extract an archive and run the `auric` executable directly from the extracted
package directory. The executable is placed in the package root alongside
`auric.yaml`, `fonts/`, `images/`, and `ROMS/`. ROM files are not distributed
with Auric; copy the required ROMs into the package's `ROMS/` directory as described in
[`ROMS/README_ROMS.md`](../ROMS/README_ROMS.md).

The Windows archive includes vcpkg runtime DLLs when the selected build links
against shared libraries. The macOS archive is built for Apple Silicon.
