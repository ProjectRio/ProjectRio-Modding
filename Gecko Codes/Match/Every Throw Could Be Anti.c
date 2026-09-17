/*###########################################################
# Every Throw Could Be Anti
###########################################################*/
// Author: PeacockSlayer
// How a throw rolls for bad chemistry: docs/import_match_gameplay.md
#include "Include/game/UnknownHomes_Game.h"
#include "Include/Symbols/game.h"

#define chemThresholds ARRAY_1D_ADDRESS(s16, 4, chemThresholds_ADDR)

CGECKO(EveryThrowCouldBeAnti, .state = MSSB_GAME,
       .notes = "Any throw between two fielders can go wild as if they had bad chemistry.");
void EveryThrowCouldBeAnti(void)
{
    chemThresholds[0] = 100;
    chemThresholds[1] = 99;
}
