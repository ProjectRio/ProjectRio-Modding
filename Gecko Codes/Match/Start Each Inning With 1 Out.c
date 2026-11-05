/*###########################################################
# Start Each Inning With 1 Out
###########################################################*/
// Author: Nuche17
// Why resetCount is the hook site: docs/match_codes.md
#include "Include/game/UnknownHomes_Game.h"
#include "Include/static/UnknownHomes_Static.h"

#define STARTING_OUTS 1

/* resetCount+0x14: replaces `stw r0, 0x10(r3)` (forcedOutToEndInningInd = 0),
   the last store before its blr. No .instruction: that store is done here, and
   nothing after it reads r0 or r3. Strikes, balls and outs are already 0. */
CGECKO(StartEachInningWith1Out, .address = 0x8069972C, .state = MSSB_GAME,
       .notes = "Every half-inning starts with 1 out, so each\n"
                "side only needs 2 more outs to end the inning.");
void StartEachInningWith1Out(void)
{
    // Replace hook instruction.
    g_Strikes.forcedOutToEndInningInd = 0;

    /* Free fielding practice also calls resetCount. */
    if (g_d_GameSettings.GameModeSelected == GAME_TYPE_PRACTICE)
        return;

    g_Strikes.outs = STARTING_OUTS;
}
