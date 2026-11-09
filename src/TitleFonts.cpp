#include "TitleFonts.h"

// The generated arrays are const (internal linkage). Including them here, in a
// single .cpp, keeps the ~145 KB of font data out of every other translation
// unit. We re-export external-linkage pointers for use via TitleFonts.h.
#include "fonts/TitleFont16.h"
#include "fonts/TitleFont32.h"
#include "fonts/TitleFont48.h"

const uint8_t* const TITLE_FONT_16 = TitleFont16;
const uint8_t* const TITLE_FONT_32 = TitleFont32;
const uint8_t* const TITLE_FONT_48 = TitleFont48;
