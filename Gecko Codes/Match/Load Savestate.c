/*###########################################################
# Load Savestate
###########################################################*/
// Author: LittleCoaks, bko
// The raw controller records this reads and the status it forces: docs/match_codes.md
#include "Include/game/UnknownHomes_Game.h"

/* The game's raw SI poll results, 8 bytes per port; bit 7 of the low button
   byte is always set in raw data. (The synced header calls this block
   g_InputBuffer and types it as PADStatus, which is 12 bytes per port.) */
typedef struct {
    u8 buttonsHi;
    u8 buttonsLo;
    s8 stickX, stickY;
    s8 cStickX;
    u8 cStickY;
    u8 triggerL, triggerR;
} RawPad;
#define g_RawPads ARRAY_1D_ADDRESS(RawPad, 4, 0x802E9F40)

#define RAWPAD_LO_ALWAYS_SET 0x80
#define RESTART_COMBO (RAWPAD_LO_ALWAYS_SET | PAD_TRIGGER_L | PAD_TRIGGER_Z)

CGECKO(RestartAtBat, .address = 0x806AA1F0, .state = MSSB_GAME,
       .instruction = "lbz r4, 0x11E(r27)",
       .notes = "Press L + Z to restart the current at-bat.\n"
                "Only works once the ball is in play, using the\n"
                "game's replay system; not while the pitch is\n"
                "still going on.");
void RestartAtBat(void)
{
    if (g_GameLogic.gameStatus != GAME_STATUS_LIVE_BALL)
        return;

    for (int port = 0; port < 4; port++)
    {
        if ((g_RawPads[port].buttonsLo & RESTART_COMBO) != RESTART_COMBO)
            continue;
        g_GameLogic.gameStatus = GAME_STATUS_TRANSITION_PREPARE_NEXT_PLAY;
        return;
    }
}
