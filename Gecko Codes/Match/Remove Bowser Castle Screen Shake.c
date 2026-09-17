/*###########################################################
# Remove Bowser Castle Screen Shake
###########################################################*/
// Author: MORI

#define SHAKE_DURATION_LOAD 0x80702B34   /* thwomp_screenShake */

CGECKO(RemoveBowserCastleScreenShake, .state = MSSB_GAME,
       .notes = "Removes the screen shake caused by the thwomps on Bowser's Castle.");
void RemoveBowserCastleScreenShake(void)
{
    PatchInstruction_Conditional(SHAKE_DURATION_LOAD, 0x3880003C, 0x38800000);   // li r4, 60 -> li r4, 0
}
