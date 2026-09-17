/*###########################################################
# Peach Blocks All Brick
###########################################################*/
// Author: LittleCoaks
#include "Include/Rio/StadiumObjects.h"

CGECKO(PeachBlocksAllBrick, .state = MSSB_GAME,
       .notes = "Changes all the blocks on Peach Garden to brick blocks.");
void PeachBlocksAllBrick(void)
{
    PeachBlocks_SetType(PEACH_BLOCK_BRICK);
}
