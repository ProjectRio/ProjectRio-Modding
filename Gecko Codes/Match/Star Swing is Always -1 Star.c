/*###########################################################
# Star Swing is Always -1 Star
###########################################################*/
// Author: Roeming
// Which cost applies to whom: docs/import_match_gameplay.md
#include "Include/game/UnknownHomes_Game.h"

typedef struct { u8 moonShotCost, captainStarCost, nonCaptain_CaptainStarCost, regularStarCost; } StarPowerCosts;
#define g_StarPowerCosts VAR_ADDRESS(StarPowerCosts, 0x807B76F4)

CGECKO(StarSwingIsAlwaysOneStar, .state = MSSB_GAME,
       .notes = "A captain character who is not your team captain pays 1 star\n"
                "for their special star swing instead of 2, like the team captain does.");
void StarSwingIsAlwaysOneStar(void)
{
    g_StarPowerCosts.nonCaptain_CaptainStarCost = 1;
}
