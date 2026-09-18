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
       .notes = "Superstars every character on both teams as the team-management\n"
                "screen walks its roster.");
void AutoSuperstar(void)
{
    READ_GAME_REG(int, team, 27);

    if (team > 1)
        return;

    for (int slot = 0; slot < 9; slot++)
        SuperstarIndicator(team, slot) = 1;

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

// reset the walk every time the team-management screen is entered
// via a second hook within createTeamManagementScreen_preGame
CGECKO(AutoSuperstarRestart, .address = 0x80048764, .instruction = "stwu r1, -0x10(r1)",
       .notes = "Restarts the superstar walk above whenever the team-management\n"
                "screen is entered.");
void AutoSuperstarRestart(void)
{
    SuperstarIndex(0) = INDEX_NOT_STARTED;
    SuperstarIndex(1) = INDEX_NOT_STARTED;
}
