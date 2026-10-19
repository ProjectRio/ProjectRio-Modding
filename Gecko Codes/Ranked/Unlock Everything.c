/*###########################################################
# Unlock Everything
###########################################################*/
// Author: PeacockSlayer, LittleCoaks
#include "Include/static/UnknownHomes_Static.h"
#include "Include/Symbols/dol.h"

// Save-data unlock flags in the DOL. Only superstarUnlocked has a decomp name so far.
#define GAME_SETTINGS_UNLOCK_12   0x800E870E   /* g_d_GameSettings + 0x12       */
#define GAME_SETTINGS_UNLOCK_14   0x800E8710   /* g_d_GameSettings + 0x14..0x19 */
#define GAME_SETTINGS_UNLOCK_1A   0x800E8716   /* g_d_GameSettings + 0x1A..0x1F */
#define UNLOCK_FLAGS_80361680     0x80361680
#define UNLOCK_FLAG_803616B0      0x803616B0
#define UNLOCK_FLAGS_80361C04     0x80361C04   /* superstarUnlocked + 0xE4 */
#define UNLOCK_FLAGS_80361C14     0x80361C14   /* superstarUnlocked + 0xF4 */

#define NUM_CHARACTERS 54

static const struct UnlockFill { u32 addr; u8 count; u8 value; } UNLOCKS[] = {
    { GAME_SETTINGS_UNLOCK_12,  1,    2 },
    { GAME_SETTINGS_UNLOCK_14,  6,    3 },
    { GAME_SETTINGS_UNLOCK_1A,  6,    1 },
    { UNLOCK_FLAGS_80361680,    0x2A, 1 },
    { UNLOCK_FLAG_803616B0,     1,    1 },
    { superstarUnlocked_ADDR,   NUM_CHARACTERS, 1 },
    { UNLOCK_FLAGS_80361C04,    4,    1 },
    { UNLOCK_FLAGS_80361C14,    2,    1 },
};

CGECKO(UnlockEverything,
       .notes = "All minigames, stadiums, characters, and star characters are unlocked.");
void UnlockEverything(void)
{
    for (int i = 0; i < (int)LEN(UNLOCKS); i++)
    {
        volatile u8* p = (volatile u8*)UNLOCKS[i].addr;
        for (int b = 0; b < UNLOCKS[i].count; b++)
            p[b] = UNLOCKS[i].value;
    }
}
