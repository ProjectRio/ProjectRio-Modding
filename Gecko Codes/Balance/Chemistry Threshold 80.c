/*###########################################################
# Chemistry Threshold 80
###########################################################*/
// Author: Clutch1908
#include "Include/mssbTypes.h"
#include "Include/Symbols/game.h"

#define GOOD_CHEMISTRY_THRESHOLD 80   /* stock 90 */

#define chemThresholds ARRAY_1D_ADDRESS(s16, 4, chemThresholds_ADDR)

CGECKO(ChemistryThreshold80, .state = MSSB_GAME,
       .notes = "Lowers the chemistry value two characters need for good chemistry from 90 to 80.");
void ChemistryThreshold80(void)
{
    chemThresholds[0] = GOOD_CHEMISTRY_THRESHOLD;
}
