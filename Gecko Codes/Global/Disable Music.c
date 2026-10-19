/*###########################################################
# Disable Music
###########################################################*/
// Author: LittleCoaks

#define LI_R0_0 0x38000000

CGECKO(DisableMusic, .notes = "Turns off all music.");
void DisableMusic(void)
{
    PatchInstruction(0x80062AB0, LI_R0_0);                           // lbz r0, 0x398(r3)
    PatchInstruction_Conditional(0x806CCCB0, 0x88030398, LI_R0_0);   // lbz r0, 0x398(r3), game.rel
}
