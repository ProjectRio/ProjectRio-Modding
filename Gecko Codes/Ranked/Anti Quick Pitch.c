/*###########################################################
# Anti Quick Pitch
###########################################################*/
// Author: PeacockSlayer, LittleCoaks
#include "Include/game/UnknownHomes_Game.h"

// ASM: replaces the pitcher's button read, whose result must come back in r0.
ASM(AntiQuickPitch,
    "lis   14, 0x8089        \n"
    "ori   14, 14, 0x80DE    \n"   /* g_Minigame.GameMode_MiniGame */
    "lbz   14, 0(14)         \n"
    "cmpwi 14, 0             \n"
    "bne   1f                \n"   /* minigames read input normally */
    "li    0, 0              \n"
    "lis   14, 0x8089        \n"
    "ori   14, 14, 0x099D    \n"   /* g_Batter.swingInd */
    "lbz   14, 0(14)         \n"
    "cmpwi 14, 1             \n"
    "beq   2f                \n"   /* batter still getting set: no input */
    "1:                      \n"
    "lhz   0, 6(30)          \n"   /* the instruction we replace */
    "2:                      \n"
    "nop                     \n",
    .address = 0x806B406C, .state = MSSB_GAME,
    .notes = "Stops the pitcher from starting a pitch while the batter is still getting set.");
