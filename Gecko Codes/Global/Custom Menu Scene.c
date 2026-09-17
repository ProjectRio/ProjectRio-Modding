/*###########################################################
# Custom Menu Scene (hello-world)
###########################################################*/
// Author: LittleCoaks
// Proves the menu-scene hijack end to end by replacing the Records scene
// (screenCode 8). Dispatch, the clean-replace trick and the no-statics
// hazard: docs/screen_dispatch.md

// no payload statics (cgecko PIC hazard): all scene state lives in claimed RAM
#define SCREENTEXT_NO_FLOAT
#define TEXT_SLOTS 4
#define TEXT_BUFFER_ADDR 0x802EC0E8   // 392 bytes, see ClaimedFreeMemory.h
#include "Include/Rio/ScreenText.h"

#include "Include/static/UnknownHomes_Static.h"
#include "Include/menus/yd_step.h"
#include "Include/text/text_channel.h"
#define g_magic  VAR_ADDRESS(u32, 0x802EC270)   // one-shot init sentinel
#define g_aCount VAR_ADDRESS(s32, 0x802EC274)

// .state = MSSB_MENU is required: 0x8069D198 is menus.rel code.
CGECKO(CustomMenuScene, .address = 0x8069D198, .state = MSSB_MENU,
                        .instruction = "blr",
                        .notes = "Developer test code. Leave this off.\n"
                                 "Replaces the Records screen with a test screen.");
void CustomMenuScene()
{
    if (g_magic != 0x5CE7E)         // one-shot init of the claimed-RAM counter
    {
        g_magic  = 0x5CE7E;
        g_aCount = 0;
    }

    // newInput is already edge-detected by the menu's input gather
    controllerInputStruct* in = Static_Stats_Tables.controllerInputs;
    u16 pressed = in[0].newInput;

    if (pressed & INPUT_BUTTON_A)
        g_aCount++;

    if (pressed & INPUT_BUTTON_B)
    {
        changeScreenVariables(5);   // back to the main menu
        return;                     // draw nothing on the exit frame
    }

    ScreenTextTick();
    WriteTextEx(320, 120, TEXT_WHITE,  TEXT_LARGE, TEXT_CENTER, "CUSTOM MENU SCENE");
    WriteTextEx(320, 172, TEXT_GREEN,  TEXT_SMALL, TEXT_CENTER, "Menu-scene hijack works!");
    WriteTextEx(320, 214, TEXT_YELLOW, TEXT_SMALL, TEXT_CENTER, "A pressed %d times", g_aCount);
    WriteTextEx(320, 262, TEXT_WHITE,  TEXT_SMALL, TEXT_CENTER, "Press B to return");
}
