/*###########################################################
# Game ID
###########################################################*/
// Author: LittleCoaks
// Why the id is rolled at the REL link sites, and which site is which: docs/rel_loader.md
#include "Include/static/UnknownHomes_Static.h"

// Claimed word the Rio client polls: non-zero = a match is in progress and this is its id
#define GameID VAR_ADDRESS(u32, 0x802EBF8C)

// Both REL link sites fire for every REL, so check which one just linked here.
static void RollGameID(void)
{
    if (inningSetting.rel != 5)                                  // menus.rel / debug.rel link here too
        return;
    if (g_d_GameSettings.GameModeSelected == GAME_TYPE_DEMO)     // title-screen attract match: not a game
        return;

    u32 tb;
    __asm__ volatile("mftb %0" : "=r"(tb));                      // time base low word, as before
    GameID = tb;
}

CGECKO(GenerateGameID_MenuSlot, .address = 0x80009BA4, .instruction = "li r0, 0",
       .notes = "Keeps track of a game ID so Rio can save stat files for each match.");
void GenerateGameID_MenuSlot(void) { RollGameID(); }

CGECKO(GenerateGameID_MatchSlot, .address = 0x80009D04, .instruction = "li r0, 0");
void GenerateGameID_MatchSlot(void) { RollGameID(); }

// Clear sites are ASM: each overwrites a `stb r0, ...` and the C2-in-C wrapper clobbers r0.

// Game over: returning to the main menu after the game ends
ASM(ClearGameID_End,
    "stb  0, 293(5)      \n"      /* overwritten instruction */
    "lis  18, 0x802E     \n"
    "ori  18, 18, 0xBF8C \n"
    "li   3, 0           \n"
    "stw  3, 0(18)       \n",
    .address = 0x8069AB2C, .state = MSSB_GAME);

// Quit mid-game from the pause menu
ASM(ClearGameID_Quit,
    "stb  0, 466(31)     \n"      /* overwritten instruction */
    "lis  8, 0x802E      \n"
    "ori  8, 8, 0xBF8C   \n"
    "li   0, 0           \n"
    "stw  0, 0(8)        \n",
    .address = 0x806ED704, .state = MSSB_GAME);

// Quit mid-game, second path
ASM(ClearGameID_Quit2,
    "stb  0, 466(31)     \n"      /* overwritten instruction */
    "lis  8, 0x802E      \n"
    "ori  8, 8, 0xBF8C   \n"
    "li   0, 0           \n"
    "stw  0, 0(8)        \n",
    .address = 0x806EDF8C, .state = MSSB_GAME);
