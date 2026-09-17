/*###########################################################
# Toy Field Exhibition
###########################################################*/
// Author: LittleCoaks
// Why the hook sits two instructions before the stadium-id store: docs/stadium_files.md
#include "Include/static/UnknownHomes_Static.h"
#include "Include/Dolphin/pad.h"

#define SOUND_STADIUM_CONFIRM      0x1B8
#define SOUND_TOY_FIELD_CONFIRM    0x1BC

// r5 already holds the chosen stadium id here; the game stores it to setup+9 right after
CGECKO(ToyFieldExhibition, .address = 0x8065066C, .state = MSSB_MENU,
       .instruction = "li r0, 1",
       .notes = "Hold L while choosing a stadium in exhibition mode to play on Toy Field.");
void ToyFieldExhibition(void)
{
    for (int port = 0; port < 4; port++)
    {
        if (g_InputBuffer.pads[port].button & PAD_TRIGGER_L)
        {
            WRITE_GAME_REG(5, STADIUM_ID_TOY_FIELD);
            return;
        }
    }
}

// Replaces `li r3, 0x1B8`: the confirm sound id handed to the sound player
CGECKO(ToyFieldExhibition_PlaySound, .address = 0x80640D54, .state = MSSB_MENU);
void ToyFieldExhibition_PlaySound(void)
{
    int sound = SOUND_STADIUM_CONFIRM;

    if (g_d_GameSettings.StadiumID + inningSetting.currentScene == 0x12)
        sound = SOUND_TOY_FIELD_CONFIRM;

    WRITE_GAME_REG(3, sound);
}
