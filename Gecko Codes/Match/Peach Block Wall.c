/*###########################################################
# Peach Block Wall
###########################################################*/
// Author: LittleCoaks
#include "Include/Rio/StadiumObjects.h"

#define WALL_ROW_LENGTH   8
#define WALL_Z            30.0f
#define WALL_UPPER_ROW_Y  -4.0f

static const f32 WALL_COLUMN_X[WALL_ROW_LENGTH] = { 2.0f, -2.0f, 6.0f, -6.0f, 10.0f, -10.0f, 14.0f, -14.0f };

CGECKO(PeachBlockWall, .state = MSSB_GAME,
       .notes = "Moves all the blocks on Peach Garden into a wall in the infield.");
void PeachBlockWall(void)
{
    for (int i = 0; i < PEACH_BLOCK_COUNT; i++)
    {
        StadiumObjectPlacement* block = &g_PeachBlocks[i];
        block->x        = WALL_COLUMN_X[i % WALL_ROW_LENGTH];
        block->y        = (i < WALL_ROW_LENGTH) ? 0.0f : WALL_UPPER_ROW_Y;
        block->z        = WALL_Z;
        block->rotation = 0.0f;
    }
}
