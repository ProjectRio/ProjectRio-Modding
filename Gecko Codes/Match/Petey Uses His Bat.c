/*###########################################################
# Petey Uses His Bat
###########################################################*/
// Author: LittleCoaks
// Why Petey's bat never shows and what this forces: docs/petey_bat.md

#define CHAR_ID_PETEY_IMM "0x26"

/* updateSpecialBatAttach: r28 = player, r29 = hand (0 = the leaf on the batting side), r3 = the hand-track flags;
   force the show-bat flag on that leaf for Petey. */
ASM(PeteyUsesHisBat,
    "lbz    0, 0x252(28)            \n"
    "cmpwi  0, " CHAR_ID_PETEY_IMM " \n"
    "bne    1f                      \n"
    "cmpwi  29, 0                   \n"
    "bne    1f                      \n"
    "oris   3, 3, 0x2000            \n"
    "1:                             \n"
    "rlwinm. 0, 3, 0, 2, 2          \n",      /* the instruction we replace */
    .address = 0x8004BCEC,
    .notes = "Petey Piranha swings a real bat, using a bat model the game has but never shows.");
