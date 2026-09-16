/*###########################################################
# Boot Directly To Game
###########################################################*/
// Author: LittleCoaks
// Force-swap sequence and the symbols it touches: docs/boot_to_match.md
// UnknownHomes_Game.h must come BEFORE UnknownHomes_Static.h (its `inningSetting` macro mangles a field name)
#include "Include/game/UnknownHomes_Game.h"
#include "Include/static/UnknownHomes_Static.h"
#include "Include/Unknown/File_0x80065dec.h"
#include "Include/Unknown/File_0x80042bf0.h"
#include "Include/Unknown/File_0x800671fc.h"
#include "Include/Unknown/File_0x80069854.h"
#include "Include/Unknown/File_0x80064754.h"
#include "Include/Unknown/File_0x800678cc.h"
#include "Include/Unknown/File_0x800426dc.h"
#include "Include/Unknown/File_0x80064a04.h"
#include "Include/menus/text_0323C.h"
#include "Include/musyx/musyx.h"


// menu->game rel swap request (loader node 0x80111300 + 0x10)
#define trigger_rel_change VAR_ADDRESS(short, 0x80111310)

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
    if (inningSetting.rel != 4)
        return;

    inningSetting.rel = 5;
    trigger_rel_change = 1;
    sndFXStartEx(0x1bb, 0x40, 0x3f, 0x0); // rio bat sound effect: shows the game is starting

    // register both humans -- what capSS load + "P2 press A to join" would do
    g_d_GameSettings.p2_CPU_match_code = P2_CPU_CODE_2_PLAYER_GAME;
    Static_Stats_Tables.playerNumberByPort[0] = 0;      // P1 = port 1
    Static_Stats_Tables.playerNumberByPort[1] = 1;      // P2 = port 2
    Static_Stats_Tables.portsActiveInMatch[0] = 0;      // 0 = active, 0xFF = inactive
    Static_Stats_Tables.portsActiveInMatch[1] = 0;
    Static_Stats_Tables.portsActiveInMatch[2] = 0xFF;
    Static_Stats_Tables.portsActiveInMatch[3] = 0xFF;
    Static_Stats_Tables.player2Ind = 1;
    g_MatchInfo.player2Ind2                    = 1;

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
    // grid bookkeeping the rest of the setup chain still expects
    for (int i = 0; i < 54; i++)
        Static_Stats_Tables.charOnCharacterGridSelected[i] = 0;
    Static_Stats_Tables.charOnCharacterGridSelected[captain0] = 1;
    Static_Stats_Tables.charOnCharacterGridSelected[captain1] = 1;

    DraftRandomTeamWithDupes(0);
    DraftRandomTeamWithDupes(1);

    // conversion chain, once, in loadDemoMatch's order
    copyInfoToInMemRoster();
    teamLogoDetermination(0);
    teamLogoDetermination(1);
    unsure_FillRosterPositions(0);
    unsure_FillRosterPositions(1);
    characterSelectScreen(0);
    characterSelectScreen(1);
    setCaptainLocInRoster();

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

// Fix for duplicate characters (docs/duplicate_characters.md). No .instruction
// on either hook: both REPLACE the instruction they overwrite.
CGECKO(DupLoadBindCharID, .address = 0x800156B0);
void DupLoadBindCharID()
{
    READ_GAME_REG(u32, entry, 5);                  // r5 = &table[r9]
    u32 obj = *(u32*)entry;                        // table[r9]
    WRITE_GAME_REG(3, obj ? obj : 0x80370F1C);
}

CGECKO(DupLoadBindStore, .address = 0x800156E4);
void DupLoadBindStore()
{
    // READ_GAME_REG can't be used twice in one function; saved r<n> is at r30 + 0x8 + (n-3)*4
    register u32 _fp __asm__("r30");
    u32 r3    = *(volatile u32*)(_fp + 0x8);              // saved r3 = table[r9]
    u32 r8    = *(volatile u32*)(_fp + 0x8 + ((8 - 3) << 2)); // saved r8
    u32 model = *(u32*)(0x8036E548 + (r8 << 2) + 11456);  // model[r8]
    *(u32*)((r3 ? r3 : 0x80370F1C) + 24) = model;          // bind; scratch on NULL
}


// Bundled codes: the chemistry writer for duplicate rosters, and (TEMPORARY,
// until the Rio client ships the REL-link Game ID code) the Game ID roll.
#include "Gecko Codes/Menu/Duplicate Characters.c"
#include "Gecko Codes/Rio Built-in/Game ID.c"
