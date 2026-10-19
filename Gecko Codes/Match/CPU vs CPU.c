/*###########################################################
# CPU vs CPU
###########################################################*/
// Author: LittleCoaks

#include "Include/game/UnknownHomes_Game.h"

CGECKO(CPUvsCPU, .state = MSSB_GAME,
       .notes = "Both teams are controlled by the CPU.\n"
                "Known issue: the CPU never charge swings.");
void CPUvsCPU()
{
    g_GameLogic.AIDifficulty0Special3Weak[0] = 0;
    g_GameLogic.AIDifficulty0Special3Weak[1] = 0;

    g_GameLogic.teamAIInd[0] = 1;
    g_GameLogic.teamAIInd[1] = 1;
    g_GameLogic.runnerAIInd[0] = 1;
    g_GameLogic.runnerAIInd[1] = 1;
    g_GameLogic.battingAIInd[0] = 1;
    g_GameLogic.battingAIInd[1] = 1;

    g_GameLogic.teamIsCPU[0] = 1;
    g_GameLogic.teamIsCPU[1] = 1;
    g_GameLogic.autoFielding[0] = 1;
    g_GameLogic.autoFielding[1] = 1;

    g_Pitcher.AIInd = 1;
    g_Pitcher.aiLevel = 0;
    g_Batter.aiControlledInd = 1;
    g_Batter.aiLevel= 0;
}