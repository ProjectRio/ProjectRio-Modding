/*###########################################################
# Dash Glitch Fix
###########################################################*/
// Author: MORI
#include "Include/game/UnknownHomes_Game.h"

#define NO_DASHING_FIELDER 0xFF

CGECKO(DashGlitchFix, .state = MSSB_GAME,
       .notes = "Fixes the glitch where the selected fielder is unable to dash.");
void DashGlitchFix(void)
{
    FielderDash* dash = (FielderDash*)g_FieldingLogic.fielderDashByPort;

    if (g_GameLogic.EventTriggers_GameHasStarted != 1 || dash->sprintingState == 0)
        return;

    if (dash->dashingFielderIndex != g_FieldingLogic.selectedFielder &&
        dash->dashingFielderIndex != NO_DASHING_FIELDER)
        dash->dashingFielderIndex = g_FieldingLogic.selectedFielder;
}
