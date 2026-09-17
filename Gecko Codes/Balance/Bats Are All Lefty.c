/*###########################################################
# Bats Are All Lefty
###########################################################*/
// Author: Roeming
#include "Gecko Codes/Balance/BatHandedness.h"

CGECKO(BatsAreAllLefty, .state = MSSB_GAME,
       .notes = "Lefty and righty batters do not have matching contact zones in the original game:\n"
                "the lefty bat is mirrored incorrectly.\n"
                "This makes every batter use the lefty contact zones, whichever side they bat from.");
void BatsAreAllLefty(void)
{
    PatchInstruction_Conditional(SKIP_MIRROR_FOR_RIGHTY, BEQ_SKIP_MIRROR, NOP);
}
