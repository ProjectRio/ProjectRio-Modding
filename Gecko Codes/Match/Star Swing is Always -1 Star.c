/*###########################################################
# Star Swing is Always -1 Star
###########################################################*/
// Author: Roeming
#include "Include/game/UnknownHomes_Game.h"

typedef struct { u8 moonShotCost, captainStarCost, nonCaptain_CaptainStarCost, regularStarCost; } StarPowerCosts;
#define g_StarPowerCosts VAR_ADDRESS(StarPowerCosts, 0x807B76F4)

CGECKO(StarSwingIsAlwaysOneStar, .state = MSSB_GAME,
       .notes = "Captain star swings cost only 1 star instead of 2.");
void StarSwingIsAlwaysOneStar(void)
{
    g_StarPowerCosts.nonCaptain_CaptainStarCost = 1;
}
