/*###########################################################
# Skip Memory Card Check
###########################################################*/
// Author: LittleCoaks
#include "Include/static/UnknownHomes_Static.h"
#include "Include/Symbols/dol.h"

#define cardCheckDone VAR_ADDRESS(u8, cardCheck_ADDR)

CGECKO(SkipMemoryCardCheck,
       .notes = "No memory card data is loaded.");
void SkipMemoryCardCheck(void)
{
    cardCheckDone = 1;
}
