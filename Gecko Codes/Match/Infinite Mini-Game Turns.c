/*###########################################################
# Infinite Mini-Game Turns
###########################################################*/
// Author: Codejunkies
#include "Include/types.h"

#define MINIGAME_PLAYERS 4
#define MinigameTurnsLeft ((u8*)0x80366148)

CGECKO(InfiniteMiniGameTurns, .notes = "Every player always has 3 turns left in minigames.");
void InfiniteMiniGameTurns(void)
{
    for (int player = 0; player < MINIGAME_PLAYERS; player++)
        MinigameTurnsLeft[player] = 3;
}
