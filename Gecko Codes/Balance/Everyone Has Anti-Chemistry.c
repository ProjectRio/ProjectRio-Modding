/*###########################################################
# Everyone Has Anti-Chemistry
###########################################################*/
// Author: PeacockSlayer, LittleCoaks
#include "Include/Rio/StatEdits.h"

#define ANTI_CHEMISTRY 1

CGECKO(EveryoneHasAntiChemistry,
       .notes = "Every character has bad chemistry with every other character, so every team is a 1-star team.");
void EveryoneHasAntiChemistry(void)
{
    FillAllChemistry(ANTI_CHEMISTRY);
}
