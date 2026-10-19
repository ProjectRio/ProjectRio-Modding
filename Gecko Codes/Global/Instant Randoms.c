/*###########################################################
# Boot Directly To Game
###########################################################*/
// Author: LittleCoaks
// Force-swap sequence and the symbols it touches: docs/boot_to_match.md
#include "Include/Rio/ForceSwapBoot.h"
#include "Include/Rio/DupCharModelBind.h"
#include "Include/Unknown/File_0x80042bf0.h"
#include "Include/Unknown/File_0x800671fc.h"
#include "Include/Unknown/File_0x800426dc.h"

#define N_CAPTAINS 12   // mapCaptainCursorPositionToCharID is the capSS grid


#define N_DRAFTABLE 54   // character IDs 0..53; every slot may repeat any of them

static void DraftRandomTeamWithDupes(int team)
{
    cursorPositions.roster.rosterCharID[team][0]        = (u8)Static_Stats_Tables.captainSelectedID[team];
    cursorPositions.roster.rosterSpotFilledInd[team][0] = 1;
    for (int slot = 1; slot < 9; slot++)
    {
        cursorPositions.roster.rosterCharID[team][slot]        = (u8)randRange_FUN_80042bf0(N_DRAFTABLE - 1, 0);
        cursorPositions.roster.rosterSpotFilledInd[team][slot] = 1;
    }
}

// No .state on purpose: this code requests the rel swap and keeps running
// through the transition, so the menu gate is the `rel != 4` check in C.
CGECKO(InstantRandoms,
       .notes = "Boots directly into a random 5-inning match, P1 vs P2, with duplicate\n"
                "characters allowed. Randomizes home/away, superstars, and star skills.");
void InstantRandoms()
{
    if (inningSetting.rel != FORCESWAP_REL_MENU)
        return;

    ForceSwap_RequestGameRel();
    ForceSwap_RegisterPlayers(false, 2);   // both humans, P2 on port 2

    // two random captains off the game's own captain-select grid
    int cap_cell_0 = randRange_FUN_80042bf0(N_CAPTAINS - 1, 0);
    int cap_cell_1 = randRange_FUN_80042bf0(N_CAPTAINS - 1, 0);
    // nudge rather than re-roll: a reroll loop could never terminate
    if (cap_cell_1 == cap_cell_0)
        cap_cell_1 = (cap_cell_1 + 1 < N_CAPTAINS) ? cap_cell_1 + 1 : 0;

    u8 captain0 = mapCaptainCursorPositionToCharID[cap_cell_0];
    u8 captain1 = mapCaptainCursorPositionToCharID[cap_cell_1];

    Static_Stats_Tables.captainSelectedID[0] = captain0;
    Static_Stats_Tables.captainSelectedID[1] = captain1;
    ForceSwap_ClearTakenGrid();
    Static_Stats_Tables.charOnCharacterGridSelected[captain0] = 1;
    Static_Stats_Tables.charOnCharacterGridSelected[captain1] = 1;

    DraftRandomTeamWithDupes(0);
    DraftRandomTeamWithDupes(1);

    ForceSwap_StageRoster();

    PatchInstruction_Conditional(0x80067220, 0x38800005, 0x38800006); // allow random toy field
    selectRandomStadium();

    // inningSetting holds the actual inning COUNT, not a menu cursor index
    inningSetting.inningCount = 5;

    // 0 = P1 away / P2 home, 1 = the reverse
    g_d_GameSettings.home_AwaySetting = (u8)randRange_FUN_80042bf0(1, 0);

    // superstars are all-or-nothing this match
    int allSuperstars = randRange_FUN_80042bf0(1, 0);

    // star skills: randomized when superstars are on, always on otherwise
    inningSetting.starSkillsSetting = (u8)(allSuperstars ? randRange_FUN_80042bf0(1, 0) : 1);

    // drive the team-management menu's superstar primitive directly (cursor is 1-indexed)
    if (allSuperstars)
    {
        for (int team = 0; team < 2; team++)
        {
            for (int slot = 1; slot <= 9; slot++)
            {
                aiPosSwapInputs.teamManagement_cursorPos[team] = (u8)slot;
                transferStatsToInMemRoster(team);
            }
            aiPosSwapInputs.teamManagement_cursorPos[team] = 0; // park the cursor, as the menu does
        }
    }

    setPortOfEachPlayer();
}

CGECKO(ToyFieldBanBunting, .address = 0x80009404, .state = MSSB_GAME,
                           .instruction = "mr r3, r25");
void ToyFieldBanBunting()
{
    if (g_d_GameSettings.StadiumID != STADIUM_ID_TOY_FIELD)
        return;
    PatchInstruction(0x806532F0, 0x48000034);   // b +0x34 -- branch over the bunt path
}


// Bundled codes: the chemistry writer for duplicate rosters, and (TEMPORARY,
// until the Rio client ships the REL-link Game ID code) the Game ID roll.
#include "Gecko Codes/Menu/Duplicate Characters.c"
#include "Gecko Codes/Rio Built-in/Game ID.c"
