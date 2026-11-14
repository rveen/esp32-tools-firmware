#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "TitleFonts.h"

class DisplayTFT : public TFT_eSPI {
public:
    static constexpr int32_t LOGICAL_W = 320;
    static constexpr int32_t LOGICAL_H = 240;
    static constexpr int32_t SCALE_NUM = 4;
    static constexpr int32_t SCALE_DEN = 3;

    DisplayTFT() : TFT_eSPI() {}

    int32_t logicalToPhysicalX(int32_t x) {
        return mapX(x);
    }

    int32_t logicalToPhysicalY(int32_t y) {
        return mapY(y);
    }

    void drawNativeString(int32_t x, int32_t y, const String& text,
                          uint32_t color, uint32_t bg,
                          uint8_t font = 2, uint8_t size = 1) {
        TFT_eSPI::setTextColor(color, bg);
        TFT_eSPI::setTextSize(size);
        TFT_eSPI::setTextDatum(TL_DATUM);
        physicalMode = true;
        TFT_eSPI::drawString(text, mapX(x), mapY(y), font);
        physicalMode = false;
    }

    void drawNativeStringTransparent(int32_t x, int32_t y, const String& text,
                                     uint32_t color,
                                     uint8_t font = 2, uint8_t size = 1) {
        TFT_eSPI::setTextColor(color);
        TFT_eSPI::setTextSize(size);
        TFT_eSPI::setTextDatum(TL_DATUM);
        physicalMode = true;
        TFT_eSPI::drawString(text, mapX(x), mapY(y), font);
        physicalMode = false;
    }

    int32_t nativeTextWidth(const String& text, uint8_t font = 2, uint8_t size = 1) {
        TFT_eSPI::setTextSize(size);
        return (TFT_eSPI::textWidth(text, font) * SCALE_DEN) / SCALE_NUM;
    }

    // ── Anti-aliased smooth text ────────────────────────────────────────────
    // Enables transparent anti-aliasing: smooth-font edge pixels are blended
    // against the real on-screen background read back over MISO (requires the
    // panel's SDO wired to GPIO19 and TFT_MISO=19). Call once after begin().
    void beginSmoothFonts() {
        TFT_eSPI::setCallback(&DisplayTFT::readBgCallback);
    }

    // Draw an anti-aliased string at true physical resolution (bypassing the
    // 4/3 logical->physical stretch). Transparent: glyph edges blend against
    // whatever is already on screen, so it works on any background. `big`
    // selects the bold title fonts, otherwise the regular body fonts.
    void drawSmoothBig(int32_t x, int32_t y, const String& text,
                       uint32_t fg, uint32_t /*bg, unused*/, int size) {
        drawSmooth(x, y, text, fg, size, true);
    }
    void drawSmoothSmall(int32_t x, int32_t y, const String& text,
                         uint32_t fg, int size) {
        drawSmooth(x, y, text, fg, size, false);
    }

    // Logical width of a smooth string, so centred/right-aligned text is
    // positioned to match what is actually drawn.
    int32_t smoothBigWidthLogical(const String& text, int size) {
        return smoothWidthLogical(text, size, true);
    }
    int32_t smoothSmallWidthLogical(const String& text, int size) {
        return smoothWidthLogical(text, size, false);
    }

    void drawPixel(int32_t x, int32_t y, uint32_t color) override {
        if (physicalMode) {
            fillPhysicalRect(x, y, 1, 1, color);
            return;
        }
        fillPhysicalRect(mapX(x), mapY(y), scaledSpan(x, 1), scaledSpan(y, 1), color);
    }

