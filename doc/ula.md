# ULA implementation (src/chip/ula.cpp / src/chip/ula.hpp)

This document describes the Oric ULA implementation found in `src/chip/ula.hpp` and `src/chip/ula.cpp` and maps the code to the informal ULA documentation available in `doc/external/OricAtmosUnofficialULAGuide-1.02.pdf` and `doc/external/Oric_graphics_in_details.html`.

Overview
--------
- The `ULA` class implements the Oric video logic: raster timing, text/hires rendering, palette handling, blink and a simple "warpmode" frame-skip used by the frontend.
- Primary entry point used by the emulator: `bool ULA::paint_raster()` which is called once per ULA raster step (line). When a frame completes it calls the frontend to render the pixel buffer.
- Rendering output is a raw pixel buffer stored in `pixels` (a `std::vector<uint8_t>`) sized to `texture_width * texture_height * texture_bpp` supplied by the frontend on construction.

Files
-----
- `src/chip/ula.hpp` — public class, enums and configurable palette.
- `src/chip/ula.cpp` — the raster logic: `paint_raster()`, `update_graphics()` and helpers.
- Reference docs: `doc/external/OricAtmosUnofficialULAGuide-1.02.pdf` and `doc/external/Oric_graphics_in_details.html`.

Public API
----------
- Constructor: `ULA(Machine& machine, Memory& memory, uint8_t texture_width, uint8_t texture_height, uint8_t texture_bpp)` — ULA is constructed with references to the emulator `Machine` and `Memory` and the target texture dimensions.
- `bool paint_raster()` — paint a single raster line. Returns `true` when a full frame has been produced and the emulator should render the `pixels` buffer to the screen.

State and configuration
-----------------------
- Palette: `uint32_t colors[8]` — default ARGB-like palette (0xffRRGGBB). You can change entries at runtime to alter the palette.
- Video mode flags (enum `VideoAttribs`): `HZ_50` (unused in the shown code but reserved) and `HIRES` (select hires mode).
- Text attributes (enum `TextAttribs`): `ALTERNATE_CHARSET`, `DOUBLE_SIZE`, `BLINKING` — used by the text rendering path.
- Internal fields used by the renderer:
  - `video_attrib`, `text_attrib` — current mode/attributes read from control characters in screen memory.
  - `raster_current` — current raster line index (0..312 exclusive of `raster_max`).
  - `blink` and `frame_count` — blink state updated per frame.
  - `warpmode_counter` — small frame-skip counter used to throttle rendering when the machine is in a warp mode.
  - `pixels` — pixel output buffer sent to the frontend when a frame completes.

Raster timing and visible area
-----------------------------
- `raster_max` is defined as 312 lines per frame.
- Visible lines are defined as 224 lines starting at `raster_visible_first = 44` (so `raster_visible_last = raster_visible_first + raster_visible_lines`). This matches the Oric's visible vertical area approximated by many emulators.
- `paint_raster()` increments `raster_current` and calls `update_graphics()` only when the current raster is within the visible range.
- When `raster_current` wraps to 0 (end of frame), `paint_raster()` calls `machine.frontend->render_graphics(pixels)` and returns `true`.
- `warpmode_counter` is used to skip rendering frames when `machine.warpmode_on` is true: it increments and only triggers full frame rendering every N frames (N = 25 in this code; it returns false early to avoid render for most frames).

Addressing and layout of screen memory
-------------------------------------
- Helper `calcRowAddr(raster_line, video_attrib)` computes the base memory address for the given raster line. Two cases:
  - Hires mode (and raster_line < 200): map to `0xa000 + raster_line * 40` — each visible raster line reads 40 bytes (for 240 pixel width at 6 pixel/char packing).
  - Text (lores or hires above 200): use `0xbb80 + (raster_line >> 3) * 40` — each eight raster lines (character row) use a set of 40 character codes at address `0xbb80 + row*40`.

Note: These addresses are derived from Oric memory layout conventions used by many emulators; confirm against your ROM/map if you changed memory mapping.

