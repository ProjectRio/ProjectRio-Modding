/*###########################################################
# Never Cull Characters
###########################################################*/
// Author: Roeming
// What each site decides: docs/widescreen.md

CGECKO(NeverCullCharacters, .notes = "Characters are always drawn, even when off screen.");
void NeverCullCharacters(void)
{
    PatchInstruction(0x8001DCF8, 0x38000007);                          // or r0, r0, r3 -> li r0, 7
    PatchInstruction_Conditional(0x806AA4E4, 0x38000002, 0x38000001);  // li r0, 2      -> li r0, 1
    PatchInstruction_Conditional(0x806AB8B4, 0x38000000, 0x38000001);  // li r0, 0      -> li r0, 1
}
