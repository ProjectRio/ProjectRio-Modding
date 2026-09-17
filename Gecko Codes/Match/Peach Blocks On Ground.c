/*###########################################################
# Peach Blocks On Ground
###########################################################*/
// Author: LittleCoaks
#include "Include/Rio/StadiumObjects.h"

CGECKO(PeachBlocksOnGround, .state = MSSB_GAME,
       .notes = "Moves the blocks of Peach Garden onto the ground instead of in the air.");
void PeachBlocksOnGround(void)
{
    for (int i = 0; i < PEACH_BLOCK_COUNT; i++)
        g_PeachBlocks[i].y = 0.0f;
}
