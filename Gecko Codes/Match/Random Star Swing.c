/*###########################################################
# Random Star Swing
###########################################################*/
// Author: UnclePunch, PeacockSlayer, LittleCoaks
#include "Include/game/UnknownHomes_Game.h"
#include "Include/Rio/TimebaseRandom.h"

#define STAR_SWING_COUNT "13"

/* ASM: r0 carries noncaptainStarSwing across the site. */
ASM(RandomStarSwing,
    TIMEBASE_RANDOM_R15(STAR_SWING_COUNT)
    "stb    15, 0x87(4)               \n"   /* g_Batter.captainStarHitPitch */
    TIMEBASE_RANDOM_CLEAR,
    .address = 0x806AD0B8, .state = MSSB_GAME,
    .notes = "Star swings are randomized.");
