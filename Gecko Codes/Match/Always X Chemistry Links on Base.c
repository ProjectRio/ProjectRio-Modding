/*###########################################################
# Always X Chemistry Links on Base
###########################################################*/
// Author: PeacockSlayer
#include "Include/game/UnknownHomes_Game.h"

#define CHEM_LINKS_ON_BASE 3   /* 0-3 */

CGECKO(AlwaysXChemistryLinksOnBase,
       .notes = "The batter always hits as if this many chemistry\n"
                "teammates were on base (3 by default, 0-3).");
void AlwaysXChemistryLinksOnBase(void)
{
    g_Batter.chemLinksOnBase = CHEM_LINKS_ON_BASE;
}
