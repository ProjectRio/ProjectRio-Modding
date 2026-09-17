/*###########################################################
# Auto Superstar
###########################################################*/
// Author: Nuche17
// Indicator bytes, progress bytes and the state machine: docs/auto_superstar.md
#include "Include/game/UnknownHomes_Game.h"
#include "Include/static/UnknownHomes_Static.h"

// Claimed progress byte per team (0x802EBF99 / 0x802EBF9A)
#define SuperstarIndex(team)     VAR_ADDRESS(u8, 0x802EBF99 + (team))
#define INDEX_NOT_STARTED        0
#define INDEX_PARK_CURSOR        0xA

// The game indexes both by team, so P2's byte is the one after P1's
#define TeamCursor(team)         aiPosSwapInputs.teamManagement_cursorPos[team]
#define SuperstarInProgress(team) (&aiPosSwapInputs.inProgress_superStarAPlayer)[team]
#define SuperstarIndicator(team, slot) inMemRoster[team][slot].stats.UnusedBytes[0]

CGECKO(AutoSuperstar, .address = 0x8005A4F4, .instruction = "lis r3, -0x7FCD",
       .notes = "Automatically applies superstar status to the marked characters when a\n"
                "team loads. Meant to be driven by other tools, not turned on by itself.");
void AutoSuperstar(void)
{
    READ_GAME_REG(int, team, 27);

    if (team > 1)
        return;

    u8 index = SuperstarIndex(team);

    if (index > INDEX_PARK_CURSOR)
        return;

    if (index == INDEX_PARK_CURSOR)
        TeamCursor(team) = 0;
    else if (index != INDEX_NOT_STARTED)
    {
        TeamCursor(team) = index;
        SuperstarInProgress(team) = SuperstarIndicator(team, index - 1);
    }

    SuperstarIndex(team) = index + 1;
}
