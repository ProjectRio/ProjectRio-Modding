/*###########################################################
# Store Random Batting Ints
###########################################################*/
// Author: Roeming
#include "Include/game/UnknownHomes_Game.h"

// Claimed halfwords the Rio client reads (ClaimedFreeMemory.h)
#define BattingRandomInt1 VAR_ADDRESS(u16, 0x802EC010)
#define BattingRandomInt2 VAR_ADDRESS(u16, 0x802EC012)
#define BattingRandomInt3 VAR_ADDRESS(u16, 0x802EC014)

// Replaces calculateIfHitBall's final `stb r0, 0x91(r4)`; r0 is the constant 1
// set three instructions earlier, so the store is replicated rather than re-run.
CGECKO(StoreRandomBattingInts, .address = 0x80651E68, .state = MSSB_GAME,
       .notes = "Saves the game's random batting values where Rio can read them for stat tracking.");
void StoreRandomBattingInts(void)
{
    READ_GAME_REG(InMemBatterType*, batter, 4);
    batter->contactMadeInd = 1;

    BattingRandomInt1 = (u16)g_Ball.StaticRandomInt1;
    BattingRandomInt2 = (u16)g_Ball.StaticRandomInt2;
    BattingRandomInt3 = (u16)g_Ball.totalFramesAtPlay;
}
