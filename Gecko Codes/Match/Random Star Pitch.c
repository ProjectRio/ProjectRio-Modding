/*###########################################################
# Random Star Pitch
###########################################################*/
// Author: UnclePunch, PeacockSlayer, LittleCoaks
#include "Include/game/UnknownHomes_Game.h"
#include "Include/game/math/game_math.h"

#define STAR_PITCH_COUNT 13

/* Replaces the stat-table load whose value the game stores as g_Pitcher.captainStarPitch. */
CGECKO(RandomStarPitch, .address = 0x806ADF8C, .state = MSSB_GAME,
       .notes = "Star pitches are randomized.");
void RandomStarPitch(void)
{
    WRITE_GAME_REG(4, RandomInt_Game(STAR_PITCH_COUNT) + 1);
}
