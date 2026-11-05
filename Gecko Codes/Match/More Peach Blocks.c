/*###########################################################
# More Peach Blocks
###########################################################*/
// Author: Nuche17
// Why 21 blocks is the limit and how the table is laid out: docs/match_codes.md
#include "Include/Rio/StadiumObjects.h"

/* Field coordinates in meters: home at the origin, +x toward 1st base, +z toward
   center. y is the block's BOTTOM, and negative is up; a block is a 4 m cube. */
#define BURIED_HALFWAY_Y   2.0f     /* 2 m below ground, 2 m above   */
#define BEHIND_2B_Y       -1.0f     /* 1 m off the ground            */
#define LINE_DRIVE_Y      -3.0f     /* 3 m off the ground            */

#define PARALLEL_1B_2B    44.0f     /* the 1B-2B baseline's angle    */
#define PARALLEL_3B_2B   316.0f     /* mirrored for the 3B-2B side   */

#define EXTRA_BLOCK_TYPE  PEACH_BLOCK_NOTE

#define VANILLA_BLOCKS     16
#define DEBRIS_ENTRIES      9
/* 52 objects hold N blocks + 9 debris + N shadows + the fountain: N <= 21. */
#define MAX_BLOCKS         21

/* Collision groups 0 and 8 are unused in vanilla (1..7 are blocks, 9 debris). */
static const StadiumObjectPlacement EXTRA_BLOCKS[] = {
    /* Between the 1B and 2B fielders, just behind the baseline, half buried. */
    { 14.2f, BURIED_HALFWAY_Y, 28.4f, PARALLEL_1B_2B, EXTRA_BLOCK_TYPE, 1, 8, PEACH_BLOCK_RANDOM },
    /* Between the SS and 3B fielders, mirrored. */
    { -14.2f, BURIED_HALFWAY_Y, 28.4f, PARALLEL_3B_2B, EXTRA_BLOCK_TYPE, 1, 0, PEACH_BLOCK_RANDOM },
    /* 3 m directly behind 2nd base. */
    { 0.0f, BEHIND_2B_Y, 41.7f, 0.0f, EXTRA_BLOCK_TYPE, 1, 8, PEACH_BLOCK_RANDOM },
    /* Between the 2B fielder and 2nd base, line-drive height. */
    { 8.5f, LINE_DRIVE_Y, 40.4f, PARALLEL_1B_2B, EXTRA_BLOCK_TYPE, 1, 8, PEACH_BLOCK_RANDOM },
    /* Between the SS and 2nd base, mirrored. */
    { -8.5f, LINE_DRIVE_Y, 40.4f, PARALLEL_3B_2B, EXTRA_BLOCK_TYPE, 1, 0, PEACH_BLOCK_RANDOM },
};
#define EXTRA_BLOCK_COUNT (sizeof(EXTRA_BLOCKS) / sizeof(EXTRA_BLOCKS[0]))
_Static_assert(VANILLA_BLOCKS + EXTRA_BLOCK_COUNT <= MAX_BLOCKS, "too many Peach blocks");

/* loadPeachGarden+0x4 (`mflr r0`): runs on every Peach Garden load, after
   game.rel has been reloaded with a stock table and before the blocks are
   created. The wrapper restores r3 (files), so the loader is unaffected.
   The new blocks go in right after the vanilla 16; the DEBRIS and SHADOW
   markers move down behind them, since the loader counts blocks up to the
   first DEBRIS entry. */
CGECKO(MorePeachBlocks, .address = 0x80739624, .state = MSSB_GAME,
       .instruction = "mflr r0",
       .notes = "Adds 5 note blocks to Peach Garden: two half\n"
                "buried behind the 1B-2B and 3B-2B baselines,\n"
                "one 3 m behind 2nd base, and two at line-drive\n"
                "height on either side of 2nd base.");
void MorePeachBlocks(void)
{
    int i = VANILLA_BLOCKS;

    for (u32 k = 0; k < EXTRA_BLOCK_COUNT; k++)
        g_PeachBlockTable[i++] = EXTRA_BLOCKS[k];

    for (int k = 0; k < DEBRIS_ENTRIES; k++)
        g_PeachBlockTable[i++] = (StadiumObjectPlacement){
            0.0f, 0.0f, 0.0f, 0.0f, PEACH_BLOCK_DEBRIS, 1, 9, PEACH_BLOCK_RANDOM };

    g_PeachBlockTable[i] = (StadiumObjectPlacement){
        0.0f, 0.0f, 0.0f, 0.0f, PEACH_BLOCK_SHADOW, 1, 9, PEACH_BLOCK_RANDOM };
}
