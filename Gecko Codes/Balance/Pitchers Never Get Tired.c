/*###########################################################
# Pitchers Never Get Tired
###########################################################*/
// Author: LittleCoaks
#include "Gecko Codes/Balance/PitcherStamina.h"

CGECKO(PitchersNeverGetTired,
       .notes = "Pitchers never get tired, however many star pitches they throw or runs they allow.");
void PitchersNeverGetTired(void)
{
    for (int i = 0; i < PITCHER_STATS_COUNT; i++)
    {
        volatile u8* stamina = (volatile u8*)(PITCHER_STATS_BASE + i * PITCHER_STATS_SIZE + PITCHER_STAMINA_OFFSET);
        stamina[1] = FULL_PITCHER_STAMINA;
    }
}
