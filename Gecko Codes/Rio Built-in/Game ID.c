/*###########################################################
# Game ID
###########################################################*/
// Author: LittleCoaks
// *Manages Game ID for stat files
#include "Include/static/UnknownHomes_Static.h"

// Claimed word (ClaimedFreeMemory.h: "(w) -- GameID"). The Rio client polls
// it: non-zero = a match is in progress and this is its id (MSB_StatTracker /
// Core.cpp read aGameId = 0x802EBF8C); 0 = no match, and the tracker treats a
// non-zero -> 0 transition mid-game as a quit.
#define GameID VAR_ADDRESS(u32, 0x802EBF8C)

/* ---- Generate ---------------------------------------------------------------
   The id used to be rolled on the menu's Start Game button (0x80042CCC, inside
   the DOL's start-game sound routine). Anything that boots a match without
   pressing that button -- Instant Randoms and the other force-swap boots --
   never got an id, so the client never wrote a stat file for it.

   Now it is rolled when game.rel is LINKED instead. The DOL loader,
   handleLoadingProcess (0x800097A0), links a REL in two places, each an OSLink
   followed by `bctrl` into the new REL's _prolog:
     - state 9  (0x80009BA0): the boot-time load and the post-game "reload the
                menu slot" path -- menus.rel, in practice;
     - state 12 (0x80009D00): the menu -> match swap that loads game.rel.
   Both are hooked; the instruction after each bctrl is `li r0,0`, so r0 is
   dead there (the C2-in-C wrapper clobbers r0). The same sites fire for every
   REL, so the body checks which one just linked: inningSetting.rel is already
   5 by then, because the game (and every boot code) sets it before requesting
   the swap and the loader reads it to pick the file. No .state on purpose -- a
   Gecko state gate only controls when the branch is first written, it never
   removes it, so the check has to be in C anyway.

   Verified live 2026-09-03 under Instant Randoms: the state-9 site fires once
   at boot with rel == 0 (skipped), the state-12 site fires with rel == 5 and
   the client wrote the match's stat files.
   -------------------------------------------------------------------------- */
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

CGECKO(GenerateGameID_MenuSlot, .address = 0x80009BA4, .instruction = "li r0, 0");
void GenerateGameID_MenuSlot(void) { RollGameID(); }

CGECKO(GenerateGameID_MatchSlot, .address = 0x80009D04, .instruction = "li r0, 0");
void GenerateGameID_MatchSlot(void) { RollGameID(); }

/* ---- Clear ------------------------------------------------------------------
   Written as ASM, not C: every site overwrites a `stb r0, ...`, and r0 is the
   value being stored. The C2-in-C wrapper clobbers r0 (mflr r0) before the body
   runs, so a C hook here would corrupt that store. Each ASM body re-runs the
   original instruction itself and touches only registers the game has already
   consumed at that point (r18 / r8 / r0, exactly as the old .asm did).
   All three sites are in game.rel, hence .state = MSSB_GAME.
   -------------------------------------------------------------------------- */

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