    void drawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t color) override {
        if (physicalMode) {
            TFT_eSPI::drawLine(x0, y0, x1, y1, color);
            return;
        }
        if (x0 == x1) {
            int32_t y = min(y0, y1);
            drawFastVLine(x0, y, abs(y1 - y0) + 1, color);
            return;
        }
        if (y0 == y1) {
            int32_t x = min(x0, x1);
            drawFastHLine(x, y0, abs(x1 - x0) + 1, color);
            return;
        }

        int32_t dx = abs(x1 - x0);
        int32_t sx = x0 < x1 ? 1 : -1;
        int32_t dy = -abs(y1 - y0);
        int32_t sy = y0 < y1 ? 1 : -1;
        int32_t err = dx + dy;

        while (true) {
            drawPixel(x0, y0, color);
            if (x0 == x1 && y0 == y1) return;
            int32_t e2 = err * 2;
            if (e2 >= dy) {
                err += dy;
                x0 += sx;
            }
            if (e2 <= dx) {
                err += dx;
                y0 += sy;
            }
        }
    }

    void drawFastVLine(int32_t x, int32_t y, int32_t h, uint32_t color) override {
        if (physicalMode) {
            fillPhysicalRect(x, y, 1, h, color);
            return;
        }
        fillRect(x, y, 1, h, color);
    }

    void drawFastHLine(int32_t x, int32_t y, int32_t w, uint32_t color) override {
        if (physicalMode) {
            fillPhysicalRect(x, y, w, 1, color);
            return;
        }
        fillRect(x, y, w, 1, color);
    }

    void fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) override {
        if (w <= 0 || h <= 0) return;
        if (physicalMode) {
            fillPhysicalRect(x, y, w, h, color);
            return;
        }
        fillPhysicalRect(mapX(x), mapY(y), scaledSpan(x, w), scaledSpan(y, h), color);
    }

    void fillScreen(uint32_t color) {
        TFT_eSPI::fillRect(0, 0, TFT_eSPI::width(), TFT_eSPI::height(), color);
    }

    void drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
        if (w <= 0 || h <= 0) return;
        drawFastHLine(x, y, w, color);
        drawFastHLine(x, y + h - 1, w, color);
        drawFastVLine(x, y, h, color);
        drawFastVLine(x + w - 1, y, h, color);
    }

    void drawCircle(int32_t x0, int32_t y0, int32_t r, uint32_t color) {
        if (r < 0) return;
        int32_t f = 1 - r;
        int32_t ddF_x = 1;
        int32_t ddF_y = -2 * r;
        int32_t x = 0;
        int32_t y = r;

        drawPixel(x0, y0 + r, color);
        drawPixel(x0, y0 - r, color);
        drawPixel(x0 + r, y0, color);
        drawPixel(x0 - r, y0, color);

        while (x < y) {
            if (f >= 0) {
                y--;
                ddF_y += 2;
                f += ddF_y;
            }
            x++;
            ddF_x += 2;
            f += ddF_x;

            drawPixel(x0 + x, y0 + y, color);
            drawPixel(x0 - x, y0 + y, color);
            drawPixel(x0 + x, y0 - y, color);
            drawPixel(x0 - x, y0 - y, color);
            drawPixel(x0 + y, y0 + x, color);
            drawPixel(x0 - y, y0 + x, color);
            drawPixel(x0 + y, y0 - x, color);
            drawPixel(x0 - y, y0 - x, color);
        }
    }

    void fillCircle(int32_t x0, int32_t y0, int32_t r, uint32_t color) {
        if (r < 0) return;
        drawFastVLine(x0, y0 - r, 2 * r + 1, color);

        int32_t f = 1 - r;
        int32_t ddF_x = 1;
        int32_t ddF_y = -2 * r;
        int32_t x = 0;
        int32_t y = r;

        while (x < y) {
            if (f >= 0) {
                y--;
                ddF_y += 2;
                f += ddF_y;
            }
            x++;
            ddF_x += 2;
            f += ddF_x;

            drawFastVLine(x0 + x, y0 - y, 2 * y + 1, color);
            drawFastVLine(x0 - x, y0 - y, 2 * y + 1, color);
            drawFastVLine(x0 + y, y0 - x, 2 * x + 1, color);
            drawFastVLine(x0 - y, y0 - x, 2 * x + 1, color);
        }
    }

