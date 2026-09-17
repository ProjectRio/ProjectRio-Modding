/*###########################################################
# Default 3 Innings
###########################################################*/
// Author: LittleCoaks
#include "Include/Rio/GameSettingsDefaults.h"

CGECKO(Default3Innings, .notes = "Game settings default to 3 innings.");
void Default3Innings(void)
{
    GameSettings_SetDefaultInnings(INNINGS_3);
}
