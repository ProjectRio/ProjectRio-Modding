/*###########################################################
# Random Star Pitch
###########################################################*/
// Author: UnclePunch, PeacockSlayer, LittleCoaks
#include "Include/game/UnknownHomes_Game.h"
#include "Include/Rio/TimebaseRandom.h"

#define STAR_PITCH_COUNT "13"

/* ASM: r0 carries nonCaptainStarPitch across the site. */
ASM(RandomStarPitch,
    TIMEBASE_RANDOM_R15(STAR_PITCH_COUNT)
    "addi   15, 15, 1                 \n"
    "stb    15, 0x147(3)              \n"   /* g_Pitcher.captainStarPitch */
    TIMEBASE_RANDOM_CLEAR,
    .address = 0x806ADF98, .state = MSSB_GAME,
    .notes = "Star pitches are randomized.");
