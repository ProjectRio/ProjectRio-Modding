/*###########################################################
# More Star Chances
###########################################################*/
// Author: LittleCoaks

#define STAR_CHANCE_PERCENT 75

#define STAR_CHANCE_ROLL_COMPARE 0x8069E788   /* matchTransitionPrepareNextAB */
#define CMPW_R3_R0               0x7C030000
#define CMPWI_R3(n)              (0x2C030000 | (n))

CGECKO(MoreStarChances, .state = MSSB_GAME,
       .notes = "Raises the odds of a star chance to 75%.\n"
                "As usual, no star chance happens with runners on base.");
void MoreStarChances(void)
{
    PatchInstruction_Conditional(STAR_CHANCE_ROLL_COMPARE, CMPW_R3_R0, CMPWI_R3(STAR_CHANCE_PERCENT));
}
