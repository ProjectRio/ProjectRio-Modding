/*###########################################################
# Control Stick Overrides DPad
###########################################################*/
// Author: LittleCoaks
// Which PADRead word this edits and why the hook sits one instruction late: docs/control_stick_dpad.md
#include "Include/types.h"

#define PAD_WORD_OFFSET     0x1C0
#define DPAD_BITS           0x000F0000
#define STICK_LOW           0x52
#define STICK_HIGH          0xAE

static bool StickPushed(u8 axis)
{
    return axis <= STICK_LOW || axis >= STICK_HIGH;
}

// The frame after `stw r0, 0x1C0(r5)`: the buttons/stick word is in memory and r0 is dead.
CGECKO(ControlStickOverridesDPad, .address = 0x800A5A00, .instruction = "slwi r0, r29, 2",
       .notes = "While the control stick is pushed, D-pad input is ignored.");
void ControlStickOverridesDPad(void)
{
    READ_GAME_REG(u8*, pad, 5);
    u32* buttonsAndStick = (u32*)(pad + PAD_WORD_OFFSET);

    u32 word   = *buttonsAndStick;
    u8  stickX = (u8)(word >> 8);
    u8  stickY = (u8)word;

    if (StickPushed(stickX) || StickPushed(stickY))
        *buttonsAndStick = word & ~DPAD_BITS;
}
