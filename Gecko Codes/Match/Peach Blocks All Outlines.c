/*###########################################################
# Peach Blocks All Outlines
###########################################################*/
// Author: LittleCoaks
#include "Include/Rio/StadiumObjects.h"

CGECKO(PeachBlocksAllOutlines, .state = MSSB_GAME,
       .notes = "Changes all the blocks on Peach Garden to outline blocks.");
void PeachBlocksAllOutlines(void)
{
    PeachBlocks_SetType(PEACH_BLOCK_OUTLINE);
}
