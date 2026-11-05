/*###########################################################
# Every At Bat Is A Star Chance
###########################################################*/
// Author: Nuche17
// Where the roll happens and why the first at-bat never gets one: docs/match_codes.md
#include "Include/game/UnknownHomes_Game.h"
#include "Include/static/UnknownHomes_Static.h"
#include "Include/Symbols/game.h"
#include "Include/Rio/PatchTable.h"

/* game.rel .data: the roll is RandomInt_Sim(100) < this (stock 30). */
#define g_StarChancePercent VAR_ADDRESS(u8, 0x807B76F0)
#define STAR_CHANCE_PERCENT 100

#define NOP 0x60000000

/* matchTransitionPrepareNextAB: `rlwinm. r0, runnersAtStartOfPlayMask, 0, 16, 27`
   then `bne` past the roll when anyone is on base. */
static const RioPatch STAR_CHANCE_PATCHES[] = {
    { 0x8069E774, 0x40820028, NOP },
};

#define animRelated ARRAY_1D_ADDRESS(u8, 0x124, animRelated_ADDR)
#define STAR_CHANCE_BANNER  0xAC     /* 1 = show the "Star Chance" HUD banner */
#define MAX_STARS           5

/* Per frame: matchTransitionPrepareNextAB reads these before every at-bat after
   the first. game.rel .data/.text, so they are rewritten after each reload. */
CGECKO(EveryAtBatIsAStarChance, .state = MSSB_GAME,
       .notes = "Every at-bat is a star chance, including the\n"
                "first one of the game and with runners on base.");
void EveryAtBatIsAStarChance(void)
{
    g_StarChancePercent = STAR_CHANCE_PERCENT;
    RioPatch_Apply(STAR_CHANCE_PATCHES, RIO_PATCH_COUNT(STAR_CHANCE_PATCHES), CGECKO_ACTIVE);
}

/* exhibitionGameTransitionCalculations, case 10: replaces the `li r3, 0x16`
   before SetGameStatus(GAME_STATUS_STAR_CHANCE_VS) for the game's first
   at-bat, which never rolls. Same conditions as the stock roll, minus the
   runner check (no runners yet). */
CGECKO(EveryAtBatIsAStarChance_FirstAtBat, .address = 0x8069EEF4, .state = MSSB_GAME,
       .instruction = "li r3, 0x16");
void EveryAtBatIsAStarChance_FirstAtBat(void)
{
    if (inningSetting.starSkillsSetting == 0)
        return;
    if (!g_d_GameSettings.exhibitionMatchInd)          /* challenge scout missions */
        return;
    if (g_GameLogic.TeamStars[0] >= MAX_STARS && g_GameLogic.TeamStars[1] >= MAX_STARS)
        return;

    g_GameLogic.IsStarChance = 1;
    animRelated[STAR_CHANCE_BANNER] = 1;
}
