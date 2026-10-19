/*###########################################################
# Never Ball Trail
###########################################################*/
// Author: Roeming
#include "Include/game/UnknownHomes_Game.h"

#define STWU_PROLOGUE 0x9421FFE0
#define BLR           0x4E800020

CGECKO(NeverBallTrail, .state = MSSB_GAME,
       .notes = "The ball never leaves a trail behind it.");
void NeverBallTrail(void)
{
    PatchInstruction_Conditional(0x806A66B4, STWU_PROLOGUE, BLR);   /* setupBallTrailEffect */
}
