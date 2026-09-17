/*###########################################################
# Default Mercy On
###########################################################*/
// Author: LittleCoaks
#include "Include/static/UnknownHomes_Static.h"

// The game's `gameSettings` object (decomp symbols.txt: .bss 0x803C5F04, size 0x70);
// not yet in the synced headers. Byte 0x3F is the mercy rule.
#define GAMESETTINGS_MERCY VAR_ADDRESS(u8, 0x803C5F43)

// Hooked a few stores after the mercy default itself, at the first site where
// r0 is no longer live in the defaults block.
CGECKO(DefaultMercyOn, .address = 0x800498EC, .instruction = "stb r6, 0x44(r7)",
       .notes = "Mercy rule starts switched on when setting up a game.");
void DefaultMercyOn(void)
{
    GAMESETTINGS_MERCY = 1;
}
