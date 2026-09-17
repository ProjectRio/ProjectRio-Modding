/*###########################################################
# Default 1 Inning
###########################################################*/
// Author: LittleCoaks
#include "Include/Rio/GameSettingsDefaults.h"

CGECKO(Default1Inning, .notes = "Game settings default to 1 inning.");
void Default1Inning(void)
{
    GameSettings_SetDefaultInnings(INNINGS_1);
}
