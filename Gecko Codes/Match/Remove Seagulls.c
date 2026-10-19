/*###########################################################
# Remove Seagulls
###########################################################*/
// Author: nuche
// The store this drops: docs/match_codes.md

#define STW_R5_1C_R6 0x90A6001C

CGECKO(RemoveSeagulls, .state = MSSB_GAME,
       .notes = "Removes the seagulls from the stadiums.");
void RemoveSeagulls(void)
{
    PatchInstruction_Conditional(0x80708E90, STW_R5_1C_R6, 0x60000000);   /* loadMarioStadium */
}
