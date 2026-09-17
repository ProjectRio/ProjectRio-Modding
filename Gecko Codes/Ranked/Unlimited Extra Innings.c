/*###########################################################
# Unlimited Extra Innings
###########################################################*/
// Author: LittleCoaks
#include "Include/game/UnknownHomes_Game.h"

#define ADDI_R0_R4_3    0x38040003   /* extra innings allowed = innings + 3   */
#define ADDI_R0_R4_246  0x380400F6   /* ... = innings + 246: never runs out   */

CGECKO(UnlimitedExtraInnings, .state = MSSB_GAME,
       .notes = "Games do not end until there is a winner.");
void UnlimitedExtraInnings(void)
{
    PatchInstruction_Conditional(0x80699A10, ADDI_R0_R4_3, ADDI_R0_R4_246);
}
