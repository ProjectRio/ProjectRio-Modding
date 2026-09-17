/*###########################################################
# Skip Memory Card Check on Main Menu
###########################################################*/
// Author: LittleCoaks
#include "Include/types.h"
#include "Include/Symbols/dol.h"

// First byte of cardCheck: non-zero = the main-menu card check already ran
#define CardCheckDone VAR_ADDRESS(u8, cardCheck_ADDR)

CGECKO(SkipMemoryCardCheckOnMainMenu,
       .notes = "Skips the memory card check when returning to the main menu.");
void SkipMemoryCardCheckOnMainMenu(void)
{
    CardCheckDone = 1;
}
