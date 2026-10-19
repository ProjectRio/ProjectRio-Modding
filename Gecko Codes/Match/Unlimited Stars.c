/*###########################################################
# Unlimited Stars
###########################################################*/
// Author: PeacockSlayer, LittleCoaks
#include "Include/game/UnknownHomes_Game.h"

CGECKO(UnlimitedStars,
       .notes = "Unlimited stars for both players.");
void UnlimitedStars(void)
{
    g_GameLogic.TeamStars[0] = 0xFF;
    g_GameLogic.TeamStars[1] = 0xFF;
}
