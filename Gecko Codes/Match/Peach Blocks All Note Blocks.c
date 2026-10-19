/*###########################################################
# Peach Blocks All Note Blocks
###########################################################*/
// Author: LittleCoaks
#include "Include/Rio/StadiumObjects.h"

CGECKO(PeachBlocksAllNoteBlocks, .state = MSSB_GAME,
       .notes = "Changes all the blocks on Peach Garden to note blocks.");
void PeachBlocksAllNoteBlocks(void)
{
    PeachBlocks_SetType(PEACH_BLOCK_NOTE);
}
