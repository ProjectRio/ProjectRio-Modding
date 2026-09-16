/*###########################################################
# ForceSwapBoot.h -- the menu->match force-swap boot sequence
###########################################################*/
// Author: LittleCoaks
// The sequence, its order, and the symbols it touches: docs/boot_to_match.md
// Call order at a boot site: RequestGameRel -> RegisterPlayers -> (captains)
// -> ClearTakenGrid (+ marks) -> (roster fill) -> StageRoster -> ... -> setPortOfEachPlayer().

#ifndef RIO_FORCESWAPBOOT_H
#define RIO_FORCESWAPBOOT_H

// UnknownHomes_Game.h must come BEFORE UnknownHomes_Static.h (its `inningSetting` macro mangles a field name)
#include "Include/game/UnknownHomes_Game.h"
#include "Include/static/UnknownHomes_Static.h"
#include "Include/Unknown/File_0x80065dec.h"
#include "Include/Unknown/File_0x80069854.h"
#include "Include/Unknown/File_0x80064754.h"
#include "Include/Unknown/File_0x800678cc.h"
#include "Include/Unknown/File_0x80064a04.h"
#include "Include/menus/text_0323C.h"
#include "Include/musyx/musyx.h"
#include "Include/Rio/RelTable.h"

#define FORCESWAP_REL_MENU 4
#define FORCESWAP_REL_GAME 5
#define FORCESWAP_BOOT_SFX 0x1bb   // rio bat sound effect: shows the game is starting

#define FORCESWAP_TAKEN_GRID_COUNT 54

// Flip rel to 5 and hand the loader the swap. Runs once: the per-frame
// guard at the call site is `rel == 4`.
static inline void ForceSwap_RequestGameRel(void)
{
    inningSetting.rel  = FORCESWAP_REL_GAME;
    RelLoader_Finished = 1;
    sndFXStartEx(FORCESWAP_BOOT_SFX, 0x40, 0x3f, 0x0);
}

// What capSS load + "P2 press A to join" would do. p2Port is the second
// human's physical port, 1-based (ignored for a CPU match, which is UNVERIFIED).
static inline void ForceSwap_RegisterPlayers(bool isCpuMatch, u8 p2Port)
{
    if (isCpuMatch)
    {
        g_d_GameSettings.p2_CPU_match_code = P2_CPU_CODE_1_PLAYER_GAME;
        Static_Stats_Tables.playerNumberByPort[0] = 0;
        Static_Stats_Tables.portsActiveInMatch[0] = 0;      // 0 = active, 0xFF = inactive
        Static_Stats_Tables.portsActiveInMatch[1] = 0xFF;
        Static_Stats_Tables.portsActiveInMatch[2] = 0xFF;
        Static_Stats_Tables.portsActiveInMatch[3] = 0xFF;
        Static_Stats_Tables.player2Ind = 0;
        g_MatchInfo.player2Ind2                    = 0;
    }
    else
    {
        g_d_GameSettings.p2_CPU_match_code = P2_CPU_CODE_2_PLAYER_GAME;
        Static_Stats_Tables.playerNumberByPort[0] = 0;      // P1 = port 1
        Static_Stats_Tables.playerNumberByPort[1] = (u8)(p2Port - 1);
        Static_Stats_Tables.portsActiveInMatch[0] = 0;
        Static_Stats_Tables.portsActiveInMatch[1] = 0;
        Static_Stats_Tables.portsActiveInMatch[2] = 0xFF;
        Static_Stats_Tables.portsActiveInMatch[3] = 0xFF;
        Static_Stats_Tables.player2Ind = 1;
        g_MatchInfo.player2Ind2                    = 1;
    }
}

// Grid bookkeeping the rest of the setup chain still expects; the caller marks its picks afterwards.
static inline void ForceSwap_ClearTakenGrid(void)
{
    for (int i = 0; i < FORCESWAP_TAKEN_GRID_COUNT; i++)
        Static_Stats_Tables.charOnCharacterGridSelected[i] = 0;
}

// The conversion chain, once, in loadDemoMatch's order. cursorPositions.roster
// (charIDs, positionSwapMapping, filled flags) must already be set.
static inline void ForceSwap_StageRoster(void)
{
    copyInfoToInMemRoster();
    teamLogoDetermination(0);
    teamLogoDetermination(1);
    unsure_FillRosterPositions(0);
    unsure_FillRosterPositions(1);
    characterSelectScreen(0);
    characterSelectScreen(1);
    setCaptainLocInRoster();
}

#endif
