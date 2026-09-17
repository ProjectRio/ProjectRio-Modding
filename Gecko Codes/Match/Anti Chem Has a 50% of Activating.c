/*###########################################################
# Anti Chem Has a 50% of Activating
###########################################################*/
// Author: PeacockSlayer, LittleCoaks
// How a throw rolls for bad chemistry: docs/import_match_gameplay.md
#include "Include/game/UnknownHomes_Game.h"

#define ANTI_CHEM_PERCENT 50   /* 100 = every bad-chemistry throw goes wild */

#define CMPW_R3_R31   0x7C03F800
#define CMPLWI_R3(n)  (0x28030000 | (n))

CGECKO(AntiChemChance, .state = MSSB_GAME,
       .notes = "Throws between fielders with bad chemistry go wild 50% of the time.");
void AntiChemChance(void)
{
    PatchInstruction_Conditional(0x806EAFF8, CMPW_R3_R31, CMPLWI_R3(ANTI_CHEM_PERCENT));   /* makeThrowVariables */
}
