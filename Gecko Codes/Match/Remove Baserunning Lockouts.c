/*###########################################################
# Remove Baserunning Lockouts
###########################################################*/
// Author: nuche17
// The lockout table and how this relates to Rio's built-in version: docs/import_match_rules.md
#include "Include/types.h"

/* game.rel .data lbl_3_data_4C54[3]; not in the decomp headers yet */
#define runnerLockoutFrames VAR_ADDRESS(s16, 0x807B625A)

CGECKO(RemoveBaserunningLockouts, .state = MSSB_GAME,
       .notes = "Removes the lockout that stops you controlling\n"
                "baserunners right after contact.");
void RemoveBaserunningLockouts(void)
{
    runnerLockoutFrames = 1;
}
