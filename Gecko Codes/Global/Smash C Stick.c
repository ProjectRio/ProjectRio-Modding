/*###########################################################
# Smash C-Stick
###########################################################*/
// Author: LittleCoaks
#include "Include/types.h"

// The raw pad word SIGetResponse copies out: <ABXY><LRZ><stickX><stickY>
#define RAW_BUTTON_A     0x01000000
#define RAW_STICK_X_MASK 0x0000FF00
#define RAW_STICK_Y_MASK 0x000000FF

#define SI_CSTICK_X      0x1C4
#define SI_CSTICK_Y      0x1C5
#define CSTICK_LOW       0x40
#define CSTICK_HIGH      0xB0

// Hooked one instruction after the pad word is stored to the caller's buffer
// (r30), where r0 is dead; the store site itself holds the word in r0.
CGECKO(SmashCStick, .address = 0x800A5A44, .instruction = "lwz r0, 0x1C4(r3)",
       .notes = "Flicking the C-stick in a direction acts like holding the control stick\n"
                "that way and pressing A at the same time.");
void SmashCStick(void)
{
    register unsigned int _sp __asm__("r30");
    u8*  si  = *(volatile u8**) (_sp + 0x8 + ((3  - 3) << 2));
    u32* out = *(volatile u32**)(_sp + 0x8 + ((30 - 3) << 2));

    u32 pad = *out;
    u8 cx = si[SI_CSTICK_X];
    u8 cy = si[SI_CSTICK_Y];

    if (cx <= CSTICK_LOW)
        pad = (pad & ~RAW_STICK_X_MASK) | RAW_BUTTON_A;
    else if (cx >= CSTICK_HIGH)
        pad |= RAW_STICK_X_MASK | RAW_BUTTON_A;

    if (cy <= CSTICK_LOW)
        pad = (pad & ~RAW_STICK_Y_MASK) | RAW_BUTTON_A;
    else if (cy >= CSTICK_HIGH)
        pad |= RAW_STICK_Y_MASK | RAW_BUTTON_A;

    *out = pad;
}
