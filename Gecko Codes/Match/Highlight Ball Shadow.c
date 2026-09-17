/*###########################################################
# Highlight Ball Shadow
###########################################################*/
// Author: LittleCoaks
// Why the hook drops the game's early return and what the two lfs patches read: docs/match_codes.md
#include "Include/game/UnknownHomes_Game.h"

#define LFS_F0_0_R29  0xC01D0000
#define LFS_F0_8_R5   0xC0050008

/* Replaces `beq <return>`: with the drop spot switched off the marker keeps
   drawing, now fed by the ball's own position. */
CGECKO(HighlightBallShadow, .address = 0x806A844C, .state = MSSB_GAME,
       .notes = "Makes the ball's shadow stand out so it is easier to track.");
void HighlightBallShadow(void)
{
    READ_GAME_REG(u32, dropSpotShown, 4);
    if (dropSpotShown)
        return;

    PatchInstruction(0x806A85B8, LFS_F0_0_R29);
    PatchInstruction(0x806A85D4, LFS_F0_8_R5);
}
