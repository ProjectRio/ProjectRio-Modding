/*###########################################################
# Always 0 outs, 0 balls, and 0 strikes
###########################################################*/
// Author: PeacockSlayer, LittleCoaks
#include "Include/game/UnknownHomes_Game.h"

CGECKO(AlwaysZeroCount,
       .notes = "A training mod: the count never moves, so the\n"
                "top of the 1st inning never ends.");
void AlwaysZeroCount(void)
{
    g_Strikes.strikes = 0;
    g_Strikes.outs = 0;
    g_Strikes.balls = 0;
}
