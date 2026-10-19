/*###########################################################
# Every Hit Is A Random Star Hit
###########################################################*/
// Author: Roeming
#include "Include/game/UnknownHomes_Game.h"

#define CAPTAIN_STAR_SWING_COUNT 12

CGECKO(EveryHitIsARandomStarHit, .address = 0x806517F4, .state = MSSB_GAME,
       .instruction = "lis r3, 0x8089",
       .notes = "Every hit is a random star hit.");
void EveryHitIsARandomStarHit(void)
{
    g_Batter.captainStarSwingActivated = g_Ball.StaticRandomInt1 % CAPTAIN_STAR_SWING_COUNT + 1;
}
