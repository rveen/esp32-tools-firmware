#include "TitleFonts.h"
#include "DisplayTFT.h"

// Background-capture buffer (one bulk read per string; see DisplayTFT.h).
uint16_t* DisplayTFT::s_bgBuf = nullptr;
int32_t DisplayTFT::s_bgX = 0, DisplayTFT::s_bgY = 0,
        DisplayTFT::s_bgW = 0, DisplayTFT::s_bgH = 0;

// Smooth-font background read-back callback: returns the current on-screen
// pixel so anti-aliased glyph edges blend against the real background. Served
// from the bulk-read buffer when the pixel is inside it, else readPixel().
uint16_t DisplayTFT::readBgCallback(uint16_t x, uint16_t y) {
    if (s_bgBuf) {
        int32_t lx = (int32_t)x - s_bgX, ly = (int32_t)y - s_bgY;
        if (lx >= 0 && ly >= 0 && lx < s_bgW && ly < s_bgH)
            return s_bgBuf[ly * s_bgW + lx];
    }
    return tft.readPixel(x, y);
}

// The generated arrays are const (internal linkage). Including them here, in a
// single .cpp, keeps the ~145 KB of font data out of every other translation
// unit. We re-export external-linkage pointers for use via TitleFonts.h.
#include "fonts/TitleFont16.h"
#include "fonts/TitleFont32.h"
#include "fonts/TitleFont48.h"
#include "fonts/BodyFont13.h"
#include "fonts/BodyFont19.h"
#include "fonts/BodyFont28.h"

const uint8_t* const TITLE_FONT_16 = TitleFont16;
const uint8_t* const TITLE_FONT_32 = TitleFont32;
const uint8_t* const TITLE_FONT_48 = TitleFont48;
const uint8_t* const BODY_FONT_13 = BodyFont13;
const uint8_t* const BODY_FONT_19 = BodyFont19;
const uint8_t* const BODY_FONT_28 = BodyFont28;
