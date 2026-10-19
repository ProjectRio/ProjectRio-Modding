/*###########################################################
# Auto Dingers - hold Z
###########################################################*/
// Author: PeacockSlayer, LittleCoaks
#include "Include/game/UnknownHomes_Game.h"

/* AtBat_ButtonInput1: held buttons, one u16 per port at stride 0x20 */
#define HeldButtons(port) VAR_ADDRESS(u16, 0x803C77B8 + (port) * 0x20)
#define PAD_Z_AND_A 0x0110
#define MAX_POWER   0xFF

CGECKO(AutoDingers, .state = MSSB_GAME,
       .notes = "Hold Z and A while the batter hits the ball and it is hit at max power.");
void AutoDingers(void)
{
    int port;
    for (port = 0; port < 4; port++)
    {
        if (HeldButtons(port) != PAD_Z_AND_A)
            continue;
        g_Batter.hitPower_capped[0] = MAX_POWER;
        g_Batter.hitPower_capped[1] = MAX_POWER;
    }
}
