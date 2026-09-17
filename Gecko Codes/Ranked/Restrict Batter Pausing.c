/*###########################################################
# Restrict Batter Pausing
###########################################################*/
// Author: LittleCoaks
#include "Include/game/UnknownHomes_Game.h"

// ASM: replaces the batter's pause-button read, whose result must come back in r0.
ASM(RestrictBatterPausing,
    "lis   6, 0x8089         \n"
    "ori   6, 6, 0x099D      \n"   /* g_Batter.swingInd */
    "lbz   0, 0(6)           \n"
    "lis   6, 0x8089         \n"
    "ori   6, 6, 0x09AD      \n"   /* g_Batter.chargeStatus */
    "lbz   6, 0(6)           \n"
    "add   6, 0, 6           \n"
    "lhz   0, 6(4)           \n"   /* the instruction we replace */
    "cmpwi 6, 0              \n"
    "beq   1f                \n"   /* standing still: pause allowed */
    "li    0, 0              \n"
    "1:                      \n"
    "nop                     \n",
    .address = 0x806EED5C, .state = MSSB_GAME,
    .notes = "The batter can only pause while standing still in the batter's box.");
