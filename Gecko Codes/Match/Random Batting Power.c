/*###########################################################
# Random Batting Power
###########################################################*/
// Author: UnclePunch, PeacockSlayer, LittleCoaks
#include "Include/game/UnknownHomes_Game.h"
#include "Include/Rio/TimebaseRandom.h"

#define MAX_POWER "160"

/* Both replace a store of hitPower_capped[0] in setDefaultInMemBatter. */
ASM(RandomBattingPower_A,
    TIMEBASE_RANDOM_R15(MAX_POWER)
    "addi   15, 15, 1                 \n"
    "stb    15, 0x80(4)               \n"
    TIMEBASE_RANDOM_CLEAR,
    .address = 0x80653724, .state = MSSB_GAME,
    .notes = "Every batter's power is randomized, from 1 to 160.");

ASM(RandomBattingPower_B,
    TIMEBASE_RANDOM_R15(MAX_POWER)
    "addi   15, 15, 1                 \n"
    "stb    15, 0x80(3)               \n"
    TIMEBASE_RANDOM_CLEAR,
    .address = 0x806536D8, .state = MSSB_GAME);
