/*###########################################################
# Always 0 outs, 0 balls, and 0 strikes
###########################################################*/
// Author: PeacockSlayer, LittleCoaks
// The original only cleared the low byte of each count: docs/import_match_rules.md
#include "Include/game/UnknownHomes_Game.h"

#define LOW_BYTE(field) (((volatile u8*)&(field))[3])

CGECKO(AlwaysZeroCount,
       .notes = "A training mod: the count never moves, so the\n"
                "top of the 1st inning never ends.");
void AlwaysZeroCount(void)
{
    LOW_BYTE(g_Strikes.strikes) = 0;
    LOW_BYTE(g_Strikes.outs)    = 0;
    LOW_BYTE(g_Strikes.balls)   = 0;
}
