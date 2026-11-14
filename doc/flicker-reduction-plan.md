# Display flicker-reduction plan

Goal: remove the visible flash that appears whenever a region of the screen is
redrawn (menu navigation, value updates, etc.) **without** hurting
responsiveness or spending much RAM.

## Background / root cause

Every widget redraw today follows the pattern **"paint background over the old
pixels, then paint new content on top."** Between those two SPI writes the
region is briefly shown blank — that is the flicker. Example, `drawMainRow()` in
`src/MenuSystem.cpp`:

```cpp
tft.fillRect(ROW_X-3, y-3, ROW_W+6, ROW_BOX_H+6, MOD_BG);  // row blanks…
tft.fillRect(ROW_X, y, ROW_W, ROW_BOX_H, bg);
drawVectorIcon(...); drawStringBig(...); drawStringCustom(...);  // …then refills
```

`startWrite()/endWrite()` only batches the SPI bus; it does **not** hide
intermediate frames. Only composing a region off-screen and pushing it in one
blit hides the blank frame.

### Hard constraint: no full back-buffer
Full double buffering = 480×320×16bpp = **~300 KB**. This `esp32dev` has
~320 KB internal DRAM, ~110 KB already used (static RAM 33.5%), and **no PSRAM**
(`platformio.ini` uses `huge_app.csv`, no `board_build.psram`). A full back
buffer will not fit. The solution must stay **region/widget-based**.

### Already done (reference implementation)
The Clock & Weather screen was converted in commit `60210a6`:
- Per-second time line composed into a persistent `TFT_eSprite` and pushed in
  one blit → flicker-free. See `drawClockTime()` in `src/ClockWeather.cpp`.
- Date/year split into `drawClockDate()`, repainted only when the day changes.
- Sprite is ~45 KB (346×66 px @16bpp), allocated once per screen, freed on exit,
  with a direct-draw fallback if allocation fails.

Reusable helpers already added to `include/DisplayTFT.h`:
- `smoothFontPtr(size, big)` — returns the baked font blob for a (size,big) pair.
- `smoothWidthPhysical(text, size, big)` — physical-pixel text width.
- `drawSmoothSprite(spr, sx, sy, text, fg, bg, size, big)` — draw opaque
  anti-aliased text INTO a sprite (no MISO read-back; blends fg→bg).

Scaling model to keep in mind: logical 320×240 → physical 480×320 via 4/3,
horizontally centred (originX = (480 − 320·4/3)/2 = 27, originY = 0). Use
`tft.logicalToPhysicalX/Y()` to map a logical rect to physical for sprites.

---

## Phase 1 — Opaque in-place text (broad, ~0 RAM) 

Where a dynamic element sits on a **solid-colour** background, it needs neither
the clear-then-draw dance nor the MISO read-back: draw the smooth font with an
explicit background colour so each glyph box overwrites itself in place.

Tasks:
1. Add an opaque smooth-text path to `DisplayTFT` (screen target, not sprite),
   e.g. `drawSmoothOpaque(x, y, text, fg, bg, size, big)`:
   - like `drawSmooth()` but `setTextColor(fg, bg)` (two-arg) and **no**
     `captureBg()` / callback read-back.
   - keep the `physicalMode` + `mapX/mapY` positioning identical.
2. Expose it through PepeDraw wrappers (parallel to `drawStringBig` /
   `drawStringCustom`), e.g. `drawStringBigOpaque` / `drawStringCustomOpaque`.
3. Convert text updates that live on a solid background to the opaque path and
   drop their preceding `fillRect` clear. Prime candidates:
   - packet/counter values, status labels, headers that change in place.
   - **Pad or fixed-width the field** (or clear only the exact delta) so a
     shorter new string cannot leave a tail from the old one.

Acceptance: updating those fields shows no blank flash; no RAM increase.
Risk: variable-width text leaving tails — handle by padding or delta-clear.

---

## Phase 2 — Pooled widget sprite + convert the main menu (reference)

