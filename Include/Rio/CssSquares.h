// Character-select grid squares and the draft "taken" table: docs/import_menu.md
#pragma once
#include "Include/static/UnknownHomes_Static.h"

#define CssSquareOwner(square)  VAR_ADDRESS(u8, 0x803C6050 + (square))
#define SQUARE_TAKEN 0
#define SQUARE_FREE  0xFF

#define CSS_SQUARE_BOWSER 11
#define CSS_SQUARE_SHYGUY 24

static inline void BanFromDraft(int square, int charID)
{
    CssSquareOwner(square) = SQUARE_TAKEN;
    Static_Stats_Tables.charOnCharacterGridSelected[charID] = 1;
}
