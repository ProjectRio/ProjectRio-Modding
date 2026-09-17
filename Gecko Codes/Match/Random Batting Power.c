/*###########################################################
# Random Batting Power
###########################################################*/
// Author: UnclePunch, PeacockSlayer, LittleCoaks
// Hook site and the skipped second store: docs/import_match_gameplay.md
#include "Include/game/UnknownHomes_Game.h"
#include "Include/game/math/game_math.h"

#define MAX_POWER 160
#define NOP       0x60000000
#define STB_R5_POWER1 0x98A40080

CGECKO(RandomBattingPower, .address = 0x806536D8, .state = MSSB_GAME,
       .notes = "Every batter's power is randomized, from 1 to 160.");
void RandomBattingPower(void)
{
    PatchInstruction_Conditional(0x80653724, STB_R5_POWER1, NOP);
    g_Batter.hitPower_capped[0] = RandomInt_Game(MAX_POWER) + 1;
    g_Batter.hitPower_capped[1] = RandomInt_Game(MAX_POWER) + 1;
}
