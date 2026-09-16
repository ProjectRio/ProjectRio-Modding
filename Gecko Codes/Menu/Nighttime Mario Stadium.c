/*###########################################################
# Nighttime Mario Stadium
###########################################################*/
// Author: LittleCoaks
// The stadium-select write site and the day/night byte: docs/stadium_files.md
#include "Include/game/UnknownHomes_Game.h"

#define STADIUM_MARIO   0
#define STADIUM_NIGHT   1

CGECKO(NighttimeMarioStadium, .address = 0x80650678, .state = MSSB_MENU,
       .instruction = "stb r0, 0x58(r3)",
       .notes = "Mario Stadium is played at night\n"
                "in exhibition mode.");
void NighttimeMarioStadium()
{
    READ_GAME_REG(u8*, setup, 4);       // the match-setup struct

    if (setup[9] == STADIUM_MARIO)
        setup[0xA] = STADIUM_NIGHT;
}
