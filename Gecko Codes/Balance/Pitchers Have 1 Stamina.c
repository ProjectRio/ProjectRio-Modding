/*###########################################################
# Pitchers Have 1 Stamina
###########################################################*/
// Author: LittleCoaks
#include "Gecko Codes/Balance/PitcherStamina.h"

#define STARTING_STAMINA 4

CGECKO(PitchersHave1Stamina, .state = MSSB_GAME,
       .notes = "Pitchers get tired after 1 run or star pitch, instead of 7.");
void PitchersHave1Stamina(void)
{
    PatchInstruction_Conditional(INIT_STATS_LOAD_STAMINA, CLRLWI_R7_R3_16, LI_R7(STARTING_STAMINA));
}
