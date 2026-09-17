/*###########################################################
# Reset Stars in 13th Inning
###########################################################*/
// Author: DannyBoy, LittleCoaks
#include "Include/game/UnknownHomes_Game.h"
#include "Include/static/UnknownHomes_Static.h"

#define RESET_INNING 13

/* inningChange: replaces `stw r0, 0(r31)`, the store of the new inning (r3 + 1).
   No .instruction: the C wrapper cannot keep r0, so the store is done here. */
CGECKO(ResetStarsIn13thInning, .address = 0x8069C6E4, .state = MSSB_GAME,
       .notes = "When the 13th inning starts, both players go back\n"
                "to the star count they started the game with.\n"
                "Meant to be used with Unlimited Extra Innings.");
void ResetStarsIn13thInning(void)
{
    READ_GAME_REG(s32, finishedInning, 3);

    g_Scores.Inning = finishedInning + 1;
    if ((u8)g_Scores.Inning != RESET_INNING)
        return;

    g_GameLogic.TeamStars[0] = Static_Stats_Tables.startingChemStars[0];
    g_GameLogic.TeamStars[1] = Static_Stats_Tables.startingChemStars[1];
}
