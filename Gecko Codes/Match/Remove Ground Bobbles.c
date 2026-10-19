/*###########################################################
# Remove Ground Bobbles
###########################################################*/
// Author: Roeming, LittleCoaks
// BobbleArray's layout: docs/import_match_gameplay.md
#include "Include/game/UnknownHomes_Game.h"
#include "Include/Symbols/game.h"

#define BobbleArray ARRAY_3D_ADDRESS(u8, 2, 4, 6, BobbleArray_ADDR)
#define NORMAL_FIELD        0
#define CHARACTER_CLASSES   4
#define GROUND_BALL_CHANCES 3

CGECKO(RemoveGroundBobbles, .state = MSSB_GAME,
       .notes = "Fielders will not bobble the ball when fielding a ground ball.");
void RemoveGroundBobbles(void)
{
    int charClass, kind;
    for (charClass = 0; charClass < CHARACTER_CLASSES; charClass++)
        for (kind = 0; kind < GROUND_BALL_CHANCES; kind++)
            BobbleArray[NORMAL_FIELD][charClass][kind] = 0;
}
