/*###########################################################
# Z + C Stick To Increase or Decrease Your Score
###########################################################*/
// Author: Roeming
// Which score each player edits and the raw controller records: docs/match_codes.md
#include "Include/game/UnknownHomes_Game.h"

/* The game's raw SI poll results, 8 bytes per port (the synced header types
   this block as PADStatus, which is 12 bytes per port). */
typedef struct {
    u8 buttonsHi;
    u8 buttonsLo;
    s8 stickX, stickY;
    s8 cStickX;
    u8 cStickY;
    u8 triggerL, triggerR;
} RawPad;
#define g_RawPads ARRAY_1D_ADDRESS(RawPad, 4, 0x802E9F40)

/* AtBat_ButtonInput1: this frame's new presses, one u16 per port at stride 0x20 */
#define NewlyPressedButtons(port) VAR_ADDRESS(u16, 0x803C77BA + (port) * 0x20)

#define CSTICK_UP_THRESHOLD   0x90   /* 0x80 is neutral */
#define CSTICK_DOWN_THRESHOLD 0x70
#define MAX_SCORE             99

CGECKO(ZCStickScore, .address = 0x8069A15C, .state = MSSB_GAME,
       .instruction = "mflr r0",
       .notes = "Z + C-stick up adds a run to your score.\n"
                "Z + C-stick down takes one away.");
void ZCStickScore(void)
{
    u32 scoreSide = g_Scores.halfInning;   /* the batter's score; flipped for the pitcher */

    for (int i = 0; i < 2; i++, scoreSide ^= 1)
    {
        u32 player = i == 0 ? g_GameLogic.teamBatting : g_GameLogic.teamFielding;
        u32 port = g_GameLogic.teams[player];
        if (port >= 4)
            continue;
        if (!(NewlyPressedButtons(port) & PAD_TRIGGER_Z))
            continue;

        u8 cStickY = g_RawPads[port].cStickY;
        if (cStickY == 0)
            continue;

        s16* score = &g_Scores.scores[scoreSide].total;
        if (cStickY >= CSTICK_UP_THRESHOLD)
        {
            if (*score < MAX_SCORE)
                (*score)++;
        }
        else if (cStickY <= CSTICK_DOWN_THRESHOLD)
        {
            if (*score > 0)
                (*score)--;
        }
    }
}
