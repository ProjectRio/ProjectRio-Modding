/*###########################################################
# Disable Haze on Every Stadium
###########################################################*/
// Author: Roeming
#include "Include/types.h"

#define HAZE_LAYER_COUNT 3
#define HazeLayerEnabled(i) VAR_ADDRESS(u8, 0x8086EF38 + (i) * 0x10)

CGECKO(DisableHazeOnEveryStadium, .state = MSSB_GAME,
       .notes = "Removes the distance haze from every stadium.");
void DisableHazeOnEveryStadium(void)
{
    for (int i = 0; i < HAZE_LAYER_COUNT; i++)
        HazeLayerEnabled(i) = 0;
}
