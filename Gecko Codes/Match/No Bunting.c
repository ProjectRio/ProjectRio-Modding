/*###########################################################
# No Bunting
###########################################################*/
// Author: Roeming
#include "Include/game/UnknownHomes_Game.h"

#define BEQ_SKIP_BUNT 0x41820034
#define B_SKIP_BUNT   0x48000034

CGECKO(NoBunting, .state = MSSB_GAME,
       .notes = "Batters cannot bunt.");
void NoBunting(void)
{
    PatchInstruction_Conditional(0x806532F0, BEQ_SKIP_BUNT, B_SKIP_BUNT);   /* batterHumanControlled */
}
