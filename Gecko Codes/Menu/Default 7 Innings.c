/*###########################################################
# Default 7 Innings
###########################################################*/
// Author: LittleCoaks
#include "Include/Rio/GameSettingsDefaults.h"

CGECKO(Default7Innings, .notes = "Game settings default to 7 innings.");
void Default7Innings(void)
{
    GameSettings_SetDefaultInnings(INNINGS_7);
}
