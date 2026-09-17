/*###########################################################
# Never Cull Stage Hazards
###########################################################*/
// Author: Roeming
// What the site decides: docs/widescreen.md

CGECKO(NeverCullStageHazards, .notes = "Stadium hazards are always drawn, even when off screen.");
void NeverCullStageHazards(void)
{
    PatchInstruction_Conditional(0x806F7B7C, 0x881A0093, 0x38000003);  // lbz r0, 0x93(r26) -> li r0, 3
}