private:
    bool physicalMode = false;

    // Smooth-font background read-back callback (physical coords). Blends AA
    // edges against the current framebuffer pixel. Served from the bulk-read
    // buffer when available, else falls back to a direct readPixel().
    static uint16_t readBgCallback(uint16_t x, uint16_t y);

    // One bulk SPI read of a region into a RAM buffer that readBgCallback
    // indexes. Capped so a very wide string falls back to per-pixel reads
    // rather than risk a large allocation.
    static uint16_t* s_bgBuf;
    static int32_t s_bgX, s_bgY, s_bgW, s_bgH;

    void captureBg(int32_t x, int32_t y, int32_t w, int32_t h) {
        s_bgBuf = nullptr;
        if (w <= 0 || h <= 0) return;
        if (x < 0) { w += x; x = 0; }
        if (y < 0) { h += y; y = 0; }
        if (x + w > TFT_eSPI::width())  w = TFT_eSPI::width()  - x;
        if (y + h > TFT_eSPI::height()) h = TFT_eSPI::height() - y;
        if (w <= 0 || h <= 0) return;
        size_t n = (size_t)w * h;
        if (n > 32768) return;                 // >64 KB: keep per-pixel fallback
        uint16_t* b = (uint16_t*)malloc(n * 2);
        if (!b) return;                        // OOM: per-pixel fallback
        TFT_eSPI::readRect(x, y, w, h, b);
        s_bgBuf = b; s_bgX = x; s_bgY = y; s_bgW = w; s_bgH = h;
    }
    void releaseBg() {
        if (s_bgBuf) { free(s_bgBuf); s_bgBuf = nullptr; }
    }

    void loadSmoothFont(int size, bool big) {
        const uint8_t* f;
        if (big) {
            f = TITLE_FONT_16;                     // size 1  -> 16 px
            if (size >= 3)      f = TITLE_FONT_48; // size 3+ -> 48 px
            else if (size == 2) f = TITLE_FONT_32; // size 2  -> 32 px
        } else {
            f = BODY_FONT_13;                      // size 1  -> 13 px
            if (size >= 3)      f = BODY_FONT_28;  // size 3+ -> 28 px
            else if (size == 2) f = BODY_FONT_19;  // size 2  -> 19 px
        }
        TFT_eSPI::loadFont(f);
    }

    void drawSmooth(int32_t x, int32_t y, const String& text,
                    uint32_t fg, int size, bool big) {
        loadSmoothFont(size, big);
        // Single-arg colour => _fillbg=false => transparent; the read-back
        // callback supplies the real background for edge anti-aliasing.
        TFT_eSPI::setTextColor((uint16_t)fg);
        TFT_eSPI::setTextDatum(TL_DATUM);
        physicalMode = true;
        int32_t px = mapX(x), py = mapY(y);
        // Capture the whole string's background in ONE bulk SPI read, so the
        // per-edge-pixel callback is served from RAM instead of doing a slow
        // readPixel() (address-window setup) for every anti-aliased pixel.
        captureBg(px - 1, py - 1,
                  TFT_eSPI::textWidth(text) + 2, TFT_eSPI::fontHeight() + 2);
        TFT_eSPI::drawString(text, px, py);
        releaseBg();
        physicalMode = false;
        TFT_eSPI::unloadFont();   // restore GLCD path for drawNativeString()
    }

    int32_t smoothWidthLogical(const String& text, int size, bool big) {
        loadSmoothFont(size, big);
        int32_t w = TFT_eSPI::textWidth(text);
        TFT_eSPI::unloadFont();
        return (w * SCALE_DEN) / SCALE_NUM;
    }

    static int32_t scaledValue(int32_t value) {
        int32_t numerator = value * SCALE_NUM;
        if (numerator >= 0) return numerator / SCALE_DEN;
        return -((-numerator + SCALE_DEN - 1) / SCALE_DEN);
    }

    int32_t viewportW() {
        return scaledValue(LOGICAL_W);
    }

    int32_t viewportH() {
        return scaledValue(LOGICAL_H);
    }

    int32_t originX() {
        return (TFT_eSPI::width() - viewportW()) / 2;
    }

    int32_t originY() {
        return (TFT_eSPI::height() - viewportH()) / 2;
    }

    int32_t mapX(int32_t x) {
        return originX() + scaledValue(x);
    }

    int32_t mapY(int32_t y) {
        return originY() + scaledValue(y);
    }

    static int32_t scaledSpan(int32_t start, int32_t span) {
        int32_t result = scaledValue(start + span) - scaledValue(start);
        return max<int32_t>(1, result);
    }

    void fillPhysicalRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
        TFT_eSPI::fillRect(x, y, w, h, color);
    }
};

extern DisplayTFT tft;
