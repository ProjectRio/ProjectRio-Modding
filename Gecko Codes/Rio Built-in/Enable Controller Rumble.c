/*###########################################################
# Enable Controller Rumble
###########################################################*/
// Author: LittleCoaks
#include "Include/types.h"

// DOL .bss byte (decomp: lbl_80366158 + 0x1F, not yet named); non-zero = rumble on
#define RumbleEnabled VAR_ADDRESS(u8, 0x80366177)

CGECKO(EnableControllerRumble,
       .notes = "Turns controller rumble on.");
void EnableControllerRumble(void)
{
    RumbleEnabled = 1;
}
