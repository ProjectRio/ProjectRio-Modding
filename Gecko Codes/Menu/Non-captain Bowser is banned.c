/*###########################################################
# Non-captain Bowser is banned
###########################################################*/
// Author: LittleCoaks
// Character-select squares: docs/import_menu.md
#include "Include/Rio/CssSquares.h"

// Hooked on the store after Bowser's own square so the reset loop's r0 (-1) is dead across the hook.
CGECKO(NonCaptainBowserIsBanned, .address = 0x80051318, .instruction = "li r0, -1",
       .notes = "Bowser can only be selected on the captain select screen.\n"
                "Useful as competitive players typically ban non-captain Bowser.");
void NonCaptainBowserIsBanned(void)
{
    BanFromDraft(CSS_SQUARE_BOWSER, CHAR_ID_BOWSER);
    CssSquareOwner(CSS_SQUARE_BOWSER + 1) = SQUARE_FREE;
}
