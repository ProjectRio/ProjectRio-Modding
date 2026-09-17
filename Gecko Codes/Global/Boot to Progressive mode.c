/*###########################################################
# Boot to Progressive mode
###########################################################*/
// Author: LittleCoaks
#include "Include/Rio/BootScene.h"

CGECKO(BootToProgressiveMode,
       .notes = "Starting the game prompts you to load it in progressive mode, which changes\n"
                "the way graphics are rendered.\nUse only one Boot To code at a time.");
void BootToProgressiveMode(void)
{
    BootScene_Force(SCENE_PROGRESSIVE_PROMPT);
}
