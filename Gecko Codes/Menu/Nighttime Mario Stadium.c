/*###########################################################
# Nighttime Mario Stadium
###########################################################*/
// Author: LittleCoaks
// The stadium-select write site and the day/night byte: docs/stadium_files.md
#include "Include/game/UnknownHomes_Game.h"

#define STADIUM_MARIO   0
#define STADIUM_NIGHT   1

// r0 (= 1) is live at the site, so the replaced `stb r0, 0x58(r3)` is done here
// rather than re-run after the wrapper has clobbered r0.
CGECKO(NighttimeMarioStadium, .address = 0x80650678, .state = MSSB_MENU,
       .notes = "Mario Stadium is played at night\n"
                "in exhibition mode.");
void NighttimeMarioStadium()
{
    register unsigned int frame __asm__("r30");
    u8* screen = *(u8* volatile*)(frame + 0x8);          // r3
    u8* setup  = *(u8* volatile*)(frame + 0x8 + 4);      // r4: the match-setup struct

    screen[0x58] = 1;
    if (setup[9] == STADIUM_MARIO)
        setup[0xA] = STADIUM_NIGHT;
}
