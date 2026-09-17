/*###########################################################
# Remove Dingus Bunt
###########################################################*/
// Author: LittleCoaks
#include "Include/game/UnknownHomes_Game.h"

// ASM: replaces the `li r0, 1` that sends the infielders charging in.
ASM(RemoveDingusBunt,
    "lis   14, 0x8089        \n"
    "ori   14, 14, 0x2899    \n"   /* g_FieldingLogic.fielderInputs, low byte */
    "lbz   0, 0(14)          \n"
    "andi. 0, 0, 0x10        \n"   /* Z held? */
    "cmpwi 0, 0              \n"
    "beq   1f                \n"   /* r0 is already 0: fielders stay put */
    "li    0, 1              \n"
    "1:                      \n"
    "nop                     \n",
    .address = 0x8069811C, .state = MSSB_GAME,
    .notes = "Infielders no longer charge in automatically on a bunt.\n"
             "The fielding player must press Z to bring them in.");
