/*###########################################################
# Hold Up for Eephus
###########################################################*/
// Author: LittleCoaks
// The two pitch-speed reads and the input byte: docs/import_match_gameplay.md
#include "Include/game/UnknownHomes_Game.h"

#define EEPHUS_SPEED "0x41"

/* g_FieldingLogic.fielderInputs, low byte: 8 = stick up */
#define SKIP_UNLESS_STICK_UP          \
    "lis   14, 0x8089             \n" \
    "ori   14, 14, 0x2899         \n" \
    "lbz   14, 0(14)              \n" \
    "cmpwi 14, 8                  \n" \
    "blt   1f                     \n"

/* ASM: the speed comes back in r0. */
ASM(HoldUpForEephus_Curve,
    "lbz   0, 0x142(31)           \n"   /* curveBallSpeed, the instruction we replace */
    SKIP_UNLESS_STICK_UP
    "li    0, " EEPHUS_SPEED     "\n"
    "1:                           \n",
    .address = 0x806B1F30, .state = MSSB_GAME,
    .notes = "Hold up on the control stick as the pitcher to lob the ball high in the air.");

/* ASM: r0 holds half of a float conversion across this site. */
ASM(HoldUpForEephus_Fast,
    "lbz   5, 0x143(6)            \n"   /* fastBallSpeed, the instruction we replace */
    SKIP_UNLESS_STICK_UP
    "li    5, " EEPHUS_SPEED     "\n"
    "1:                           \n",
    .address = 0x806B1E40, .state = MSSB_GAME);
