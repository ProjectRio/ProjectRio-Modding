/*###########################################################
# Bobble More
###########################################################*/
// Author: taukhan, PeacockSlayer, LittleCoaks
// The three "no bobble" stores this replaces, and its quirks: docs/import_match_gameplay.md
#include "Include/game/UnknownHomes_Game.h"

/* ASM: two of the sites return the value in r0, and the scratch registers are
   the ones the in-service code uses. */
#define RANDOM_BOBBLE(result, scaled, roll)                  \
    "li     " scaled ", 16                               \n" \
    "mftb   " roll "                                     \n" \
    "rlwinm " roll ", " roll ", 10, 16, 31               \n" \
    "mullw  " roll ", " roll ", " scaled "               \n" \
    "srawi  " scaled ", " roll ", 16                     \n" \
    "addze  " scaled ", " scaled "                       \n" \
    "cmpwi  " scaled ", 11                               \n" \
    "bgt    1f                                           \n" \
    "li     " result ", 0                                \n" \
    "b      2f                                           \n" \
    "1:                                                  \n" \
    "li     " result ", 3                                \n" \
    "2:                                                  \n"

ASM(BobbleMore_CatchAction, RANDOM_BOBBLE("3", "14", "15"),
    .address = 0x8066B9C0, .state = MSSB_GAME,
    .notes = "Fielders bobble the ball far more often, but not every time.\n"
             "Careful when throwing to your teammate!");

ASM(BobbleMore_GroundBall, RANDOM_BOBBLE("0", "16", "17"),
    .address = 0x80665AE4, .state = MSSB_GAME);

ASM(BobbleMore_Reset, RANDOM_BOBBLE("0", "16", "17"),
    .address = 0x80665730, .state = MSSB_GAME);
