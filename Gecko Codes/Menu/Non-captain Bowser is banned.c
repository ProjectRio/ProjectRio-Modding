/*###########################################################
# Non-captain Bowser is banned
###########################################################*/
// Author: LittleCoaks
// Character-select squares: docs/import_menu.md
#include "Include/static/UnknownHomes_Static.h"

#define BOWSER_SQUARE 11
#define CssSquareOwner(square)  VAR_ADDRESS(u8, 0x803C6050 + (square))
#define SQUARE_TAKEN 0

// Hooked on the store after Bowser's own square so the reset loop's r0 (-1) is dead across the hook.
CGECKO(NonCaptainBowserIsBanned, .address = 0x80051318, .instruction = "li r0, -1",
       .notes = "Bowser can only be selected on the captain select screen.\n"
                "Useful as competitive players typically ban non-captain Bowser.");
void NonCaptainBowserIsBanned(void)
{
    CssSquareOwner(BOWSER_SQUARE) = SQUARE_TAKEN;
    CssSquareOwner(BOWSER_SQUARE + 1) = 0xFF;
    Static_Stats_Tables.charOnCharacterGridSelected[CHAR_ID_BOWSER] = 1;
}
