/*###########################################################
# Disable Bobbles
###########################################################*/
// Author: PeacockSlayer
// What the bobble values mean: docs/import_match_gameplay.md
#include "Include/game/UnknownHomes_Game.h"

#define BOBBLE_STATE 0

CGECKO(DisableBobbles, .state = MSSB_GAME,
       .notes = "Fielders never bobble the ball.");
void DisableBobbles(void)
{
    int i;
    for (i = 0; i < 9; i++)
        g_Fielders[i].bobble = BOBBLE_STATE;
}
