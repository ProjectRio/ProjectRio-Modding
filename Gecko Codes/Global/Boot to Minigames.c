/*###########################################################
# Boot to Minigames
###########################################################*/
// Author: LittleCoaks, Roeming
#include "Include/Rio/BootScene.h"

CGECKO(BootToMinigames, .address = BOOT_SCENE_DISPATCH, .state = MSSB_BOOT,
       .notes = "Game boots to the Minigames menu.\nUse only one Boot To code at a time.");
void BootToMinigames(void)
{
    BootScene_EnterGameMode(GAME_TYPE_MINIGAMES);
}
