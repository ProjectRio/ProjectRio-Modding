/*###########################################################
# No Thwomps
###########################################################*/
// Author: Nuche17
// Why one flag removes all six thwomps: docs/match_codes.md
#include "Include/Rio/StadiumObjects.h"

/* thwompStaticValues[].type (usedFlag): 7 ends the list. */
#define THWOMP_END_OF_LIST 7

/* loadBowserCastle+0x4 (`mflr r0`): runs on every Bowser's Castle load, after
   game.rel has been reloaded with a stock table and before the thwomps are
   created. The wrapper restores r3 (files), so the loader is unaffected. */
CGECKO(NoThwomps, .address = 0x807076E8, .state = MSSB_GAME,
       .instruction = "mflr r0",
       .notes = "Bowser's Castle has no thwomps.");
void NoThwomps(void)
{
    g_ThwompPlacements[0].type = THWOMP_END_OF_LIST;
}
