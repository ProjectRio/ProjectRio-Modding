/*###########################################################
# Bobble Always
###########################################################*/
// Author: PeacockSlayer
// What the bobble values mean: docs/import_match_gameplay.md
#include "Include/game/UnknownHomes_Game.h"

#define BOBBLE_STATE 3

CGECKO(BobbleAlways, .state = MSSB_GAME,
       .notes = "A joke code: fielders bobble the ball non stop. Makes the game unplayable.");
void BobbleAlways(void)
{
    int i;
    for (i = 0; i < 9; i++)
        g_Fielders[i].bobble = BOBBLE_STATE;
}
