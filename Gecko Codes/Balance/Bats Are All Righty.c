/*###########################################################
# Bats Are All Righty
###########################################################*/
// Author: Roeming
#include "Gecko Codes/Balance/BatHandedness.h"

CGECKO(BatsAreAllRighty, .state = MSSB_GAME,
       .notes = "Lefty and righty batters do not have matching contact zones in the original game:\n"
                "the lefty bat is mirrored incorrectly.\n"
                "This makes every batter use the correct righty contact zones, whichever side they bat from.");
void BatsAreAllRighty(void)
{
    PatchInstruction_Conditional(MIRROR_CONTACT_FOR_LEFTY, FNEG_F9, NOP);
}
