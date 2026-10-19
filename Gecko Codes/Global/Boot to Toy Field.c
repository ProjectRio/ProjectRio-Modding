/*###########################################################
# Boot to Toy Field
###########################################################*/
// Author: LittleCoaks, Roeming
#include "Include/Rio/BootScene.h"

CGECKO(BootToToyField, .address = BOOT_SCENE_DISPATCH, .state = MSSB_BOOT,
       .notes = "Game boots to the Toy Field menu.\nUse only one Boot To code at a time.");
void BootToToyField(void)
{
    BootScene_EnterGameMode(GAME_TYPE_TOY_FIELD);
}