Character rendering and control codes
-------------------------------------
- The ULA reads 40 bytes per character row (columns) from video memory.
- Each byte `ch` is treated as either a control byte or a character code.
  - Control bytes are detected when `(ch & 0x60) == 0` (bits 5 and 6 clear). These bytes change ink, paper, video mode, text attributes or blinking state.
  - Control semantics implemented in `update_graphics()`:
    - `case 0x00`: set ink color (foreground) using `colors[ch & 7]`.
    - `case 0x08`: set `text_attrib` and blink enable/disable.
    - `case 0x10`: set paper color (background).
    - `case 0x18`: set `video_attrib` (select HIRES/lores) and recompute row base address.
- Character codes with the high bit set (0x80) invert foreground and background colors for that cell.

Character pixel bits and masks
------------------------------
- `mask` is calculated per character using `uint8_t mask = frame_count & 0x10 ? 0x3f : blink;` — this interacts with blinking and frame count to alter visible character data (used for character generator indexing/visibility).
- For text mode (non-hires or hires when raster_line >= 200) the code computes character pixel data by indexing into a character memory block at either `0xb400` (default text charset) or `0x9800` (hires > 200 charset). An `apan` (row within character) index of 0..7 is computed; when `text_attrib & DOUBLE_SIZE` is set, `apan` uses `(raster_line >> 1) & 0x07` to stretch characters vertically.
- In hires mode for raster lines < 200 the code uses `chr_dat = ch & mask;` — i.e., bytes in screen memory directly encode pixel patterns, and the mask is applied to produce blinking / warping effects.

Pixel packing and output
------------------------
- Each character cell maps to 6 pixels horizontally. For each of the 6 bit positions (0x20 .. 0x01) the code writes one pixel to the `pixels` buffer as either foreground or background color.
- `pixels` is an 8-bit per component buffer sized with `texture_width * texture_height * texture_bpp`. The code uses a `uint32_t*` pointer overlay (`texture_line`) to write 32-bit color values directly (so `texture_bpp` is expected to be 4 / 32-bit in the existing frontend usage). If you switch to a different `texture_bpp`, you must ensure the rendering loop and frontend are consistent.

Blink and frame_count
---------------------
- `blink` is initialized to `0x3f`. `frame_count` increments once per frame in `paint_raster()` when the raster wraps.
- `mask` selects whether to use 0x3f or `blink` depending on `frame_count & 0x10` — this yields a blink rhythm over multiple frames.

Warp mode
---------
- `machine.warpmode_on` is consulted at the end of each frame: when on, `warpmode_counter` increments modulo 25 and the renderer returns early (doesn't render) for most increments — this reduces frames actually submitted to the frontend to speed up emulation in "warp" mode.

Integration points and responsibilities
---------------------------------------
- `ULA` interacts with the rest of the emulator via:
  - Read access to `memory.mem[...]` for framebuffer and charset bytes.
  - `machine.frontend->render_graphics(pixels)` called to submit a full-frame buffer to the frontend.
  - `machine.warpmode_on` used to optionally skip/frame-throttle rendering.
- The ULA does not handle timing beyond per-line progression — the emulator must call `paint_raster()` at the right frequency to reach accurate video timing.

Where the code deviates from (or requires confirmation against) the reference docs
--------------------------------------------------------------------------------
- Visible area and raster timings: `raster_visible_first = 44` and 224 visible lines are chosen values and may differ slightly from other emulators or the real hardware's timing depending on PAL/NTSC variants. Compare with the guides for exact timing if you need cycle-accurate raster effects.
- Character ROM addresses (`0xb400` and `0x9800`) are conventions used by this emulator; confirm they match the memory map the rest of the emulator uses.
- The exact blink rhythm created by `frame_count & 0x10` together with the `blink` variable is a pragmatic choice by this implementation and may not match other implementations exactly — adjust if you need the same blink cadence as a specific real machine.
- The code expects `texture_bpp` compatible with a 32-bit write path (`uint32_t* texture_line`); if your front-end uses a different pixel format, you must adapt `update_graphics()`.

