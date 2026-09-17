/*###########################################################
# Allow Freecam From External Programs
###########################################################*/
// Author: Roeming
// Which word toggles the camera skip and what each patch site does: docs/freecam.md
#include "Include/types.h"
#include "Include/Rio/PatchTable.h"

// The external tool writes 1 here to take over the camera (see docs/freecam.md about the claim).
#define FreecamRequested (VAR_ADDRESS(u32, 0x802EC020) == 1)

// Skip the game's camera update while the tool drives the camera
static const RioPatch CAMERA_UPDATE_SKIP[] = {
    { 0x80052988, 0x800D8140, 0x480003A0 },
};

// Always on: never cull characters or stadium hazards, force the visibility flags
static const RioPatch VISIBILITY_PATCHES[] = {
    { 0x8001DCF8, 0x7C001B78, 0x38000007 },
    { 0x806AA4E4, 0x38000002, 0x38000001 },
    { 0x806AB8B4, 0x38000000, 0x38000001 },
    { 0x800BD9DC, 0x7C831B78, 0x38600007 },
    { 0x806F7B7C, 0x881A0093, 0x38000003 },
};

CGECKO(AllowFreecamFromExternalPrograms,
       .notes = "To be used with Roeming's hitbox visualization tools.");
void AllowFreecamFromExternalPrograms(void)
{
    RioPatch_Apply(CAMERA_UPDATE_SKIP, RIO_PATCH_COUNT(CAMERA_UPDATE_SKIP), FreecamRequested);
    RioPatch_Apply(VISIBILITY_PATCHES, RIO_PATCH_COUNT(VISIBILITY_PATCHES), true);
}
