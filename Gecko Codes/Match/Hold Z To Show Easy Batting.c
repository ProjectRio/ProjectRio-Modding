/*###########################################################
# Hold Z To Show Easy Batting
###########################################################*/
// Author: Roeming
// The replaced compare and why r4 carries the answer: docs/match_codes.md
#include "Include/game/UnknownHomes_Game.h"

/* The synced header types g_Controls as a pointer; the game keeps four InputStructs here. */
#define g_ControlsByPort ARRAY_1D_ADDRESS(InputStruct, 4, 0x80893928)

/* The game tests its easy-batting setting here (`cmplwi r0, 1`, equal = guide
   path). We answer with the batter's Z instead: equal while Z is up. */
CGECKO(HoldZToShowEasyBatting, .address = 0x806A82B0, .state = MSSB_GAME,
       .instruction = "cmplwi r4, 1",
       .notes = "Hold Z while batting to show the easy-batting guide.\n"
                "No changes to gameplay.");
void HoldZToShowEasyBatting(void)
{
    u32 port = g_GameLogic.teams[g_GameLogic.teamBatting];
    bool zHeld = (g_ControlsByPort[port].buttonInput & INPUT_TRIGGER_Z) != 0;

    WRITE_GAME_REG(4, zHeld ? 0 : 1);
}
