/*###########################################################
# Noki Walljump
###########################################################*/
// Author: Clutch1908
#include "Include/Rio/StatEdits.h"

#define NOKI_ABILITIES (FIELDING_ABILITIES_WALL_JUMP | FIELDING_ABILITIES_SLIDING_CATCH)

static const StatEdit NOKI_WALLJUMP[] = {
    ABILITIES_LOW(CHAR_ID_NOKI_BLUE,  NOKI_ABILITIES),
    ABILITIES_LOW(CHAR_ID_NOKI_RED,   NOKI_ABILITIES),
    ABILITIES_LOW(CHAR_ID_NOKI_GREEN, NOKI_ABILITIES),
};

CGECKO(NokiWalljump,
       .notes = "All three Nokis can wall jump.");
void NokiWalljump(void)
{
    ApplyStatEdits(NOKI_WALLJUMP, LEN(NOKI_WALLJUMP));
}
