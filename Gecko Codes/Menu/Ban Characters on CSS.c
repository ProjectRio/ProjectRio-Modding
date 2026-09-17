/*###########################################################
# Ban Characters on CSS
###########################################################*/
// Author: LittleCoaks
// Character-select squares and the pad bytes read here: docs/import_menu.md
#include "Include/Dolphin/pad.h"

#define PLAYER_COUNT 4
#define PadNewButtonsHigh(player)  VAR_ADDRESS(u8, 0x803C77BE + (player) * 0x20)
#define CssCursorSquare(player)    VAR_ADDRESS(u8, 0x803C6041 + (player) * 2)
#define CssSquareOwner(square)     VAR_ADDRESS(u8, 0x803C6050 + (square))
#define SQUARE_TAKEN 0

CGECKO(BanCharactersOnCSS, .address = 0x80051AA8, .instruction = "lhz r0, 0x14(r27)",
       .notes = "Press X when hovering over a character to ban them.");
void BanCharactersOnCSS(void)
{
    for (int player = 0; player < PLAYER_COUNT; player++)
    {
        if (PadNewButtonsHigh(player) == (PAD_BUTTON_X >> 8))
            CssSquareOwner(CssCursorSquare(player)) = SQUARE_TAKEN;
    }
}
