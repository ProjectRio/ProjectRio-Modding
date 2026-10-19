/*###########################################################
# Boot to Practice Mode
###########################################################*/
// Author: LittleCoaks, Roeming
#include "Include/Rio/BootScene.h"

CGECKO(BootToPracticeMode, .address = BOOT_SCENE_DISPATCH, .state = MSSB_BOOT,
       .notes = "Game boots to the Practice menu.\nUse only one Boot To code at a time.");
void BootToPracticeMode(void)
{
    BootScene_EnterGameMode(GAME_TYPE_PRACTICE);
}