For regions that opaque-in-place can't cover (overlap a non-solid background,
change shape — the selection glow box, icon+text composited together, variable
geometry), compose the whole widget in RAM and push once.

Tasks:
1. Add a pooled scratch-sprite helper to `DisplayTFT`, one buffer reused by all
   callers, sized to the largest common widget:
   ```cpp
   TFT_eSprite& beginPanel(lx, ly, lw, lh);  // lazily (re)allocates the pool
                                             // buffer to fit; fills bg
   void        endPanel();                    // pushSprite at mapped phys coords
   ```
   - Hold ONE pooled buffer (grow-only up to a cap, e.g. a menu row / value
     field). Never hold multiple large sprites at once.
   - Provide a fallback (if alloc fails, draw directly — accept minor flicker).
   - Decide ownership/lifetime: allocate lazily, free on screen exit (add a
     `tft.releasePanel()` call in each screen's exit path), mirroring the clock.
2. Convert `drawMainRow()` (`src/MenuSystem.cpp`) to compose a row into the
   pooled sprite (box bg + glow/rect + `drawVectorIcon` + title + subtitle +
   index) and push once. A row is ~203×39 px ≈ 16 KB.
   - `changeMainEntry()` already repaints only the 2 affected rows — keep that;
     just route both through the sprite path. Avoid full-screen
     `redrawMainMenu()` except on first entry.
3. Convert `drawPreviewPanel()` similarly (144×156 logical ≈ 192×208 ≈ 80 KB —
   either its own transient sprite or redraw its sub-fields opaquely).
4. `drawVectorIcon` and friends draw via `tft.*` primitives in logical coords —
   they must draw into the sprite instead. Either add sprite-targeted variants
   or have `beginPanel` return a target the existing helpers can use. Resolve
   this indirection cleanly (it is the main design decision of this phase).

Acceptance: moving through the menu is flicker-free and still snappy; RAM rise
limited to one pooled buffer, held only while the menu is active.

---

## Phase 3 — Roll out to remaining dynamic screens

Convert the other screens that redraw frequently (scanners, monitors, radio,
etc.) widget-by-widget using the Phase 1 opaque path and the Phase 2
`beginPanel/endPanel` helper. No new mechanism — just apply the two tools.

Order by how visible the flicker is / how often the screen updates.

---

## RAM & responsiveness budget
- Stay region-based; **one** pooled sprite per active screen, freed on exit.
- Target peak extra heap ≤ ~45–80 KB transiently (plenty of headroom:
  ~218 KB free after static RAM).
- Sprites should *improve* responsiveness: one bulk push (optionally DMA —
  `SUPPORT_TRANSACTIONS` is already enabled) beats many small scaled primitive
  writes, and drops the slow MISO read-backs.

## Verification each phase
1. `pio run` builds; check RAM/flash % in the size report.
2. Flash: `pio run -t upload --upload-port /dev/ttyUSB0`.
3. Eyeball the converted screen on the panel for flashing.
4. When satisfied, rebuild the web-flasher binaries and commit:
   - app = `.pio/build/esp32dev/firmware.bin`
   - merged (bootloader@0x1000, partitions@0x8000, boot_app0@0xe000,
     app@0x10000) via `esptool.py merge_bin` → copy to
     `assets/Firmware/{firmware.bin,firmware-merged.bin}` and
     `firmware/firmware-merged.bin`.
   - bootloader/partitions are unchanged — verify with `cmp`.

## Open design questions to settle first
- Phase 2 step 4: cleanest way to let the existing logical-coord draw helpers
  (`drawVectorIcon`, `drawStringBig`, lines/rects) target a sprite. Options:
  (a) sprite-aware variants, (b) a "current target" pointer in DisplayTFT that
  the overrides honour, (c) draw primitives directly into the sprite with a
  thin coord shim. Pick one before converting many screens.
- Whether to keep the pooled buffer alive across screens (simpler, costs RAM
  while idle) or allocate/free per screen (what the clock does).
