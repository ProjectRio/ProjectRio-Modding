/*###########################################################
# No Batter Pausing
###########################################################*/
// Author: LittleCoaks

#define BATTER_PAUSE_INPUT_LOAD 0x806EED5C   /* match_checkForPause */

CGECKO(NoBatterPausing, .state = MSSB_GAME,
       .notes = "The batter cannot pause the game.");
void NoBatterPausing(void)
{
    PatchInstruction_Conditional(BATTER_PAUSE_INPUT_LOAD, 0xA0040006, 0x38000000);   // lhz r0, 6(r4) -> li r0, 0
}
