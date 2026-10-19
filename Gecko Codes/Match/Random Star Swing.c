/*###########################################################
# Random Star Swing
###########################################################*/
// Author: UnclePunch, PeacockSlayer, LittleCoaks
#include "Include/game/UnknownHomes_Game.h"
#include "Include/game/math/game_math.h"

/* Replaces the stat-table load whose value the game stores as g_Batter.captainStarHitPitch. */
CGECKO(RandomStarSwing, .address = 0x806AD0B0, .state = MSSB_GAME,
       .notes = "Star swings are randomized.");
void RandomStarSwing(void)
{
    WRITE_GAME_REG(8, RandomInt_Game(CAPTAIN_STAR_TYPE_DAISY) + CAPTAIN_STAR_TYPE_MARIO);
}
