/*###########################################################
# Default 9 Innings
###########################################################*/
// Author: LittleCoaks
#include "Include/Rio/GameSettingsDefaults.h"

CGECKO(Default9Innings, .notes = "Game settings default to 9 innings.");
void Default9Innings(void)
{
    GameSettings_SetDefaultInnings(INNINGS_9);
}
