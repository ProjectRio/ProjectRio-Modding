/*###########################################################
# Peach Blocks All Metal
###########################################################*/
// Author: LittleCoaks
#include "Include/Rio/StadiumObjects.h"

CGECKO(PeachBlocksAllMetal, .state = MSSB_GAME,
       .notes = "Changes all the blocks on Peach Garden to metal blocks.");
void PeachBlocksAllMetal(void)
{
    PeachBlocks_SetType(PEACH_BLOCK_METAL);
}
