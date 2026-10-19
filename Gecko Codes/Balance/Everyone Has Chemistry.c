/*###########################################################
# Everyone Has Chemistry
###########################################################*/
// Author: PeacockSlayer, LittleCoaks
#include "Include/game/UnknownHomes_Game.h"
#include "Include/Rio/StatEdits.h"
#include "Gecko Codes/Balance/BatHandedness.h"

#define FULL_CHEMISTRY 99

CGECKO(EveryoneHasChemistry,
       .notes = "Every character has chemistry with every other character, so every team is a 5-star team.\n"
                "Also fixes Bowser's contact zone when he bats lefty, so it matches his smaller righty one.");
void EveryoneHasChemistry(void)
{
    FillAllChemistry(FULL_CHEMISTRY);
}

CGECKO(UseRightyContactForBowser, .state = MSSB_GAME);
void UseRightyContactForBowser(void)
{
    PatchInstruction_Conditional(MIRROR_CONTACT_FOR_LEFTY, NOP, FNEG_F9);
    if (g_Batter.charID == CHAR_ID_BOWSER)
        PatchInstruction_Conditional(MIRROR_CONTACT_FOR_LEFTY, FNEG_F9, NOP);
}
