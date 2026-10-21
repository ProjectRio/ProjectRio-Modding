/*###########################################################
# CPU vs CPU
###########################################################*/
// Author: Nuche17

#include "Include/static/UnknownHomes_Static.h"

CGECKO(CPUvsCPU, .state = MSSB_GAME,
       .notes = "Both teams are controlled by the CPU.");
void CPUvsCPU()
{
    g_d_GameSettings.p2_CPU_match_code = 2; // 2 = CPU vs CPU    
}