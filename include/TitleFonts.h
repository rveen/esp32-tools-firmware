#pragma once
#include <Arduino.h>

// Anti-aliased smooth fonts for FONT_BIG titles (see src/TitleFonts.cpp and
// tools/ttf2vlw.py). Only the pointers are exported here so the large font
// byte arrays live in a single translation unit, not in every file that
// includes DisplayTFT.h.
//
// Baked physical pixel heights match FONT_BIG's on-screen box (12 * size * 4/3):
//   size 1 -> 16 px, size 2 -> 32 px, size >= 3 -> 48 px.
extern const uint8_t* const TITLE_FONT_16;
extern const uint8_t* const TITLE_FONT_32;
extern const uint8_t* const TITLE_FONT_48;

// Body (FONT_SMALL) smooth fonts, Liberation Sans Regular, baked at the body
// text physical heights (7 * size * 4/3):
//   size 1 -> 10 px, size 2 -> 19 px, size 3 -> 28 px.
extern const uint8_t* const BODY_FONT_10;
extern const uint8_t* const BODY_FONT_19;
extern const uint8_t* const BODY_FONT_28;
