/*###########################################################
# Checksum
###########################################################*/
// Author: LittleCoaks
// What goes into the sum, and where it lands: docs/netplay_checksum.md
#include "Include/game/UnknownHomes_Game.h"
#include "Include/static/UnknownHomes_Static.h"
#include "Include/Symbols/dol.h"
#include "Include/Symbols/game.h"

#define DesyncChecksum    VAR_ADDRESS(u32, 0x802EBFB8)     /* see ClaimedFreeMemory.h */

// Game state the decomp has not named yet
#define PickoffAttempt    VAR_ADDRESS(u8, 0x80892857)
#define GameIsLive        VAR_ADDRESS(u8, hugeAnimStruct_ADDR + 0xE61)
#define IsReplay          VAR_ADDRESS(u8, 0x80872540)      /* g_Camera + 0xAD4 */
#define OutsDuringPlay    VAR_ADDRESS(u8, animRelated_ADDR + 0xA9)
#define FinalResult       VAR_ADDRESS(u8, 0x80893BAA)

#define ROSTER_SLOTS      18
#define HIGH_BYTE(s16v)   ((u8)((s16v) >> 8))

static u32 SumMenuState(void)
{
    return inningSetting.currentScene
         + inningSetting.previousScene
         + *(u16*)g_d_GameSettings.PlayerPorts;
}

static u32 SumTeams(void)
{
    const u8* charIDs = (const u8*)cursorPositions.roster.rosterCharID;
    const u8* starred = Static_Stats_Tables.charIsStarred;
    u32 sum = 0;

    for (int i = 0; i < ROSTER_SLOTS; i++)
        sum += charIDs[i] + i + starred[i] + i;
    return sum;
}

static u32 SumMatchState(void)
{
    u32 sum = 0;

    sum += g_GameLogic.gameStatus;
    sum += g_GameLogic.gameStatus_prev;
    sum += g_Batter.contactMadeInd;
    sum += PickoffAttempt;
    sum += GameIsLive;
    sum += g_Batter.noSwingAnimationInd;
    sum += IsReplay;
    sum += (u8)g_Batter.rosterID;
    sum += (u8)g_Scores.Inning;
    sum += g_Scores.halfInning;
    sum += (u8)g_Strikes.balls;
    sum += (u8)g_Strikes.strikes;
    sum += (u8)g_Strikes.outs;
    sum += g_GameLogic.TeamStars[0];
    sum += g_GameLogic.TeamStars[1];
    sum += g_GameLogic.IsStarChance;
    sum += g_Batter.chemLinksOnBase;
    sum += g_Runners[1].runnerDidntReachOnError;
    sum += g_Runners[2].runnerDidntReachOnError;
    sum += g_Runners[3].runnerDidntReachOnError;
    sum += OutsDuringPlay;
    sum += g_Batter.hitByPitch;
    sum += FinalResult;
    sum += HIGH_BYTE(g_Scores.scores[0].total);
    sum += HIGH_BYTE(g_Scores.scores[1].total);
    sum += (u8)g_Batter.rosterID;
    sum += (u8)g_Pitcher.rosterID;
    sum += *(u32*)&g_Ball.AtBat_Contact_BallPos.x;
    sum += *(u32*)&g_Ball.AtBat_Contact_BallPos.y;
    sum += *(u32*)&g_Ball.AtBat_Contact_BallPos.z;
    return sum;
}

CGECKO(Checksum, .address = 0x8000928C, .instruction = "cmplwi r24, 0",
       .notes = "Netplay sync check: sums up game state so Rio can confirm every player's game agrees.");
void Checksum(void)
{
    DesyncChecksum = SumMenuState() + SumTeams() + SumMatchState();
}
