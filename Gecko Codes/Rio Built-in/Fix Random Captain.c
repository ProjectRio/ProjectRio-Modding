/*###########################################################
# Fix Random Captain
###########################################################*/
// Author: LittleCoaks
#include "Include/types.h"

#define RANDOM_CAPTAIN_SEED_OFFSET 0x330

// Replaces `stw r0, 0(r4)` (r0 = *r4 + r3) with the same store plus a copy
// into the word the random-captain pick actually draws from.
CGECKO(FixRandomCaptain, .address = 0x8063F7C4, .state = MSSB_MENU,
       .notes = "Makes the random captain pick truly random.");
void FixRandomCaptain(void)
{
    register unsigned int _sp __asm__("r30");
    u32  added = *(volatile u32*)(_sp + 0x8 + ((3 - 3) << 2));
    u32* seed  = *(u32* volatile*)(_sp + 0x8 + ((4 - 3) << 2));

    u32 value = *seed + added;
    seed[0] = value;
    seed[RANDOM_CAPTAIN_SEED_OFFSET / sizeof(u32)] = value;
}
