/*###########################################################
# Disable Replays
###########################################################*/
// Author: LittleCoaks
#include "Include/game/UnknownHomes_Game.h"

// determineIfReplayShouldPlay's "yes" exit: `stb r0, 0x39(r3)` with r0 = 1.
// The store is replaced outright (no .instruction), so the flag is written as 0.
CGECKO(DisableReplays, .address = 0x806BB21C, .state = MSSB_GAME,
       .notes = "Replays never play after a hit.");
void DisableReplays(void)
{
    READ_GAME_REG(u8*, replayState, 3);

    replayState[0x39] = 0;
}
